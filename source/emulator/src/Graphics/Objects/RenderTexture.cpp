#include "Emulator/Graphics/Objects/RenderTexture.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/Utils.h"
#include "Emulator/Profiler.h"

// IWYU pragma: no_forward_declare VkImageView_T

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

static bool buffer_is_tiled(uint64_t vaddr, uint64_t size)
{
	if ((size & 0x7u) == 0)
	{
		const auto* ptr     = reinterpret_cast<const uint64_t*>(vaddr);
		const auto* ptr_end = reinterpret_cast<const uint64_t*>(vaddr + size / 8);
		for (uint64_t element = *ptr; ptr < ptr_end; ptr++)
		{
			if (element != *ptr)
			{
				return true;
			}
		}
		return false;
	}
	return true;
}

static void update_func(GraphicContext* ctx, const uint64_t* params, void* obj, const uint64_t* vaddr, const uint64_t* size, int vaddr_num)
{
	KYTY_PROFILER_BLOCK("RenderTextureObject::update_func");

	EXIT_IF(obj == nullptr);
	EXIT_IF(ctx == nullptr);
	EXIT_IF(params == nullptr);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num != 1);

	auto* vk_obj = static_cast<RenderTextureVulkanImage*>(obj);

	bool tiled = (params[RenderTextureObject::PARAM_TILED] != 0);
	// bool neo    = (params[RenderTextureObject::PARAM_NEO] != 0);
	auto pitch = params[RenderTextureObject::PARAM_PITCH];
	auto width = params[RenderTextureObject::PARAM_WIDTH];
	// auto height = params[RenderTextureObject::PARAM_HEIGHT];

	vk_obj->layout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (tiled && buffer_is_tiled(*vaddr, *size))
	{
		EXIT_NOT_IMPLEMENTED(width != pitch);
		auto* temp_buf = new uint8_t[*size];
		KYTY_NOT_IMPLEMENTED;
		// TODO()
		// TileConvertTiledToLinear(temp_buf, reinterpret_cast<void*>(*vaddr), TileMode::VideoOutTiled, width, height, neo);
		// UtilFillImage(ctx, vk_obj, temp_buf, *size, pitch);
		delete[] temp_buf;
	} else
	{
		UtilFillImage(ctx, vk_obj, reinterpret_cast<void*>(*vaddr), *size, pitch,
		              static_cast<uint64_t>(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	}
}

static void update2_func(GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* params, void* obj, GpuMemoryScenario scenario,
                         const Vector<GpuMemoryObject>& objects)
{
	KYTY_PROFILER_BLOCK("RenderTextureObject::update_func");

	EXIT_IF(obj == nullptr);
	EXIT_IF(ctx == nullptr);
	EXIT_IF(params == nullptr);
	EXIT_IF(objects.IsEmpty());

	auto* vk_obj = static_cast<RenderTextureVulkanImage*>(obj);

	// bool tiled = (params[RenderTextureObject::PARAM_TILED] != 0);
	// bool neo    = (params[RenderTextureObject::PARAM_NEO] != 0);
	// auto pitch  = params[RenderTextureObject::PARAM_PITCH];
	auto width  = params[RenderTextureObject::PARAM_WIDTH];
	auto height = params[RenderTextureObject::PARAM_HEIGHT];

	vk_obj->layout = VK_IMAGE_LAYOUT_UNDEFINED;

	//	if (objects.Size() == 2 && objects.At(0).type == GpuMemoryObjectType::StorageBuffer &&
	//	    objects.At(1).type == GpuMemoryObjectType::StorageTexture && scenario == GpuMemoryScenario::GenerateMips)
	//	{
	//		auto* src_obj = static_cast<StorageTextureVulkanImage*>(objects.At(1).obj);
	if (objects.Size() == 3 && objects.At(0).type == GpuMemoryObjectType::StorageBuffer &&
	    objects.At(1).type == GpuMemoryObjectType::Texture && objects.At(2).type == GpuMemoryObjectType::StorageTexture &&
	    scenario == GpuMemoryScenario::GenerateMips)
	{
		auto* src_obj = static_cast<StorageTextureVulkanImage*>(objects.At(2).obj);

		uint32_t mip_width  = src_obj->extent.width;
		uint32_t mip_height = src_obj->extent.height;

		Vector<ImageImageCopy> regions(1);

		bool updated = false;

		for (uint32_t i = 0; i < 16 && !updated; i++)
		{
			if (mip_width == width && mip_height == height)
			{
				auto mipmap_offset = UtilCalcMipmapOffset(i, width, height);

				regions[0].src_image = src_obj;
				regions[0].src_level = 0;
				regions[0].dst_level = 0;
				regions[0].width     = mip_width;
				regions[0].height    = mip_height;
				regions[0].src_x     = mipmap_offset.first;
				regions[0].src_y     = mipmap_offset.second;
				regions[0].dst_x     = 0;
				regions[0].dst_y     = 0;

				if (buffer == nullptr)
				{
					UtilFillImage(ctx, regions, vk_obj, static_cast<uint64_t>(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
				} else
				{
					UtilImageToImage(buffer, regions, vk_obj, static_cast<uint64_t>(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
				}

				updated = true;
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

		EXIT_NOT_IMPLEMENTED(!updated);
	} else
	{
		KYTY_NOT_IMPLEMENTED;
	}
}

static void* create_func(GraphicContext* ctx, const uint64_t* params, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                         VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("RenderTextureObject::Create");

	EXIT_IF(vaddr_num != 1 || size == nullptr || vaddr == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(ctx == nullptr);

	auto pixel_format = params[RenderTextureObject::PARAM_FORMAT];
	auto width        = params[RenderTextureObject::PARAM_WIDTH];
	auto height       = params[RenderTextureObject::PARAM_HEIGHT];

	VkFormat vk_format = VK_FORMAT_UNDEFINED;

	switch (pixel_format)
	{
		case static_cast<uint64_t>(RenderTextureFormat::R8Unorm): vk_format = VK_FORMAT_R8_UNORM; break;
		case static_cast<uint64_t>(RenderTextureFormat::R8G8B8A8Unorm): vk_format = VK_FORMAT_R8G8B8A8_UNORM; break;
		case static_cast<uint64_t>(RenderTextureFormat::R8G8B8A8Srgb): vk_format = VK_FORMAT_R8G8B8A8_SRGB; break;
		case static_cast<uint64_t>(RenderTextureFormat::B8G8R8A8Unorm): vk_format = VK_FORMAT_B8G8R8A8_UNORM; break;
		case static_cast<uint64_t>(RenderTextureFormat::B8G8R8A8Srgb): vk_format = VK_FORMAT_B8G8R8A8_SRGB; break;
		default: EXIT("unknown format: %" PRIu64 "\n", pixel_format);
	}

	EXIT_NOT_IMPLEMENTED(width == 0);
	EXIT_NOT_IMPLEMENTED(height == 0);

	auto* vk_obj = new RenderTextureVulkanImage;

	vk_obj->extent.width  = width;
	vk_obj->extent.height = height;
	vk_obj->format        = vk_format;
	vk_obj->image         = nullptr;

	for (auto& view: vk_obj->image_view)
	{
		view = nullptr;
	}

	VkImageCreateInfo image_info {};
	image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext         = nullptr;
	image_info.flags         = 0;
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.extent.width  = vk_obj->extent.width;
	image_info.extent.height = vk_obj->extent.height;
	image_info.extent.depth  = 1;
	image_info.mipLevels     = 1;
	image_info.arrayLayers   = 1;
	image_info.format        = vk_obj->format;
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage         = static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) |
	                   static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT) |
	                   static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_TRANSFER_DST_BIT) |
	                   static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_SAMPLED_BIT);
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples     = VK_SAMPLE_COUNT_1_BIT;

	vkCreateImage(ctx->device, &image_info, nullptr, &vk_obj->image);

	EXIT_NOT_IMPLEMENTED(vk_obj->image == nullptr);

	vkGetImageMemoryRequirements(ctx->device, vk_obj->image, &mem->requirements);

	mem->property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	bool allocated = VulkanAllocate(ctx, mem);

	EXIT_NOT_IMPLEMENTED(!allocated);

	VulkanBindImageMemory(ctx, vk_obj, mem);

	vk_obj->memory = *mem;

	printf("RenderTextureObject::Create()\n");
	printf("\t mem->requirements.size = %" PRIu64 "\n", mem->requirements.size);
	printf("\t width                  = %" PRIu64 "\n", width);
	printf("\t height                 = %" PRIu64 "\n", height);
	printf("\t size                   = %" PRIu64 "\n", *size);

	// EXIT_NOT_IMPLEMENTED(mem->requirements.size > *size);

	update_func(ctx, params, vk_obj, vaddr, size, vaddr_num);

	VkImageViewCreateInfo create_info {};
	create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.pNext                           = nullptr;
	create_info.flags                           = 0;
	create_info.image                           = vk_obj->image;
	create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format                          = vk_obj->format;
	create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.baseMipLevel   = 0;
	create_info.subresourceRange.layerCount     = 1;
	create_info.subresourceRange.levelCount     = 1;

	vkCreateImageView(ctx->device, &create_info, nullptr, &vk_obj->image_view[VulkanImage::VIEW_DEFAULT]);

	VkImageViewCreateInfo create_info2 {};
	create_info2.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info2.pNext                           = nullptr;
	create_info2.flags                           = 0;
	create_info2.image                           = vk_obj->image;
	create_info2.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	create_info2.format                          = vk_obj->format;
	create_info2.components.r                    = VK_COMPONENT_SWIZZLE_B;
	create_info2.components.g                    = VK_COMPONENT_SWIZZLE_G;
	create_info2.components.b                    = VK_COMPONENT_SWIZZLE_R;
	create_info2.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info2.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info2.subresourceRange.baseArrayLayer = 0;
	create_info2.subresourceRange.baseMipLevel   = 0;
	create_info2.subresourceRange.layerCount     = 1;
	create_info2.subresourceRange.levelCount     = 1;

	vkCreateImageView(ctx->device, &create_info2, nullptr, &vk_obj->image_view[VulkanImage::VIEW_BGRA]);

	EXIT_NOT_IMPLEMENTED(vk_obj->image_view[VulkanImage::VIEW_DEFAULT] == nullptr);
	EXIT_NOT_IMPLEMENTED(vk_obj->image_view[VulkanImage::VIEW_BGRA] == nullptr);

	return vk_obj;
}

static void* create2_func(GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* params, GpuMemoryScenario scenario,
                          const Vector<GpuMemoryObject>& objects, VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("RenderTextureObject::CreateFromObjects");

	EXIT_IF(objects.IsEmpty());
	EXIT_IF(mem == nullptr);
	EXIT_IF(ctx == nullptr);

	auto pixel_format = params[RenderTextureObject::PARAM_FORMAT];
	auto width        = params[RenderTextureObject::PARAM_WIDTH];
	auto height       = params[RenderTextureObject::PARAM_HEIGHT];

	VkFormat vk_format = VK_FORMAT_UNDEFINED;

	switch (pixel_format)
	{
		case static_cast<uint64_t>(RenderTextureFormat::R8Unorm): vk_format = VK_FORMAT_R8_UNORM; break;
		case static_cast<uint64_t>(RenderTextureFormat::R8G8B8A8Unorm): vk_format = VK_FORMAT_R8G8B8A8_UNORM; break;
		case static_cast<uint64_t>(RenderTextureFormat::R8G8B8A8Srgb): vk_format = VK_FORMAT_R8G8B8A8_SRGB; break;
		case static_cast<uint64_t>(RenderTextureFormat::B8G8R8A8Unorm): vk_format = VK_FORMAT_B8G8R8A8_UNORM; break;
		case static_cast<uint64_t>(RenderTextureFormat::B8G8R8A8Srgb): vk_format = VK_FORMAT_B8G8R8A8_SRGB; break;
		default: EXIT("unknown format: %" PRIu64 "\n", pixel_format);
	}

	EXIT_NOT_IMPLEMENTED(width == 0);
	EXIT_NOT_IMPLEMENTED(height == 0);

	auto* vk_obj = new RenderTextureVulkanImage;

	vk_obj->extent.width  = width;
	vk_obj->extent.height = height;
	vk_obj->format        = vk_format;
	vk_obj->image         = nullptr;

	for (auto& view: vk_obj->image_view)
	{
		view = nullptr;
	}

	VkImageCreateInfo image_info {};
	image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext         = nullptr;
	image_info.flags         = 0;
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.extent.width  = vk_obj->extent.width;
	image_info.extent.height = vk_obj->extent.height;
	image_info.extent.depth  = 1;
	image_info.mipLevels     = 1;
	image_info.arrayLayers   = 1;
	image_info.format        = vk_obj->format;
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage         = static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) |
	                   static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT) |
	                   static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_TRANSFER_DST_BIT) |
	                   static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_SAMPLED_BIT);
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples     = VK_SAMPLE_COUNT_1_BIT;

	vkCreateImage(ctx->device, &image_info, nullptr, &vk_obj->image);

	EXIT_NOT_IMPLEMENTED(vk_obj->image == nullptr);

	vkGetImageMemoryRequirements(ctx->device, vk_obj->image, &mem->requirements);

	mem->property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	bool allocated = VulkanAllocate(ctx, mem);

	EXIT_NOT_IMPLEMENTED(!allocated);

	VulkanBindImageMemory(ctx, vk_obj, mem);

	vk_obj->memory = *mem;

	printf("RenderTextureObject::CreateFromObjects()\n");
	printf("\t mem->requirements.size = %" PRIu64 "\n", mem->requirements.size);
	printf("\t width                  = %" PRIu64 "\n", width);
	printf("\t height                 = %" PRIu64 "\n", height);
	// printf("\t size                   = %" PRIu64 "\n", *size);

	// EXIT_NOT_IMPLEMENTED(mem->requirements.size > *size);

	update2_func(ctx, buffer, params, vk_obj, scenario, objects);

	VkImageViewCreateInfo create_info {};
	create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.pNext                           = nullptr;
	create_info.flags                           = 0;
	create_info.image                           = vk_obj->image;
	create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format                          = vk_obj->format;
	create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.baseMipLevel   = 0;
	create_info.subresourceRange.layerCount     = 1;
	create_info.subresourceRange.levelCount     = 1;

	vkCreateImageView(ctx->device, &create_info, nullptr, &vk_obj->image_view[VulkanImage::VIEW_DEFAULT]);

	VkImageViewCreateInfo create_info2 {};
	create_info2.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info2.pNext                           = nullptr;
	create_info2.flags                           = 0;
	create_info2.image                           = vk_obj->image;
	create_info2.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	create_info2.format                          = vk_obj->format;
	create_info2.components.r                    = VK_COMPONENT_SWIZZLE_B;
	create_info2.components.g                    = VK_COMPONENT_SWIZZLE_G;
	create_info2.components.b                    = VK_COMPONENT_SWIZZLE_R;
	create_info2.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info2.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info2.subresourceRange.baseArrayLayer = 0;
	create_info2.subresourceRange.baseMipLevel   = 0;
	create_info2.subresourceRange.layerCount     = 1;
	create_info2.subresourceRange.levelCount     = 1;

	vkCreateImageView(ctx->device, &create_info2, nullptr, &vk_obj->image_view[VulkanImage::VIEW_BGRA]);

	EXIT_NOT_IMPLEMENTED(vk_obj->image_view[VulkanImage::VIEW_DEFAULT] == nullptr);
	EXIT_NOT_IMPLEMENTED(vk_obj->image_view[VulkanImage::VIEW_BGRA] == nullptr);

	return vk_obj;
}

