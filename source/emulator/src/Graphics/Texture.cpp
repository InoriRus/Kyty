#include "Emulator/Graphics/Texture.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/Tile.h"
#include "Emulator/Graphics/Utils.h"
#include "Emulator/Profiler.h"

#include <vulkan/vulkan_core.h>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

static VkFormat get_texture_format(uint32_t dfmt, uint32_t nfmt)
{
	if (nfmt == 9 && dfmt == 10)
	{
		return VK_FORMAT_R8G8B8A8_SRGB;
	}
	if (nfmt == 9 && dfmt == 37)
	{
		return VK_FORMAT_BC3_SRGB_BLOCK;
	}
	EXIT("unknown format: nfmt = %u, dfmt = %u\n", nfmt, dfmt);
	return VK_FORMAT_UNDEFINED;
}

static VkComponentSwizzle get_swizzle(uint8_t s)
{
	switch (s)
	{
		case 0: return VK_COMPONENT_SWIZZLE_ZERO; break;
		case 1: return VK_COMPONENT_SWIZZLE_ONE; break;
		case 4: return VK_COMPONENT_SWIZZLE_R; break;
		case 5: return VK_COMPONENT_SWIZZLE_G; break;
		case 6: return VK_COMPONENT_SWIZZLE_B; break;
		case 7: return VK_COMPONENT_SWIZZLE_A; break;
		case 2:
		case 3:
		default: EXIT("unknown swizzle: %d\n", static_cast<int>(s));
	}
	return VK_COMPONENT_SWIZZLE_IDENTITY;
}

