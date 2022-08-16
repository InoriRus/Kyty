#include "Emulator/Graphics/Objects/Texture.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/Shader.h"
#include "Emulator/Graphics/Tile.h"
#include "Emulator/Graphics/Utils.h"
#include "Emulator/Profiler.h"

// IWYU pragma: no_forward_declare VkImageView_T

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

static VkFormat get_texture_format(uint32_t dfmt, uint32_t nfmt, uint32_t fmt)
{
	if (fmt == 0)
	{
		if (nfmt == 9 && dfmt == 10)
		{
			return VK_FORMAT_R8G8B8A8_SRGB;
		}
		if (nfmt == 0 && dfmt == 10)
		{
			return VK_FORMAT_R8G8B8A8_UNORM;
		}
		if (nfmt == 0 && dfmt == 1)
		{
			return VK_FORMAT_R8_UNORM;
		}
		if (nfmt == 0 && dfmt == 3)
		{
			return VK_FORMAT_R8G8_UNORM;
		}
		if (nfmt == 9 && dfmt == 37)
		{
			return VK_FORMAT_BC3_SRGB_BLOCK;
		}
		if (nfmt == 0 && dfmt == 37)
		{
			return VK_FORMAT_BC3_UNORM_BLOCK;
		}
		if (nfmt == 0 && dfmt == 36)
		{
			return VK_FORMAT_BC2_UNORM_BLOCK;
		}
		if (nfmt == 0 && dfmt == 35)
		{
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		}
		EXIT("unknown format: nfmt = %u, dfmt = %u\n", nfmt, dfmt);
	} else
	{
		if (fmt == 56)
		{
			return VK_FORMAT_R8G8B8A8_UNORM;
		}
		EXIT("unknown format: fmt = %u\n", fmt);
	}
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

static VkImageUsageFlags get_usage()
{
	VkImageUsageFlags vk_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	vk_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

	return vk_usage;
}

static bool CheckFormat(GraphicContext* ctx, VkImageCreateInfo* image_info)
{
	VkImageFormatProperties props {};
	if (vkGetPhysicalDeviceImageFormatProperties(ctx->physical_device, image_info->format, image_info->imageType, image_info->tiling,
	                                             image_info->usage, image_info->flags, &props) == VK_ERROR_FORMAT_NOT_SUPPORTED)
	{
		if (image_info->format == VK_FORMAT_R8G8B8A8_SRGB)
		{
			// TODO() convert SRGB -> LINEAR in shader
			image_info->format = VK_FORMAT_R8G8B8A8_UNORM;
			bool result        = CheckFormat(ctx, image_info);
			printf("replace VK_FORMAT_R8G8B8A8_SRGB => VK_FORMAT_R8G8B8A8_UNORM [%s]\n", (!result ? "FAIL" : "SUCCESS"));
			return result;
		}
		if (image_info->format == VK_FORMAT_B8G8R8A8_SRGB)
		{
			// TODO() convert SRGB -> LINEAR in shader
			image_info->format = VK_FORMAT_B8G8R8A8_UNORM;
			bool result        = CheckFormat(ctx, image_info);
			printf("replace VK_FORMAT_B8G8R8A8_SRGB => VK_FORMAT_B8G8R8A8_UNORM [%s]\n", (!result ? "FAIL" : "SUCCESS"));
			return result;
		}
		return false;
	}
	return true;
}

static bool CheckSwizzle(GraphicContext* /*ctx*/, VkImageCreateInfo* image_info, VkComponentMapping* components)
{
	if ((image_info->usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0)
	{
		if (components->r == VK_COMPONENT_SWIZZLE_R && components->g == VK_COMPONENT_SWIZZLE_G && components->b == VK_COMPONENT_SWIZZLE_B &&
		    components->a == VK_COMPONENT_SWIZZLE_A)
		{
			return true;
		}

		if (components->r == VK_COMPONENT_SWIZZLE_B && components->g == VK_COMPONENT_SWIZZLE_G && components->b == VK_COMPONENT_SWIZZLE_R &&
		    components->a == VK_COMPONENT_SWIZZLE_A && image_info->format == VK_FORMAT_R8G8B8A8_SRGB)
		{
			printf("replace VK_FORMAT_R8G8B8A8_SRGB => VK_FORMAT_B8G8R8A8_SRGB\n");

			components->r      = VK_COMPONENT_SWIZZLE_R;
			components->g      = VK_COMPONENT_SWIZZLE_G;
			components->b      = VK_COMPONENT_SWIZZLE_B;
			components->a      = VK_COMPONENT_SWIZZLE_A;
			image_info->format = VK_FORMAT_B8G8R8A8_SRGB;
			return true;
		}

		// TODO() swizzle channels in shader

		return false;
	}
	return true;
}

static void update_func(GraphicContext* ctx, const uint64_t* params, void* obj, const uint64_t* vaddr, const uint64_t* size, int vaddr_num)
{
	KYTY_PROFILER_BLOCK("TextureObject::update_func");

	EXIT_IF(obj == nullptr);
	EXIT_IF(ctx == nullptr);
	EXIT_IF(params == nullptr);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num != 1);

	auto* vk_obj = static_cast<TextureVulkanImage*>(obj);

	auto tile   = params[TextureObject::PARAM_TILE];
	auto fmt    = (params[TextureObject::PARAM_FORMAT] >> 16u) & 0xffffu;
	auto dfmt   = (params[TextureObject::PARAM_FORMAT] >> 8u) & 0xffu;
	auto nfmt   = (params[TextureObject::PARAM_FORMAT]) & 0xffu;
	auto width  = params[TextureObject::PARAM_WIDTH_HEIGHT] >> 32u;
	auto height = params[TextureObject::PARAM_WIDTH_HEIGHT] & 0xffffffffu;
	auto levels = params[TextureObject::PARAM_LEVELS] & 0xffffffffu;
	auto pitch  = params[TextureObject::PARAM_PITCH];
	bool neo    = Config::IsNeo();

	VkImageLayout vk_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	EXIT_NOT_IMPLEMENTED(levels >= 16);

	TileSizeOffset level_sizes[16];

	if (fmt != 0)
	{
		EXIT_NOT_IMPLEMENTED(tile != 0);

		TileGetTextureSize2(fmt, width, height, pitch, levels, tile, nullptr, level_sizes, nullptr);
	} else
	{
		EXIT_NOT_IMPLEMENTED(tile != 8 && tile != 13);

		TileGetTextureSize(dfmt, nfmt, width, height, pitch, levels, tile, neo, nullptr, level_sizes, nullptr);
	}

	// dbg_test_mipmaps(ctx, VK_FORMAT_BC3_SRGB_BLOCK, 512, 512);

	uint32_t mip_width  = width;
	uint32_t mip_height = height;
	uint32_t mip_pitch  = pitch;

	Vector<BufferImageCopy> regions(levels);
	for (uint32_t i = 0; i < levels; i++)
	{
		EXIT_NOT_IMPLEMENTED(level_sizes[i].size == 0);

		regions[i].offset    = level_sizes[i].offset;
		regions[i].width     = mip_width;
		regions[i].height    = mip_height;
		regions[i].pitch     = mip_pitch;
		regions[i].dst_level = i;
		regions[i].dst_x     = 0;
		regions[i].dst_y     = 0;

		if (mip_width > 1)
		{
			mip_width /= 2;
		}
		if (mip_height > 1)
		{
			mip_height /= 2;
		}
		if (mip_pitch > 1)
		{
			mip_pitch /= 2;
		}
	}

	if (fmt == 0)
	{
		if (tile == 13)
		{
			// EXIT_NOT_IMPLEMENTED(pitch != width);
			EXIT_NOT_IMPLEMENTED(fmt != 0);
			auto* temp_buf = new uint8_t[*size];
			TileConvertTiledToLinear(temp_buf, reinterpret_cast<void*>(*vaddr), TileMode::TextureTiled, dfmt, nfmt, width, height, pitch,
			                         levels, neo);
			UtilFillImage(ctx, vk_obj, temp_buf, *size, regions, static_cast<uint64_t>(vk_layout));
			delete[] temp_buf;
		} else if (tile == 8)
		{
			UtilFillImage(ctx, vk_obj, reinterpret_cast<void*>(*vaddr), *size, regions, static_cast<uint64_t>(vk_layout));
		}
	} else
	{
		if (tile == 0)
		{
			UtilFillImage(ctx, vk_obj, reinterpret_cast<void*>(*vaddr), *size, regions, static_cast<uint64_t>(vk_layout));
		}
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void update2_func(GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* params, void* obj, GpuMemoryScenario scenario,
                         const Vector<GpuMemoryObject>& objects)
{
	KYTY_PROFILER_BLOCK("TextureObject::update2_func");

	EXIT_IF(obj == nullptr);
	EXIT_IF(ctx == nullptr);
	EXIT_IF(params == nullptr);
	EXIT_IF(objects.IsEmpty());

	auto* vk_obj = static_cast<TextureVulkanImage*>(obj);

	auto width  = params[TextureObject::PARAM_WIDTH_HEIGHT] >> 32u;
	auto height = params[TextureObject::PARAM_WIDTH_HEIGHT] & 0xffffffffu;
	auto levels = params[TextureObject::PARAM_LEVELS] & 0xffffffffu;

	VkImageLayout vk_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	EXIT_NOT_IMPLEMENTED(levels >= 16);

	uint32_t mip_width  = width;
	uint32_t mip_height = height;

	Vector<ImageImageCopy> regions(levels);

	if (objects.Size() == 1 && objects.At(0).type == GpuMemoryObjectType::StorageTexture && scenario == GpuMemoryScenario::Common)
	{
		auto* src_obj = static_cast<StorageTextureVulkanImage*>(objects.At(0).obj);

		for (uint32_t i = 0; i < levels; i++)
		{
			auto mipmap_offset = UtilCalcMipmapOffset(i, width, height);

			regions[i].src_image = src_obj;
			regions[i].src_level = 0;
			regions[i].dst_level = i;
			regions[i].width     = mip_width;
			regions[i].height    = mip_height;
			regions[i].src_x     = mipmap_offset.first;
			regions[i].src_y     = mipmap_offset.second;
			regions[i].dst_x     = 0;
			regions[i].dst_y     = 0;

			if (mip_width > 1)
			{
				mip_width /= 2;
			}
			if (mip_height > 1)
			{
				mip_height /= 2;
			}
		}
	} else if (levels == objects.Size() && scenario == GpuMemoryScenario::Common)
	{
		for (uint32_t i = 0; i < levels; i++)
		{
			const auto& object = objects.At(i);

			EXIT_NOT_IMPLEMENTED(object.type != GpuMemoryObjectType::RenderTexture);

			auto* src_obj = static_cast<RenderTextureVulkanImage*>(object.obj);

			regions[i].src_image = src_obj;
			regions[i].src_level = 0;
			regions[i].dst_level = i;
			regions[i].width     = mip_width;
			regions[i].height    = mip_height;
			regions[i].src_x     = 0;
			regions[i].src_y     = 0;
			regions[i].dst_x     = 0;
			regions[i].dst_y     = 0;

			if (mip_width > 1)
			{
				mip_width /= 2;
			}
			if (mip_height > 1)
			{
				mip_height /= 2;
			}
		}
		//	} else if (objects.Size() >= 2 && objects.At(0).type == GpuMemoryObjectType::StorageBuffer &&
		//	           objects.At(1).type == GpuMemoryObjectType::StorageTexture && scenario == GpuMemoryScenario::GenerateMips)
	} else if (objects.Size() >= 3 && objects.At(0).type == GpuMemoryObjectType::StorageBuffer &&
	           objects.At(1).type == GpuMemoryObjectType::Texture && objects.At(2).type == GpuMemoryObjectType::StorageTexture &&
	           scenario == GpuMemoryScenario::GenerateMips)
	{
		for (uint32_t i = 0; i < levels; i++)
		{
			VulkanImage* src_image = nullptr;
			bool         storage   = false;

			for (const auto& o: objects)
			{
				if (o.type == GpuMemoryObjectType::StorageTexture)
				{
					auto* src_obj = static_cast<StorageTextureVulkanImage*>(o.obj);
					if (mip_width == src_obj->extent.width && mip_height == src_obj->extent.height)
					{
						src_image = src_obj;
						storage   = true;
						break;
					}
				} else if (o.type == GpuMemoryObjectType::RenderTexture)
				{
					auto* src_obj = static_cast<RenderTextureVulkanImage*>(o.obj);
					if (mip_width == src_obj->extent.width && mip_height == src_obj->extent.height)
					{
						src_image = src_obj;
						storage   = false;
						break;
					}
				}
			}

			EXIT_NOT_IMPLEMENTED(src_image == nullptr);

			if (storage)
			{
				auto mipmap_offset = UtilCalcMipmapOffset(i, width, height);

				regions[i].src_image = src_image;
				regions[i].src_level = 0;
				regions[i].dst_level = i;
				regions[i].width     = mip_width;
				regions[i].height    = mip_height;
				regions[i].src_x     = mipmap_offset.first;
				regions[i].src_y     = mipmap_offset.second;
				regions[i].dst_x     = 0;
				regions[i].dst_y     = 0;
			} else
			{
				regions[i].src_image = src_image;
				regions[i].src_level = 0;
				regions[i].dst_level = i;
				regions[i].width     = mip_width;
				regions[i].height    = mip_height;
				regions[i].src_x     = 0;
				regions[i].src_y     = 0;
				regions[i].dst_x     = 0;
				regions[i].dst_y     = 0;
			}

			if (mip_width > 1)
			{
				mip_width /= 2;
			}
			if (mip_height > 1)
			{
				mip_height /= 2;
			}
		}
	} else
	{
		KYTY_NOT_IMPLEMENTED;
	}

	if (buffer == nullptr)
	{
		UtilFillImage(ctx, regions, vk_obj, static_cast<uint64_t>(vk_layout));
	} else
	{
		UtilImageToImage(buffer, regions, vk_obj, static_cast<uint64_t>(vk_layout));
	}
}

static void* create_func(GraphicContext* ctx, const uint64_t* params, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                         VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("TextureObject::Create");

	EXIT_IF(size == nullptr || vaddr == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(ctx == nullptr);
	EXIT_IF(params == nullptr);

	auto fmt        = (params[TextureObject::PARAM_FORMAT] >> 16u) & 0xffffu;
	auto dfmt       = (params[TextureObject::PARAM_FORMAT] >> 8u) & 0xffu;
	auto nfmt       = (params[TextureObject::PARAM_FORMAT]) & 0xffu;
	auto width      = params[TextureObject::PARAM_WIDTH_HEIGHT] >> 32u;
	auto height     = params[TextureObject::PARAM_WIDTH_HEIGHT] & 0xffffffffu;
	auto base_level = params[TextureObject::PARAM_LEVELS] >> 32u;
	auto levels     = params[TextureObject::PARAM_LEVELS] & 0xffffffffu;
	auto swizzle    = params[TextureObject::PARAM_SWIZZLE];

	VkImageUsageFlags vk_usage = get_usage();

	VkComponentMapping components {};

	components.r = get_swizzle(GetDstSel(swizzle, 0));
	components.g = get_swizzle(GetDstSel(swizzle, 1));
	components.b = get_swizzle(GetDstSel(swizzle, 2));
	components.a = get_swizzle(GetDstSel(swizzle, 3));

	auto pixel_format = get_texture_format(dfmt, nfmt, fmt);

	EXIT_NOT_IMPLEMENTED(pixel_format == VK_FORMAT_UNDEFINED);
	EXIT_NOT_IMPLEMENTED(width == 0);
	EXIT_NOT_IMPLEMENTED(height == 0);

	auto* vk_obj = new TextureVulkanImage;

	VkImageCreateInfo image_info {};
	image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext         = nullptr;
	image_info.flags         = 0;
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.extent.width  = width;
	image_info.extent.height = height;
	image_info.extent.depth  = 1;
	image_info.mipLevels     = levels;
	image_info.arrayLayers   = 1;
	image_info.format        = pixel_format;
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage         = vk_usage;
	image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples       = VK_SAMPLE_COUNT_1_BIT;

	if (!CheckSwizzle(ctx, &image_info, &components))
	{
		EXIT("swizzle is not supported");
	}

	if (!CheckFormat(ctx, &image_info))
	{
		EXIT("format is not supported");
	}

	vk_obj->extent.width  = width;
	vk_obj->extent.height = height;
	vk_obj->format        = image_info.format;
	vk_obj->image         = nullptr;
	vk_obj->layout        = image_info.initialLayout;

	for (auto& view: vk_obj->image_view)
	{
		view = nullptr;
	}

	vkCreateImage(ctx->device, &image_info, nullptr, &vk_obj->image);

	EXIT_NOT_IMPLEMENTED(vk_obj->image == nullptr);

	vkGetImageMemoryRequirements(ctx->device, vk_obj->image, &mem->requirements);

	mem->property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	bool allocated = VulkanAllocate(ctx, mem);

	EXIT_NOT_IMPLEMENTED(!allocated);

	VulkanBindImageMemory(ctx, vk_obj, mem);

	vk_obj->memory = *mem;

	update_func(ctx, params, vk_obj, vaddr, size, vaddr_num);

	VkImageViewCreateInfo create_info {};
	create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.pNext                           = nullptr;
	create_info.flags                           = 0;
	create_info.image                           = vk_obj->image;
	create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format                          = vk_obj->format;
	create_info.components                      = components;
	create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.baseMipLevel   = base_level;
	create_info.subresourceRange.layerCount     = 1;
	create_info.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

	vkCreateImageView(ctx->device, &create_info, nullptr, &vk_obj->image_view[VulkanImage::VIEW_DEFAULT]);

	EXIT_NOT_IMPLEMENTED(vk_obj->image_view[VulkanImage::VIEW_DEFAULT] == nullptr);

	return vk_obj;
}

static void* create2_func(GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* params, GpuMemoryScenario scenario,
                          const Vector<GpuMemoryObject>& objects, VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("TextureObject::CreateFromObjects");

	EXIT_IF(objects.IsEmpty());
	EXIT_IF(mem == nullptr);
	EXIT_IF(ctx == nullptr);
	EXIT_IF(params == nullptr);

	auto fmt        = (params[TextureObject::PARAM_FORMAT] >> 16u) & 0xffffu;
	auto dfmt       = (params[TextureObject::PARAM_FORMAT] >> 8u) & 0xffu;
	auto nfmt       = (params[TextureObject::PARAM_FORMAT]) & 0xffu;
	auto width      = params[TextureObject::PARAM_WIDTH_HEIGHT] >> 32u;
	auto height     = params[TextureObject::PARAM_WIDTH_HEIGHT] & 0xffffffffu;
	auto base_level = params[TextureObject::PARAM_LEVELS] >> 32u;
	auto levels     = params[TextureObject::PARAM_LEVELS] & 0xffffffffu;
	auto swizzle    = params[TextureObject::PARAM_SWIZZLE];

	VkImageUsageFlags vk_usage = get_usage();

	VkComponentMapping components {};

	components.r = get_swizzle(GetDstSel(swizzle, 0));
	components.g = get_swizzle(GetDstSel(swizzle, 1));
	components.b = get_swizzle(GetDstSel(swizzle, 2));
	components.a = get_swizzle(GetDstSel(swizzle, 3));

	auto pixel_format = get_texture_format(dfmt, nfmt, fmt);

	EXIT_NOT_IMPLEMENTED(pixel_format == VK_FORMAT_UNDEFINED);
	EXIT_NOT_IMPLEMENTED(width == 0);
	EXIT_NOT_IMPLEMENTED(height == 0);

	auto* vk_obj = new TextureVulkanImage;

	VkImageCreateInfo image_info {};
	image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext         = nullptr;
	image_info.flags         = 0;
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.extent.width  = width;
	image_info.extent.height = height;
	image_info.extent.depth  = 1;
	image_info.mipLevels     = levels;
	image_info.arrayLayers   = 1;
	image_info.format        = pixel_format;
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage         = vk_usage;
	image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples       = VK_SAMPLE_COUNT_1_BIT;

	if (!CheckSwizzle(ctx, &image_info, &components))
	{
		EXIT("swizzle is not supported");
	}

	if (!CheckFormat(ctx, &image_info))
	{
		EXIT("format is not supported");
	}

	vk_obj->extent.width  = width;
	vk_obj->extent.height = height;
	vk_obj->format        = image_info.format;
	vk_obj->image         = nullptr;
	vk_obj->layout        = image_info.initialLayout;

	for (auto& view: vk_obj->image_view)
	{
		view = nullptr;
	}

	vkCreateImage(ctx->device, &image_info, nullptr, &vk_obj->image);

	EXIT_NOT_IMPLEMENTED(vk_obj->image == nullptr);

	vkGetImageMemoryRequirements(ctx->device, vk_obj->image, &mem->requirements);

	mem->property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	bool allocated = VulkanAllocate(ctx, mem);

	EXIT_NOT_IMPLEMENTED(!allocated);

	VulkanBindImageMemory(ctx, vk_obj, mem);

	vk_obj->memory = *mem;

	update2_func(ctx, buffer, params, vk_obj, scenario, objects);

	VkImageViewCreateInfo create_info {};
	create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.pNext                           = nullptr;
	create_info.flags                           = 0;
	create_info.image                           = vk_obj->image;
	create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format                          = vk_obj->format;
	create_info.components                      = components;
	create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.baseMipLevel   = base_level;
	create_info.subresourceRange.layerCount     = 1;
	create_info.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

	vkCreateImageView(ctx->device, &create_info, nullptr, &vk_obj->image_view[VulkanImage::VIEW_DEFAULT]);

	EXIT_NOT_IMPLEMENTED(vk_obj->image_view[VulkanImage::VIEW_DEFAULT] == nullptr);

	return vk_obj;
}

static void delete_func(GraphicContext* ctx, void* obj, VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("TextureObject::delete_func");

	auto* vk_obj = reinterpret_cast<TextureVulkanImage*>(obj);

	EXIT_IF(vk_obj == nullptr);
	EXIT_IF(ctx == nullptr);

	DeleteDescriptor(vk_obj);

	vkDestroyImageView(ctx->device, vk_obj->image_view[VulkanImage::VIEW_DEFAULT], nullptr);

	vkDestroyImage(ctx->device, vk_obj->image, nullptr);

	VulkanFree(ctx, mem);

	delete vk_obj;
}

bool TextureObject::Equal(const uint64_t* other) const
{
	return (params[PARAM_FORMAT] == other[PARAM_FORMAT] && params[PARAM_PITCH] == other[PARAM_PITCH] &&
	        params[PARAM_WIDTH_HEIGHT] == other[PARAM_WIDTH_HEIGHT] && params[PARAM_LEVELS] == other[PARAM_LEVELS] &&
	        params[PARAM_TILE] == other[PARAM_TILE] && params[PARAM_NEO] == other[PARAM_NEO] &&
	        params[PARAM_SWIZZLE] == other[PARAM_SWIZZLE]);
}

GpuObject::create_func_t TextureObject::GetCreateFunc() const
{
	return create_func;
}

GpuObject::create_from_objects_func_t TextureObject::GetCreateFromObjectsFunc() const
{
	return create2_func;
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