static void delete_func(GraphicContext* ctx, void* obj, VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("RenderTextureObject::delete_func");

	auto* vk_obj = reinterpret_cast<RenderTextureVulkanImage*>(obj);

	EXIT_IF(vk_obj == nullptr);
	EXIT_IF(ctx == nullptr);

	DeleteDescriptor(vk_obj);

	DeleteFramebuffer(vk_obj);

	vkDestroyImageView(ctx->device, vk_obj->image_view[VulkanImage::VIEW_DEFAULT], nullptr);

	vkDestroyImage(ctx->device, vk_obj->image, nullptr);

	VulkanFree(ctx, mem);

	delete vk_obj;
}

static void write_back(GraphicContext* ctx, const uint64_t* params, void* obj, const uint64_t* vaddr, const uint64_t* size, int vaddr_num)
{
	KYTY_PROFILER_BLOCK("RenderTextureObject::write_back");

	EXIT_IF(ctx == nullptr);
	EXIT_IF(obj == nullptr);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num != 1);

	bool tiled = (params[RenderTextureObject::PARAM_TILED] != 0);
	auto pitch = params[RenderTextureObject::PARAM_PITCH];
	auto width = params[RenderTextureObject::PARAM_WIDTH];

	EXIT_IF(!(params[RenderTextureObject::PARAM_WRITE_BACK] != 0));

	EXIT_NOT_IMPLEMENTED(tiled);
	EXIT_NOT_IMPLEMENTED(width != pitch);

	auto* vk_obj = reinterpret_cast<RenderTextureVulkanImage*>(obj);

	UtilFillBuffer(ctx, reinterpret_cast<void*>(*vaddr), *size, pitch, vk_obj,
	               static_cast<uint64_t>(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
}

bool RenderTextureObject::Equal(const uint64_t* other) const
{
	return (params[PARAM_FORMAT] == other[PARAM_FORMAT] && params[PARAM_WIDTH] == other[PARAM_WIDTH] &&
	        params[PARAM_HEIGHT] == other[PARAM_HEIGHT] && params[PARAM_TILED] == other[PARAM_TILED] &&
	        params[PARAM_PITCH] == other[PARAM_PITCH] && params[PARAM_PITCH] == other[PARAM_PITCH] &&
	        params[PARAM_WRITE_BACK] == other[PARAM_WRITE_BACK]);
}

GpuObject::create_func_t RenderTextureObject::GetCreateFunc() const
{
	return create_func;
}

GpuObject::create_from_objects_func_t RenderTextureObject::GetCreateFromObjectsFunc() const
{
	return create2_func;
}

GpuObject::delete_func_t RenderTextureObject::GetDeleteFunc() const
{
	return delete_func;
}

GpuObject::update_func_t RenderTextureObject::GetUpdateFunc() const
{
	return update_func;
}

GpuObject::write_back_func_t RenderTextureObject::GetWriteBackFunc() const
{
	bool wb = (params[RenderTextureObject::PARAM_WRITE_BACK] != 0);
	return (wb ? write_back : nullptr);
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