void* TextureObject::Create(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, VulkanMemory* mem) const
{
	KYTY_PROFILER_BLOCK("TextureObject::Create");

	EXIT_IF(size == nullptr || vaddr == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(ctx == nullptr);

	auto dfmt    = params[PARAM_DFMT];
	auto nfmt    = params[PARAM_NFMT];
	auto width   = params[PARAM_WIDTH];
	auto height  = params[PARAM_HEIGHT];
	auto levels  = params[PARAM_LEVELS];
	auto swizzle = params[PARAM_SWIZZLE];

	auto pixel_format = get_texture_format(dfmt, nfmt);

	EXIT_NOT_IMPLEMENTED(pixel_format == VK_FORMAT_UNDEFINED);
	EXIT_NOT_IMPLEMENTED(width == 0);
	EXIT_NOT_IMPLEMENTED(height == 0);

	auto* vk_obj = new TextureVulkanImage;

	vk_obj->extent.width  = width;
	vk_obj->extent.height = height;
	vk_obj->format        = pixel_format;
	vk_obj->image         = nullptr;
	vk_obj->image_view    = nullptr;

	VkImageCreateInfo image_info {};
	image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext         = nullptr;
	image_info.flags         = 0;
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.extent.width  = vk_obj->extent.width;
	image_info.extent.height = vk_obj->extent.height;
	image_info.extent.depth  = 1;
	image_info.mipLevels     = levels;
	image_info.arrayLayers   = 1;
	image_info.format        = vk_obj->format;
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples       = VK_SAMPLE_COUNT_1_BIT;

	vkCreateImage(ctx->device, &image_info, nullptr, &vk_obj->image);

	EXIT_NOT_IMPLEMENTED(vk_obj->image == nullptr);

	vkGetImageMemoryRequirements(ctx->device, vk_obj->image, &mem->requirements);

	mem->property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	bool allocated = VulkanAllocate(ctx, mem);

	EXIT_NOT_IMPLEMENTED(!allocated);

	VulkanBindImageMemory(ctx, vk_obj, mem);

	vk_obj->memory = *mem;

	// EXIT_NOT_IMPLEMENTED(mem->requirements.size > *size);

	GetUpdateFunc()(ctx, params, vk_obj, vaddr, size, vaddr_num);

	VkImageViewCreateInfo create_info {};
	create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.pNext                           = nullptr;
	create_info.flags                           = 0;
	create_info.image                           = vk_obj->image;
	create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format                          = vk_obj->format;
	create_info.components.r                    = get_swizzle(swizzle & 0xffu);
	create_info.components.g                    = get_swizzle((swizzle >> 8u) & 0xffu);
	create_info.components.b                    = get_swizzle((swizzle >> 16u) & 0xffu);
	create_info.components.a                    = get_swizzle((swizzle >> 24u) & 0xffu);
	create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.baseMipLevel   = 0;
	create_info.subresourceRange.layerCount     = 1;
	create_info.subresourceRange.levelCount     = 1;

	vkCreateImageView(ctx->device, &create_info, nullptr, &vk_obj->image_view);

	EXIT_NOT_IMPLEMENTED(vk_obj->image_view == nullptr);

	return vk_obj;
}

static void update_func(GraphicContext* ctx, const uint64_t* params, void* obj, const uint64_t* vaddr, const uint64_t* size, int vaddr_num)
{
	KYTY_PROFILER_BLOCK("TextureObject::update_func");

	EXIT_IF(obj == nullptr);
	EXIT_IF(ctx == nullptr);
	EXIT_IF(params == nullptr);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num != 1);

	auto* vk_obj = static_cast<TextureVulkanImage*>(obj);

	bool tile   = (params[TextureObject::PARAM_TILE] != 0);
	auto dfmt   = params[TextureObject::PARAM_DFMT];
	auto nfmt   = params[TextureObject::PARAM_NFMT];
	auto width  = params[TextureObject::PARAM_WIDTH];
	auto height = params[TextureObject::PARAM_HEIGHT];
	auto levels = params[TextureObject::PARAM_LEVELS];
	bool neo    = Config::IsNeo();

	EXIT_NOT_IMPLEMENTED(levels >= 16);

	uint32_t level_sizes[16];

	TileGetTextureSize(dfmt, nfmt, width, height, levels, tile, neo, nullptr, level_sizes, nullptr, nullptr);

	// dbg_test_mipmaps(ctx, VK_FORMAT_BC3_SRGB_BLOCK, 512, 512);

	uint32_t offset     = 0;
	uint32_t mip_width  = width;
	uint32_t mip_height = height;

	Vector<BufferImageCopy> regions(levels);
	for (uint32_t i = 0; i < levels; i++)
	{
		EXIT_NOT_IMPLEMENTED(level_sizes[i] == 0);

		regions[i].offset = offset;
		regions[i].width  = mip_width;
		regions[i].height = mip_height;

		offset += level_sizes[i];

		if (mip_width > 1)
		{
			mip_width /= 2;
		}
		if (mip_height > 1)
		{
			mip_height /= 2;
		}
	}

	if (tile)
	{
		auto* temp_buf = new uint8_t[*size];
		TileConvertTiledToLinear(temp_buf, reinterpret_cast<void*>(*vaddr), TileMode::TextureTiled, dfmt, nfmt, width, height, levels, neo);
		UtilFillImage(ctx, vk_obj, temp_buf, *size, regions);
		delete[] temp_buf;
	} else
	{
		UtilFillImage(ctx, vk_obj, reinterpret_cast<void*>(*vaddr), *size, regions);
	}
}

bool TextureObject::Equal(const uint64_t* other) const
{
	return (params[PARAM_DFMT] == other[PARAM_DFMT] && params[PARAM_NFMT] == other[PARAM_NFMT] &&
	        params[PARAM_WIDTH] == other[PARAM_WIDTH] && params[PARAM_HEIGHT] == other[PARAM_HEIGHT] &&
	        params[PARAM_LEVELS] == other[PARAM_LEVELS] && params[PARAM_TILE] == other[PARAM_TILE] &&
	        params[PARAM_NEO] == other[PARAM_NEO] && params[PARAM_SWIZZLE] == other[PARAM_SWIZZLE]);
}

static void delete_func(GraphicContext* ctx, void* obj, VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("TextureObject::delete_func");

	auto* vk_obj = reinterpret_cast<TextureVulkanImage*>(obj);

	EXIT_IF(vk_obj == nullptr);
	EXIT_IF(ctx == nullptr);

	DeleteDescriptor(vk_obj);

	vkDestroyImageView(ctx->device, vk_obj->image_view, nullptr);

	vkDestroyImage(ctx->device, vk_obj->image, nullptr);

	VulkanFree(ctx, mem);

	delete vk_obj;
}

GpuObject::delete_func_t TextureObject::GetDeleteFunc() const
{
	return delete_func;
}

GpuObject::update_func_t TextureObject::GetUpdateFunc() const
{
	return update_func;
}

} // namespace Kyty::Libs::Graphics

#endif
