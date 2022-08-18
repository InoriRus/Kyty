#include "Emulator/Graphics/GraphicsRender.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/Hash.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRun.h"
#include "Emulator/Graphics/HardwareContext.h"
#include "Emulator/Graphics/Objects/DepthStencilBuffer.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"
#include "Emulator/Graphics/Objects/IndexBuffer.h"
#include "Emulator/Graphics/Objects/Label.h"
#include "Emulator/Graphics/Objects/RenderTexture.h"
#include "Emulator/Graphics/Objects/StorageBuffer.h"
#include "Emulator/Graphics/Objects/StorageTexture.h"
#include "Emulator/Graphics/Objects/Texture.h"
#include "Emulator/Graphics/Objects/VertexBuffer.h"
#include "Emulator/Graphics/Shader.h"
#include "Emulator/Graphics/Tile.h"
#include "Emulator/Graphics/Utils.h"
#include "Emulator/Graphics/VideoOut.h"
#include "Emulator/Graphics/Window.h"
#include "Emulator/Kernel/EventQueue.h"
#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Profiler.h"

#include <atomic>

// IWYU pragma: no_forward_declare VkImageView_T

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

constexpr int GRAPHICS_EVENT_EOP = 0x40;

struct Label;
struct RenderDepthInfo;

struct VulkanDescriptor
{
	VkDescriptorSet descriptor_set = nullptr;
};

// Pack structs to guarantee the uniquess of object representation
#pragma pack(push, 1)

struct PipelineStencilStaticState
{
	VkStencilOp failOp      = VK_STENCIL_OP_KEEP;
	VkStencilOp passOp      = VK_STENCIL_OP_KEEP;
	VkStencilOp depthFailOp = VK_STENCIL_OP_KEEP;
	VkCompareOp compareOp   = VK_COMPARE_OP_NEVER;
};

struct PipelineStencilDynamicState
{
	uint32_t compareMask = 0;
	uint32_t writeMask   = 0;
	uint32_t reference   = 0;
};

struct PipelineStaticParameters
{
	float                      viewport_scale[3]        = {};
	float                      viewport_offset[3]       = {};
	int                        scissor_ltrb[4]          = {0};
	VkPrimitiveTopology        topology                 = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	bool                       with_depth               = false;
	bool                       depth_test_enable        = false;
	bool                       depth_write_enable       = false;
	VkCompareOp                depth_compare_op         = VK_COMPARE_OP_NEVER;
	bool                       depth_bounds_test_enable = false;
	float                      depth_min_bounds         = 0.0f;
	float                      depth_max_bounds         = 0.0f;
	bool                       stencil_test_enable      = false;
	PipelineStencilStaticState stencil_front;
	PipelineStencilStaticState stencil_back;
	uint32_t                   color_mask           = 0;
	bool                       cull_front           = false;
	bool                       cull_back            = false;
	bool                       face                 = false;
	uint8_t                    color_srcblend       = 0;
	uint8_t                    color_comb_fcn       = 0;
	uint8_t                    color_destblend      = 0;
	uint8_t                    alpha_srcblend       = 0;
	uint8_t                    alpha_comb_fcn       = 0;
	uint8_t                    alpha_destblend      = 0;
	bool                       separate_alpha_blend = false;
	bool                       blend_enable         = false;
	float                      blend_color_red      = 0.0f;
	float                      blend_color_green    = 0.0f;
	float                      blend_color_blue     = 0.0f;
	float                      blend_color_alpha    = 0.0f;

	bool operator==(const PipelineStaticParameters& other) const;
};

struct PipelineDynamicParameters
{
	bool vk_dynamic_state_line_width             = false;
	bool vk_dynamic_state_stencil_compare_mask   = false;
	bool vk_dynamic_state_stencil_write_mask     = false;
	bool vk_dynamic_state_stencil_reference      = false;
	bool vk_dynamic_state_color_write_enable_ext = false;

	float line_width         = 1.0f;
	bool  color_write_enable = true;

	PipelineStencilDynamicState stencil_front;
	PipelineStencilDynamicState stencil_back;

	bool operator==(const PipelineDynamicParameters& other) const;
};
#pragma pack(pop)

struct VulkanPipeline
{
	VkPipelineLayout                pipeline_layout = nullptr;
	VkPipeline                      pipeline        = nullptr;
	const PipelineStaticParameters* static_params   = nullptr;
	PipelineDynamicParameters*      dynamic_params  = nullptr;
};

class PipelineCache
{
public:
	PipelineCache() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~PipelineCache() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(PipelineCache);

	VulkanPipeline* CreatePipeline(VulkanFramebuffer* framebuffer, RenderColorInfo* color, RenderDepthInfo* depth,
	                               const ShaderVertexInputInfo* vs_input_info, HW::Context* ctx, HW::Shader* sh_ctx,
	                               const ShaderPixelInputInfo* ps_input_info, VkPrimitiveTopology topology);
	VulkanPipeline* CreatePipeline(const ShaderComputeInputInfo* input_info, const HW::ComputeShaderInfo* cs_regs,
	                               const HW::ShaderRegisters* sh_regs);
	void            DeletePipeline(VulkanPipeline* pipeline);
	void            DeletePipelines(VulkanFramebuffer* framebuffer);
	void            DeleteAllPipelines();

private:
	static constexpr uint32_t MAX_PIPELINES = 128;

	struct Pipeline
	{
		uint64_t                   render_pass_id = 0;
		ShaderId                   vs_shader_id;
		ShaderId                   ps_shader_id;
		ShaderId                   cs_shader_id;
		VulkanPipeline*            pipeline       = nullptr;
		PipelineStaticParameters*  static_params  = nullptr;
		PipelineDynamicParameters* dynamic_params = nullptr;
	};

	[[nodiscard]] VulkanPipeline* Find(const Pipeline& p) const;

	void DeletePipelineInternal(uint32_t id);

	void DumpToFile(Core::File* f, const Pipeline& p);
	void DumpPipeline(const char* action, uint32_t id);

	Vector<Pipeline> m_pipelines;
	Core::Mutex      m_mutex;
};

struct VulkanDescriptorSet
{
	VkDescriptorSet       set     = nullptr;
	VkDescriptorSetLayout layout  = nullptr;
	int                   pool_id = -1;
};

class DescriptorCache
{
public:
	enum class Stage
	{
		Unknown,
		Vertex,
		Pixel,
		Compute
	};

	static constexpr int BUFFERS_MAX          = ShaderStorageResources::BUFFERS_MAX;
	static constexpr int TEXTURES_SAMPLED_MAX = ShaderTextureResources::RES_MAX;
	static constexpr int TEXTURES_STORAGE_MAX = ShaderTextureResources::RES_MAX;
	static constexpr int SAMPLERS_MAX         = ShaderSamplerResources::RES_MAX;
	static constexpr int PUSH_CONSTANTS_MAX   = 16 * 4;
	static constexpr int GDS_BUFFER_MAX       = 1;

	DescriptorCache() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~DescriptorCache() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(DescriptorCache);

	VkDescriptorSetLayout GetDescriptorSetLayout(Stage stage, const ShaderBindResources& bind);

	VulkanDescriptorSet* Allocate(Stage stage, int storage_buffers_num, int textures2d_sampled_num, int textures2d_storage_num,
	                              int samplers_num, int gds_buffers_num);
	void                 Free(VulkanDescriptorSet* set);

	VulkanDescriptorSet* GetDescriptor(Stage stage, VulkanBuffer** storage_buffers, VulkanImage** textures2d_sampled,
	                                   const int* textures2d_sampled_view, VulkanImage** textures2d_storage, uint64_t* samplers,
	                                   VulkanBuffer** gds_buffers, const ShaderBindResources& bind);
	void                 FreeDescriptor(VulkanBuffer* buffer);
	void                 FreeDescriptor(VulkanImage* image);

private:
	struct Set
	{
		VulkanDescriptorSet* set                                         = nullptr;
		int                  next_free_set                               = -1;
		uint32_t             hash                                        = 0;
		Stage                stage                                       = Stage::Unknown;
		int                  storage_buffers_num                         = 0;
		uint64_t             storage_buffers_id[BUFFERS_MAX]             = {};
		int                  textures2d_sampled_num                      = 0;
		uint64_t             textures2d_sampled_id[TEXTURES_SAMPLED_MAX] = {};
		int                  textures2d_storage_num                      = 0;
		uint64_t             textures2d_storage_id[TEXTURES_STORAGE_MAX] = {};
		int                  samplers_num                                = 0;
		uint64_t             samplers_id[SAMPLERS_MAX]                   = {};
		int                  gds_buffers_num                             = 0;
		uint64_t             gds_buffers_id[GDS_BUFFER_MAX]              = {};
	};

	struct Pool
	{
		VkDescriptorPool pool           = nullptr;
		int              next_free_pool = -1;
		bool             free           = true;
	};

	void Init();
	void CreatePool();

	static uint32_t CalcHash(const Set& s);

	VulkanDescriptorSet* FindSet(const Set& s);

	Core::Mutex  m_mutex;
	Vector<Pool> m_pools;
	Vector<Set>  m_sets;
	int          m_first_free_set  = -1;
	int          m_first_free_pool = -1;

	Core::Hashmap<uint32_t, Vector<int>> m_sets_map;

	VkDescriptorSetLayout m_descriptor_set_layout_vertex[BUFFERS_MAX + 1][TEXTURES_SAMPLED_MAX + 1][TEXTURES_STORAGE_MAX + 1]
	                                                    [SAMPLERS_MAX + 1][GDS_BUFFER_MAX + 1] = {};
	VkDescriptorSetLayout m_descriptor_set_layout_pixel[BUFFERS_MAX + 1][TEXTURES_SAMPLED_MAX + 1][TEXTURES_STORAGE_MAX + 1]
	                                                   [SAMPLERS_MAX + 1][GDS_BUFFER_MAX + 1] = {};
	VkDescriptorSetLayout m_descriptor_set_layout_compute[BUFFERS_MAX + 1][TEXTURES_SAMPLED_MAX + 1][TEXTURES_STORAGE_MAX + 1]
	                                                     [SAMPLERS_MAX + 1][GDS_BUFFER_MAX + 1] = {};
	bool m_initialized                                                                          = false;
};

class SamplerCache
{
public:
	SamplerCache() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~SamplerCache() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(SamplerCache);

	VkSampler GetSampler(uint64_t id);
	uint64_t  GetSamplerId(const ShaderSamplerResource& r);

private:
	struct Sampler
	{
		ShaderSamplerResource r;
		VkSampler             vk = nullptr;
	};

	Core::Mutex     m_mutex;
	Vector<Sampler> m_samplers;
};

struct VulkanFramebuffer
{
	VkRenderPass  render_pass    = nullptr;
	uint64_t      render_pass_id = 0;
	VkFramebuffer framebuffer    = nullptr;
};

class FramebufferCache
{
public:
	FramebufferCache() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~FramebufferCache() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(FramebufferCache);

	VulkanFramebuffer* CreateFramebuffer(RenderColorInfo* color, RenderDepthInfo* depth);
	void               FreeFramebufferByColor(VulkanImage* image);
	void               FreeFramebufferByDepth(DepthStencilVulkanImage* image);

private:
	VideoOutVulkanImage* CreateDummyBuffer(VkFormat format, uint32_t width, uint32_t height);

	struct Framebuffer
	{
		VulkanFramebuffer* framebuffer          = nullptr;
		uint64_t           image_id             = 0;
		uint64_t           depth_id             = 0;
		bool               depth_clear_enable   = false;
		bool               stencil_clear_enable = false;
	};

	Core::Mutex                  m_mutex;
	Vector<Framebuffer>          m_framebuffers;
	Vector<VideoOutVulkanImage*> m_dummy_buffers;
};

class GdsBuffer
{
public:
	GdsBuffer() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~GdsBuffer() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(GdsBuffer);

	void Clear(GraphicContext* ctx, uint64_t dw_offset, uint32_t dw_num, uint32_t clear_value);
	void Read(GraphicContext* ctx, uint32_t* dst, uint32_t dw_offset, uint32_t dw_size);

	VulkanBuffer* GetBuffer(GraphicContext* ctx);

private:
	static constexpr uint64_t DW_SIZE = 0x3000;

	void Init(GraphicContext* ctx);

	Core::Mutex   m_mutex;
	VulkanBuffer* m_buffer = nullptr;
};

class RenderContext
{
public:
	RenderContext()
	    : m_pipeline_cache(new PipelineCache), m_descriptor_cache(new DescriptorCache), m_framebuffer_cache(new FramebufferCache),
	      m_sampler_cache(new SamplerCache), m_gds_buffer(new GdsBuffer)
	{
		EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
	}
	virtual ~RenderContext() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(RenderContext);

	void            SetGraphicCtx(GraphicContext* ctx) { m_graphic_ctx = ctx; }
	GraphicContext* GetGraphicCtx() { return m_graphic_ctx; }

	Core::Mutex&      GetMutex() { return m_mutex; }
	PipelineCache*    GetPipelineCache() { return m_pipeline_cache; }
	DescriptorCache*  GetDescriptorCache() { return m_descriptor_cache; }
	FramebufferCache* GetFramebufferCache() { return m_framebuffer_cache; }
	SamplerCache*     GetSamplerCache() { return m_sampler_cache; }
	GdsBuffer*        GetGdsBuffer() { return m_gds_buffer; }

	void AddEopEq(LibKernel::EventQueue::KernelEqueue eq);
	void DeleteEopEq(LibKernel::EventQueue::KernelEqueue eq);
	void TriggerEopEvent();

private:
	Core::Mutex       m_mutex;
	PipelineCache*    m_pipeline_cache    = nullptr;
	DescriptorCache*  m_descriptor_cache  = nullptr;
	FramebufferCache* m_framebuffer_cache = nullptr;
	SamplerCache*     m_sampler_cache     = nullptr;
	GraphicContext*   m_graphic_ctx       = nullptr;
	GdsBuffer*        m_gds_buffer        = nullptr;

	Core::Mutex                                 m_eop_mutex;
	Vector<LibKernel::EventQueue::KernelEqueue> m_eop_eqs;
};

struct RenderDepthInfo
{
	VkFormat                    format                   = VK_FORMAT_UNDEFINED;
	uint32_t                    width                    = 0;
	uint32_t                    height                   = 0;
	bool                        htile                    = false;
	bool                        neo                      = false;
	uint64_t                    depth_buffer_size        = 0;
	uint64_t                    depth_buffer_vaddr       = 0;
	uint64_t                    depth_tile_swizzle       = 0;
	uint64_t                    stencil_buffer_size      = 0;
	uint64_t                    stencil_buffer_vaddr     = 0;
	uint64_t                    stencil_tile_swizzle     = 0;
	uint64_t                    htile_buffer_size        = 0;
	uint64_t                    htile_buffer_vaddr       = 0;
	uint64_t                    htile_tile_swizzle       = 0;
	bool                        depth_clear_enable       = false;
	float                       depth_clear_value        = 0.0f;
	bool                        depth_test_enable        = false;
	bool                        depth_write_enable       = false;
	VkCompareOp                 depth_compare_op         = VK_COMPARE_OP_NEVER;
	bool                        depth_bounds_test_enable = false;
	float                       depth_min_bounds         = 0.0f;
	float                       depth_max_bounds         = 0.0f;
	bool                        stencil_clear_enable     = false;
	uint8_t                     stencil_clear_value      = 0;
	bool                        stencil_test_enable      = false;
	PipelineStencilStaticState  stencil_static_front;
	PipelineStencilStaticState  stencil_static_back;
	PipelineStencilDynamicState stencil_dynamic_front;
	PipelineStencilDynamicState stencil_dynamic_back;
	DepthStencilVulkanImage*    vulkan_buffer = nullptr;
	uint64_t                    vaddr[3]      = {};
	uint64_t                    size[3]       = {};
	int                         vaddr_num     = 0;
};

enum class RenderColorType
{
	NoColorOutput,
	DisplayBuffer,
	// OffscreenBuffer,
	RenderTexture,
};

struct RenderColorInfo
{
	RenderColorType type          = RenderColorType::NoColorOutput;
	VulkanImage*    vulkan_buffer = nullptr;
	uint64_t        base_addr     = 0;
	uint64_t        buffer_size   = 0;
};

class CommandPool
{
public:
	CommandPool() = default;
	~CommandPool() // NOLINT
	{
		// TODO(): check if destructor is called from std::_Exit()
		// DeleteAll();
	}

	KYTY_CLASS_NO_COPY(CommandPool);

	VulkanCommandPool* GetPool(int id)
	{
		if (m_pool[id] == nullptr)
		{
			Create(id);
		}
		return m_pool[id];
	}

private:
	void Create(int id);
	void DeleteAll();

	VulkanCommandPool* m_pool[GraphicContext::QUEUES_NUM] = {};
};

static RenderContext*           g_render_ctx = nullptr;
static thread_local CommandPool g_command_pool;

static void uc_print(const char* func, const HW::UserConfig& uc)
{
	printf("%s\n", func);

	const auto& ge_cntl = uc.GetGeControl();
	const auto& user_en = uc.GetGeUserVgprEn();

	printf("\t GetPrimType()         = 0x%08" PRIx32 "\n", uc.GetPrimType());
	printf("\t primitive_group_size  = 0x%04" PRIx16 "\n", ge_cntl.primitive_group_size);
	printf("\t vertex_group_size     = 0x%04" PRIx16 "\n", ge_cntl.vertex_group_size);
	printf("\t en_user_vgpr1         = %s\n", user_en.vgpr1 ? "true" : "false");
	printf("\t en_user_vgpr2         = %s\n", user_en.vgpr2 ? "true" : "false");
	printf("\t en_user_vgpr3         = %s\n", user_en.vgpr3 ? "true" : "false");
}

static void uc_check(const HW::UserConfig& uc)
{
	const auto& ge_cntl = uc.GetGeControl();
	const auto& user_en = uc.GetGeUserVgprEn();

	EXIT_NOT_IMPLEMENTED(ge_cntl.primitive_group_size != 0x0000 && ge_cntl.primitive_group_size != 0x0040);
	EXIT_NOT_IMPLEMENTED(ge_cntl.vertex_group_size != 0x0000 && ge_cntl.vertex_group_size != 0x0040);
	EXIT_NOT_IMPLEMENTED(user_en.vgpr1 != false);
	EXIT_NOT_IMPLEMENTED(user_en.vgpr2 != false);
	EXIT_NOT_IMPLEMENTED(user_en.vgpr3 != false);
}

static void sh_print(const char* func, const HW::Shader& /*uc*/)
{
	printf("%s\n", func);
}

static void sh_check(const HW::Shader& /*uc*/) {}

static Core::StringList rt_print(const char* func, const HW::RenderTarget& rt)
{
	Core::StringList dst;

	dst.Add(String::FromPrintf("%s\n", func));

	dst.Add(String::FromPrintf("\t base.addr                       = 0x%016" PRIx64 "\n", rt.base.addr));
	dst.Add(String::FromPrintf("\t pitch.pitch_div8_minus1         = 0x%08" PRIx32 "\n", rt.pitch.pitch_div8_minus1));
	dst.Add(String::FromPrintf("\t pitch.fmask_pitch_div8_minus1   = 0x%08" PRIx32 "\n", rt.pitch.fmask_pitch_div8_minus1));
	dst.Add(String::FromPrintf("\t slice.slice_div64_minus1        = 0x%08" PRIx32 "\n", rt.slice.slice_div64_minus1));
	dst.Add(String::FromPrintf("\t view.base_array_slice_index     = 0x%08" PRIx32 "\n", rt.view.base_array_slice_index));
	dst.Add(String::FromPrintf("\t view.last_array_slice_index     = 0x%08" PRIx32 "\n", rt.view.last_array_slice_index));
	dst.Add(String::FromPrintf("\t view.current_mip_level          = 0x%08" PRIx32 "\n", rt.view.current_mip_level));
	dst.Add(String::FromPrintf("\t info.fmask_compression_enable   = %s\n", rt.info.fmask_compression_enable ? "true" : "false"));

	// dst.Add(String::FromPrintf("\t info.fmask_compression_mode     = 0x%08" PRIx32 "\n", rt.info.fmask_compression_mode));
	dst.Add(String::FromPrintf("\t info.fmask_data_compression_disable = %s\n", rt.info.fmask_data_compression_disable ? "true" : "false"));
	dst.Add(String::FromPrintf("\t info.fmask_one_frag_mode        = %s\n", rt.info.fmask_one_frag_mode ? "true" : "false"));

	dst.Add(String::FromPrintf("\t info.cmask_fast_clear_enable    = %s\n", rt.info.cmask_fast_clear_enable ? "true" : "false"));
	dst.Add(String::FromPrintf("\t info.dcc_compression_enable     = %s\n", rt.info.dcc_compression_enable ? "true" : "false"));
	dst.Add(String::FromPrintf("\t info.neo_mode                   = %s\n", rt.info.neo_mode ? "true" : "false"));
	dst.Add(String::FromPrintf("\t info.cmask_tile_mode            = 0x%08" PRIx32 "\n", rt.info.cmask_tile_mode));
	dst.Add(String::FromPrintf("\t info.cmask_tile_mode_neo        = 0x%08" PRIx32 "\n", rt.info.cmask_tile_mode_neo));
	dst.Add(String::FromPrintf("\t info.format                     = 0x%08" PRIx32 "\n", rt.info.format));
	dst.Add(String::FromPrintf("\t info.channel_type               = 0x%08" PRIx32 "\n", rt.info.channel_type));
	dst.Add(String::FromPrintf("\t info.channel_order              = 0x%08" PRIx32 "\n", rt.info.channel_order));
	dst.Add(String::FromPrintf("\t info.blend_bypa                 = %s\n", rt.info.blend_bypass ? "true" : "false"));
	dst.Add(String::FromPrintf("\t info.blend_clamp                = %s\n", rt.info.blend_clamp ? "true" : "false"));
	dst.Add(String::FromPrintf("\t info.round_mode                 = %s\n", rt.info.round_mode ? "true" : "false"));
	dst.Add(String::FromPrintf("\t attrib.force_dest_alpha_to_one  = %s\n", rt.attrib.force_dest_alpha_to_one ? "true" : "false"));
	dst.Add(String::FromPrintf("\t attrib.tile_mode                = 0x%08" PRIx32 "\n", rt.attrib.tile_mode));
	dst.Add(String::FromPrintf("\t attrib.fmask_tile_mode          = 0x%08" PRIx32 "\n", rt.attrib.fmask_tile_mode));
	dst.Add(String::FromPrintf("\t attrib.num_samples              = 0x%08" PRIx32 "\n", rt.attrib.num_samples));
	dst.Add(String::FromPrintf("\t attrib.num_fragments            = 0x%08" PRIx32 "\n", rt.attrib.num_fragments));
	dst.Add(String::FromPrintf("\t attrib2.width                   = 0x%08" PRIx32 "\n", rt.attrib2.width));
	dst.Add(String::FromPrintf("\t attrib2.height                  = 0x%08" PRIx32 "\n", rt.attrib2.height));
	dst.Add(String::FromPrintf("\t attrib2.num_mip_levels          = 0x%08" PRIx32 "\n", rt.attrib2.num_mip_levels));
	dst.Add(String::FromPrintf("\t attrib3.depth                   = 0x%08" PRIx32 "\n", rt.attrib3.depth));
	dst.Add(String::FromPrintf("\t attrib3.tile_mode               = 0x%08" PRIx32 "\n", rt.attrib3.tile_mode));
	dst.Add(String::FromPrintf("\t attrib3.dimension               = 0x%08" PRIx32 "\n", rt.attrib3.dimension));
	dst.Add(String::FromPrintf("\t attrib3.cmask_pipe_aligned      = %s\n", rt.attrib3.cmask_pipe_aligned ? "true" : "false"));
	dst.Add(String::FromPrintf("\t attrib3.dcc_pipe_aligned        = %s\n", rt.attrib3.dcc_pipe_aligned ? "true" : "false"));
	dst.Add(String::FromPrintf("\t dcc.max_uncompressed_block_size = 0x%08" PRIx32 "\n", rt.dcc.max_uncompressed_block_size));
	dst.Add(String::FromPrintf("\t dcc.max_compressed_block_size   = 0x%08" PRIx32 "\n", rt.dcc.max_compressed_block_size));
	dst.Add(String::FromPrintf("\t dcc.min_compressed_block_size   = 0x%08" PRIx32 "\n", rt.dcc.min_compressed_block_size));
	dst.Add(String::FromPrintf("\t dcc.color_transform             = 0x%08" PRIx32 "\n", rt.dcc.color_transform));
	dst.Add(String::FromPrintf("\t dcc.overwrite_combiner_disable  = %s\n", rt.dcc.overwrite_combiner_disable ? "true" : "false"));
	dst.Add(String::FromPrintf("\t dcc.independent_64b_blocks      = %s\n", rt.dcc.independent_64b_blocks ? "true" : "false"));
	dst.Add(String::FromPrintf("\t dcc.independent_128b_blocks     = %s\n", rt.dcc.independent_128b_blocks ? "true" : "false"));
	dst.Add(String::FromPrintf("\t data_write_on_dcc_clear_to_reg  = %s\n", rt.dcc.data_write_on_dcc_clear_to_reg ? "true" : "false"));
	dst.Add(String::FromPrintf("\t dcc.dcc_clear_key_enable        = %s\n", rt.dcc.dcc_clear_key_enable ? "true" : "false"));
	dst.Add(String::FromPrintf("\t cmask.addr                      = 0x%016" PRIx64 "\n", rt.cmask.addr));
	dst.Add(String::FromPrintf("\t cmask_slice.slice_minus1        = 0x%08" PRIx32 "\n", rt.cmask_slice.slice_minus1));
	dst.Add(String::FromPrintf("\t fmask.addr                      = 0x%016" PRIx64 "\n", rt.fmask.addr));
	dst.Add(String::FromPrintf("\t fmask_slice.slice_minus1        = 0x%08" PRIx32 "\n", rt.fmask_slice.slice_minus1));
	dst.Add(String::FromPrintf("\t clear_word0.word0               = 0x%08" PRIx32 "\n", rt.clear_word0.word0));
	dst.Add(String::FromPrintf("\t clear_word1.word1               = 0x%08" PRIx32 "\n", rt.clear_word1.word1));
	dst.Add(String::FromPrintf("\t dcc_addr.addr                   = 0x%016" PRIx64 "\n", rt.dcc_addr.addr));
	dst.Add(String::FromPrintf("\t size.width                      = 0x%08" PRIx32 "\n", rt.size.width));
	dst.Add(String::FromPrintf("\t size.height                     = 0x%08" PRIx32 "\n", rt.size.height));

	return dst;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void rt_check(const HW::RenderTarget& rt)
{
	if (rt.base.addr != 0)
	{
		bool ps5 = Config::IsNextGen();
		// bool render_to_texture = (rt.attrib.tile_mode == 0x0d);
		//  EXIT_NOT_IMPLEMENTED(rt.base_addr == 0);
		if (ps5)
		{
			EXIT_NOT_IMPLEMENTED(rt.pitch.pitch_div8_minus1 != 0);
			EXIT_NOT_IMPLEMENTED(rt.pitch.fmask_pitch_div8_minus1 != 0);
			EXIT_NOT_IMPLEMENTED(rt.slice.slice_div64_minus1 != 0);
		}
		EXIT_NOT_IMPLEMENTED(rt.view.base_array_slice_index != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.view.last_array_slice_index != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.view.current_mip_level != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.info.fmask_compression_enable != false);

		// EXIT_NOT_IMPLEMENTED(rt.info.fmask_compression_mode != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.info.fmask_data_compression_disable != false);
		EXIT_NOT_IMPLEMENTED(rt.info.fmask_one_frag_mode != false);

		EXIT_NOT_IMPLEMENTED(rt.info.cmask_fast_clear_enable != false);
		EXIT_NOT_IMPLEMENTED(rt.info.dcc_compression_enable != false);
		EXIT_NOT_IMPLEMENTED(!(rt.attrib.tile_mode == 0x0d) && rt.info.neo_mode != Config::IsNeo());
		EXIT_NOT_IMPLEMENTED(rt.info.cmask_tile_mode != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.info.cmask_tile_mode_neo != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.info.blend_bypass != false);
		// EXIT_NOT_IMPLEMENTED(rt.info.blend_clamp != false);
		EXIT_NOT_IMPLEMENTED(rt.info.round_mode != false);
		//		 EXIT_NOT_IMPLEMENTED(rt.format != 0x0000000a);
		// EXIT_NOT_IMPLEMENTED(rt.channel_type != 0x00000006);
		// EXIT_NOT_IMPLEMENTED(rt.channel_order != 0x00000001);
		EXIT_NOT_IMPLEMENTED(rt.attrib.force_dest_alpha_to_one != false);
		// EXIT_NOT_IMPLEMENTED(rt.tile_mode != 0x0000000a);
		// EXIT_NOT_IMPLEMENTED(rt.fmask_tile_mode != 0x0000000a);
		EXIT_NOT_IMPLEMENTED(rt.attrib.num_samples != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.attrib.num_fragments != 0x00000000);
		if (ps5)
		{
			EXIT_NOT_IMPLEMENTED(rt.attrib2.width == 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib2.height == 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib2.num_mip_levels != 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.depth != 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.tile_mode != 0x0000001b);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.dimension != 0x00000001);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.cmask_pipe_aligned != true);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.dcc_pipe_aligned != true);
		} else
		{
			EXIT_NOT_IMPLEMENTED(rt.attrib2.width != 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib2.height != 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib2.num_mip_levels != 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.depth != 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.tile_mode != 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.dimension != 0x00000000);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.cmask_pipe_aligned != false);
			EXIT_NOT_IMPLEMENTED(rt.attrib3.dcc_pipe_aligned != false);
		}
		// EXIT_NOT_IMPLEMENTED(rt.dcc_max_uncompressed_block_size != 0x00000002);
		// EXIT_NOT_IMPLEMENTED(rt.dcc.max_compressed_block_size != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.dcc.min_compressed_block_size != 0x00000000);
		// EXIT_NOT_IMPLEMENTED(rt.dcc.color_transform != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.dcc.overwrite_combiner_disable != false);
		// EXIT_NOT_IMPLEMENTED(rt.dcc.force_independent_blocks != false);
		// EXIT_NOT_IMPLEMENTED(rt.dcc.independent_128b_blocks != false);
		// EXIT_NOT_IMPLEMENTED(rt.dcc.data_write_on_dcc_clear_to_reg != false);
		EXIT_NOT_IMPLEMENTED(rt.dcc.dcc_clear_key_enable != false);
		EXIT_NOT_IMPLEMENTED(rt.cmask.addr != 0x0000000000000000);
		EXIT_NOT_IMPLEMENTED(rt.cmask_slice.slice_minus1 != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.fmask.addr != 0x0000000000000000);
		EXIT_NOT_IMPLEMENTED(rt.fmask_slice.slice_minus1 != 0x00000000 && rt.fmask_slice.slice_minus1 != rt.slice.slice_div64_minus1);
		EXIT_NOT_IMPLEMENTED(rt.clear_word0.word0 != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.clear_word1.word1 != 0x00000000);
		EXIT_NOT_IMPLEMENTED(rt.dcc_addr.addr != 0x0000000000000000);
		if (ps5)
		{
			EXIT_NOT_IMPLEMENTED(rt.size.width != 0);
			EXIT_NOT_IMPLEMENTED(rt.size.height != 0);
		}
	}
}

static void z_print(const char* func, const HW::DepthRenderTarget& z)
{
	printf("%s\n", func);

	printf("\t z_info.format                         = 0x%08" PRIx32 "\n", z.z_info.format);
	printf("\t z_info.tile_mode_index                = 0x%08" PRIx32 "\n", z.z_info.tile_mode_index);
	printf("\t z_info.num_samples                    = 0x%08" PRIx32 "\n", z.z_info.num_samples);
	printf("\t z_info.tile_surface_enable            = %s\n", z.z_info.tile_surface_enable ? "true" : "false");
	printf("\t z_info.expclear_enabled               = %s\n", z.z_info.expclear_enabled ? "true" : "false");
	printf("\t z_info.zrange_precision               = 0x%08" PRIx32 "\n", z.z_info.zrange_precision);
	printf("\t z_info.embedded_sample_locations      = %s\n", z.z_info.embedded_sample_locations ? "true" : "false");
	printf("\t z_info.partially_resident             = %s\n", z.z_info.partially_resident ? "true" : "false");
	printf("\t z_info.num_mip_levels                 = 0x%02" PRIx8 "\n", z.z_info.num_mip_levels);
	printf("\t z_info.plane_compression              = 0x%02" PRIx8 "\n", z.z_info.plane_compression);
	printf("\t stencil_info.format                   = 0x%08" PRIx32 "\n", z.stencil_info.format);
	printf("\t stencil_info.tile_stencil_disable     = %s\n", z.stencil_info.tile_stencil_disable ? "true" : "false");
	printf("\t stencil_info.expclear_enabled         = %s\n", z.stencil_info.expclear_enabled ? "true" : "false");
	printf("\t stencil_info.tile_mode_index          = 0x%08" PRIx32 "\n", z.stencil_info.tile_mode_index);
	printf("\t stencil_info.tile_split               = 0x%08" PRIx32 "\n", z.stencil_info.tile_split);
	printf("\t stencil_info.texture_compatible_stencil = %s\n", z.stencil_info.texture_compatible_stencil ? "true" : "false");
	printf("\t stencil_info.partially_resident       = %s\n", z.stencil_info.partially_resident ? "true" : "false");
	printf("\t depth_info.addr5_swizzle_mask         = 0x%08" PRIx32 "\n", z.depth_info.addr5_swizzle_mask);
	printf("\t depth_info.array_mode                 = 0x%08" PRIx32 "\n", z.depth_info.array_mode);
	printf("\t depth_info.pipe_config                = 0x%08" PRIx32 "\n", z.depth_info.pipe_config);
	printf("\t depth_info.bank_width                 = 0x%08" PRIx32 "\n", z.depth_info.bank_width);
	printf("\t depth_info.bank_height                = 0x%08" PRIx32 "\n", z.depth_info.bank_height);
	printf("\t depth_info.macro_tile_aspect          = 0x%08" PRIx32 "\n", z.depth_info.macro_tile_aspect);
	printf("\t depth_info.num_banks                  = 0x%08" PRIx32 "\n", z.depth_info.num_banks);
	printf("\t depth_view.slice_start                = 0x%08" PRIx32 "\n", z.depth_view.slice_start);
	printf("\t depth_view.slice_max                  = 0x%08" PRIx32 "\n", z.depth_view.slice_max);
	printf("\t depth_view.current_mip_level          = 0x%02" PRIx8 "\n", z.depth_view.current_mip_level);
	printf("\t depth_view.depth_write_disable        = %s\n", z.depth_view.depth_write_disable ? "true" : "false");
	printf("\t depth_view.stencil_write_disable      = %s\n", z.depth_view.stencil_write_disable ? "true" : "false");
	printf("\t htile_surface.linear                  = 0x%08" PRIx32 "\n", z.htile_surface.linear);
	printf("\t htile_surface.full_cache              = 0x%08" PRIx32 "\n", z.htile_surface.full_cache);
	printf("\t htile_surface.htile_uses_preload_win  = 0x%08" PRIx32 "\n", z.htile_surface.htile_uses_preload_win);
	printf("\t htile_surface.preload                 = 0x%08" PRIx32 "\n", z.htile_surface.preload);
	printf("\t htile_surface.prefetch_width          = 0x%08" PRIx32 "\n", z.htile_surface.prefetch_width);
	printf("\t htile_surface.prefetch_height         = 0x%08" PRIx32 "\n", z.htile_surface.prefetch_height);
	printf("\t htile_surface.dst_outside_zero_to_one = 0x%08" PRIx32 "\n", z.htile_surface.dst_outside_zero_to_one);
	printf("\t z_read_base_addr                      = 0x%016" PRIx64 "\n", z.z_read_base_addr);
	printf("\t stencil_read_base_addr                = 0x%016" PRIx64 "\n", z.stencil_read_base_addr);
	printf("\t z_write_base_addr                     = 0x%016" PRIx64 "\n", z.z_write_base_addr);
	printf("\t stencil_write_base_addr               = 0x%016" PRIx64 "\n", z.stencil_write_base_addr);
	printf("\t pitch_div8_minus1                     = 0x%08" PRIx32 "\n", z.pitch_div8_minus1);
	printf("\t height_div8_minus1                    = 0x%08" PRIx32 "\n", z.height_div8_minus1);
	printf("\t slice_div64_minus1                    = 0x%08" PRIx32 "\n", z.slice_div64_minus1);
	printf("\t htile_data_base_addr                  = 0x%016" PRIx64 "\n", z.htile_data_base_addr);
	printf("\t width                                 = 0x%08" PRIx32 "\n", z.width);
	printf("\t height                                = 0x%08" PRIx32 "\n", z.height);
	printf("\t size.x_max                            = 0x%04" PRIx16 "\n", z.size.x_max);
	printf("\t size.y_max                            = 0x%04" PRIx16 "\n", z.size.y_max);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void z_check(const HW::DepthRenderTarget& z)
{
	if (z.z_info.format == 0)
	{
		EXIT_NOT_IMPLEMENTED(z.z_info.format != 0);
		EXIT_NOT_IMPLEMENTED(z.z_info.tile_mode_index != 0);
		EXIT_NOT_IMPLEMENTED(z.z_info.num_samples != 0);
		EXIT_NOT_IMPLEMENTED(z.z_info.tile_surface_enable != false);
		EXIT_NOT_IMPLEMENTED(z.z_info.expclear_enabled != false);
		EXIT_NOT_IMPLEMENTED(z.z_info.zrange_precision != 0);
		EXIT_NOT_IMPLEMENTED(z.z_info.embedded_sample_locations != false);
		EXIT_NOT_IMPLEMENTED(z.z_info.partially_resident != false);
		EXIT_NOT_IMPLEMENTED(z.z_info.num_mip_levels != 0);
		EXIT_NOT_IMPLEMENTED(z.z_info.plane_compression != 0);
	} else
	{
		EXIT_NOT_IMPLEMENTED(z.z_info.format != 0x00000003);
		// EXIT_NOT_IMPLEMENTED(z.z_info.tile_mode_index != (Config::IsNeo() ? 0x00000002 : 0));
		EXIT_NOT_IMPLEMENTED(z.z_info.num_samples != 0x00000000);
		// EXIT_NOT_IMPLEMENTED(z.z_info.tile_surface_enable != true);
		EXIT_NOT_IMPLEMENTED(z.z_info.expclear_enabled != false);
		EXIT_NOT_IMPLEMENTED(z.z_info.zrange_precision != 0x00000001);
		EXIT_NOT_IMPLEMENTED(z.z_info.embedded_sample_locations != false);
		EXIT_NOT_IMPLEMENTED(z.z_info.partially_resident != false);
		EXIT_NOT_IMPLEMENTED(z.z_info.num_mip_levels != 0);
		EXIT_NOT_IMPLEMENTED(z.z_info.plane_compression != 0);
	}

	if (z.stencil_info.format == 0)
	{
		// EXIT_NOT_IMPLEMENTED(z.stencil_info.format != 0);
		//  EXIT_NOT_IMPLEMENTED(z.stencil_info.tile_stencil_disable != false);
		EXIT_NOT_IMPLEMENTED(z.stencil_info.expclear_enabled != false);
		// EXIT_NOT_IMPLEMENTED(z.stencil_info.tile_mode_index != 0);
		// EXIT_NOT_IMPLEMENTED(z.stencil_info.tile_split != 0);
		// EXIT_NOT_IMPLEMENTED(z.stencil_info.texture_compatible_stencil != true);
		EXIT_NOT_IMPLEMENTED(z.stencil_info.partially_resident != false);
	} else
	{
		// EXIT_NOT_IMPLEMENTED(z.stencil_info.format != 0x00000001);
		EXIT_NOT_IMPLEMENTED(z.stencil_info.tile_stencil_disable != true);
		EXIT_NOT_IMPLEMENTED(z.stencil_info.expclear_enabled != false);
		// EXIT_NOT_IMPLEMENTED(z.stencil_info.tile_mode_index != (Config::IsNeo() ? 0x00000002 : 0));
		// EXIT_NOT_IMPLEMENTED(z.stencil_info.tile_split != (Config::IsNeo() ? 0x00000002 : 0));
		// EXIT_NOT_IMPLEMENTED(z.stencil_info.texture_compatible_stencil != true);
		EXIT_NOT_IMPLEMENTED(z.stencil_info.partially_resident != false);
	}

	if (z.z_info.format != 0 || z.stencil_info.format != 0)
	{
		bool ps5 = Config::IsNextGen();

		if (ps5)
		{
			EXIT_NOT_IMPLEMENTED(z.depth_info.addr5_swizzle_mask != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.depth_info.array_mode != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.depth_info.pipe_config != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.depth_info.bank_width != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.depth_info.bank_height != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.depth_info.macro_tile_aspect != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.depth_info.num_banks != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.linear != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.full_cache != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.htile_uses_preload_win != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.preload != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.prefetch_width != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.prefetch_height != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.dst_outside_zero_to_one != 0x00000000);
		} else
		{
			EXIT_NOT_IMPLEMENTED(z.depth_info.addr5_swizzle_mask != 0x00000001);
			EXIT_NOT_IMPLEMENTED(z.depth_info.array_mode != 0x00000004);
			EXIT_NOT_IMPLEMENTED(z.depth_info.pipe_config != (Config::IsNeo() ? 0x00000012 : 0x0c));
			EXIT_NOT_IMPLEMENTED(z.depth_info.bank_width != 0x00000000);
			// EXIT_NOT_IMPLEMENTED(z.depth_info.bank_height != (Config::IsNeo() ? 0x00000001 : 2));
			// EXIT_NOT_IMPLEMENTED(z.depth_info.macro_tile_aspect != (Config::IsNeo() ? 0x00000000 : 2));
			// EXIT_NOT_IMPLEMENTED(z.depth_info.num_banks != (Config::IsNeo() ? 0x00000002 : 3));
			EXIT_NOT_IMPLEMENTED(z.htile_surface.linear != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.full_cache != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.htile_uses_preload_win != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.preload != 0x00000001);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.prefetch_width != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.prefetch_height != 0x00000000);
			EXIT_NOT_IMPLEMENTED(z.htile_surface.dst_outside_zero_to_one != 0x00000000);
		}
		EXIT_NOT_IMPLEMENTED(z.depth_view.slice_start != 0x00000000);
		EXIT_NOT_IMPLEMENTED(z.depth_view.slice_max != 0x00000000);
		EXIT_NOT_IMPLEMENTED(z.depth_view.current_mip_level != 0x00000000);
		EXIT_NOT_IMPLEMENTED(z.depth_view.depth_write_disable != false);
		EXIT_NOT_IMPLEMENTED(z.depth_view.stencil_write_disable != false);
		EXIT_NOT_IMPLEMENTED(z.z_read_base_addr != z.z_write_base_addr);
		EXIT_NOT_IMPLEMENTED(z.stencil_read_base_addr != z.stencil_write_base_addr);
		EXIT_NOT_IMPLEMENTED(z.z_write_base_addr == 0);
		EXIT_NOT_IMPLEMENTED(z.stencil_info.format != 0 && z.stencil_write_base_addr == 0);
		// EXIT_NOT_IMPLEMENTED(z.pitch_div8_minus1 != 0x000000ff);
		// EXIT_NOT_IMPLEMENTED(z.height_div8_minus1 != 0x0000008f);
		// EXIT_NOT_IMPLEMENTED(z.slice_div64_minus1 != 0x00008fff);
		// EXIT_NOT_IMPLEMENTED(z.htile_data_base_addr == 0);
		// EXIT_NOT_IMPLEMENTED(z.width != 0x00000780);
		// EXIT_NOT_IMPLEMENTED(z.height != 0x00000438);
		if (ps5)
		{
			EXIT_NOT_IMPLEMENTED(z.width != 0);
			EXIT_NOT_IMPLEMENTED(z.height != 0);
			EXIT_NOT_IMPLEMENTED(z.size.x_max == 0);
			EXIT_NOT_IMPLEMENTED(z.size.y_max == 0);
		}
	}
}

static void clip_print(const char* func, const HW::ClipControl& c)
{
	printf("%s\n", func);

	printf("\t user_clip_planes                    = 0x%02" PRIx8 "\n", c.user_clip_planes);
	printf("\t user_clip_plane_mode                = 0x%02" PRIx8 "\n", c.user_clip_plane_mode);
	printf("\t dx_clip_space                       = %s\n", c.dx_clip_space ? "true" : "false");
	printf("\t vertex_kill_any                     = %s\n", c.vertex_kill_any ? "true" : "false");
	printf("\t min_z_clip_disable                  = %s\n", c.min_z_clip_disable ? "true" : "false");
	printf("\t max_z_clip_disable                  = %s\n", c.max_z_clip_disable ? "true" : "false");
	printf("\t user_clip_plane_negate_y            = %s\n", c.user_clip_plane_negate_y ? "true" : "false");
	printf("\t clip_disable                        = %s\n", c.clip_disable ? "true" : "false");
	printf("\t user_clip_plane_cull_only           = %s\n", c.user_clip_plane_cull_only ? "true" : "false");
	printf("\t cull_on_clipping_error_disable      = %s\n", c.cull_on_clipping_error_disable ? "true" : "false");
	printf("\t linear_attribute_clip_enable        = %s\n", c.linear_attribute_clip_enable ? "true" : "false");
	printf("\t force_viewport_index_from_vs_enable = %s\n", c.force_viewport_index_from_vs_enable ? "true" : "false");
}

static void clip_check(const HW::ClipControl& c)
{
	EXIT_NOT_IMPLEMENTED(c.user_clip_planes != 0);
	EXIT_NOT_IMPLEMENTED(c.user_clip_plane_mode != 0);
	EXIT_NOT_IMPLEMENTED(c.dx_clip_space != false);
	EXIT_NOT_IMPLEMENTED(c.vertex_kill_any != false);
	EXIT_NOT_IMPLEMENTED(c.min_z_clip_disable != false);
	EXIT_NOT_IMPLEMENTED(c.max_z_clip_disable != false);
	EXIT_NOT_IMPLEMENTED(c.user_clip_plane_negate_y != false);
	EXIT_NOT_IMPLEMENTED(c.clip_disable != false);
	EXIT_NOT_IMPLEMENTED(c.user_clip_plane_cull_only != false);
	EXIT_NOT_IMPLEMENTED(c.cull_on_clipping_error_disable != false);
	EXIT_NOT_IMPLEMENTED(c.linear_attribute_clip_enable != false);
	EXIT_NOT_IMPLEMENTED(c.force_viewport_index_from_vs_enable != false);
}

static void rc_print(const char* func, const HW::RenderControl& c)
{
	printf("%s\n", func);

	printf("\t depth_clear_enable       = %s\n", c.depth_clear_enable ? "true" : "false");
	printf("\t stencil_clear_enable     = %s\n", c.stencil_clear_enable ? "true" : "false");
	printf("\t resummarize_enable       = %s\n", c.resummarize_enable ? "true" : "false");
	printf("\t stencil_compress_disable = %s\n", c.stencil_compress_disable ? "true" : "false");
	printf("\t depth_compress_disable   = %s\n", c.depth_compress_disable ? "true" : "false");
	printf("\t copy_centroid            = %s\n", c.copy_centroid ? "true" : "false");
	printf("\t copy_sample              = %" PRIu8 "\n", c.copy_sample);
}

static void rc_check(const HW::RenderControl& c)
{
	// EXIT_NOT_IMPLEMENTED(c.depth_clear_enable != false);
	// EXIT_NOT_IMPLEMENTED(c.stencil_clear_enable != false);
	EXIT_NOT_IMPLEMENTED(c.resummarize_enable != false);
	// EXIT_NOT_IMPLEMENTED(c.stencil_compress_disable != false);
	// EXIT_NOT_IMPLEMENTED(c.depth_compress_disable != false);
	EXIT_NOT_IMPLEMENTED(c.copy_centroid != false);
	EXIT_NOT_IMPLEMENTED(c.copy_sample != 0);
}

static void mc_print(const char* func, const HW::ModeControl& c)
{
	printf("%s\n", func);

	printf("\t cull_front               = %s\n", c.cull_front ? "true" : "false");
	printf("\t cull_back                = %s\n", c.cull_back ? "true" : "false");
	printf("\t face                     = %s\n", c.face ? "true" : "false");
	printf("\t poly_mode                = %" PRIu8 "\n", c.poly_mode);
	printf("\t polymode_front_ptype     = %" PRIu8 "\n", c.polymode_front_ptype);
	printf("\t polymode_back_ptype      = %" PRIu8 "\n", c.polymode_back_ptype);
	printf("\t poly_offset_front_enable = %s\n", c.poly_offset_front_enable ? "true" : "false");
	printf("\t poly_offset_back_enable  = %s\n", c.poly_offset_back_enable ? "true" : "false");
	printf("\t vtx_window_offset_enable = %s\n", c.vtx_window_offset_enable ? "true" : "false");
	printf("\t provoking_vtx_last       = %s\n", c.provoking_vtx_last ? "true" : "false");
	printf("\t persp_corr_dis           = %s\n", c.persp_corr_dis ? "true" : "false");
}

static void mc_check(const HW::ModeControl& c)
{
	// EXIT_NOT_IMPLEMENTED(c.cull_front != false);
	// EXIT_NOT_IMPLEMENTED(c.cull_back != false);
	// EXIT_NOT_IMPLEMENTED(c.face != false);
	EXIT_NOT_IMPLEMENTED(c.poly_mode != 0);
	EXIT_NOT_IMPLEMENTED(c.polymode_front_ptype != 0 && c.polymode_front_ptype != 2);
	EXIT_NOT_IMPLEMENTED(c.polymode_back_ptype != 0 && c.polymode_back_ptype != 2);
	EXIT_NOT_IMPLEMENTED(c.poly_offset_front_enable != false);
	EXIT_NOT_IMPLEMENTED(c.poly_offset_back_enable != false);
	EXIT_NOT_IMPLEMENTED(c.vtx_window_offset_enable != false);
	EXIT_NOT_IMPLEMENTED(c.provoking_vtx_last != false);
	EXIT_NOT_IMPLEMENTED(c.persp_corr_dis != false);
}

static void bc_print(const char* func, const HW::BlendControl& c, const HW::BlendColor& color, const HW::ColorControl& cc)
{
	printf("%s\n", func);

	printf("\t color_srcblend       = %" PRIu8 "\n", c.color_srcblend);
	printf("\t color_comb_fcn       = %" PRIu8 "\n", c.color_comb_fcn);
	printf("\t color_destblend      = %" PRIu8 "\n", c.color_destblend);
	printf("\t alpha_srcblend       = %" PRIu8 "\n", c.alpha_srcblend);
	printf("\t alpha_comb_fcn       = %" PRIu8 "\n", c.alpha_comb_fcn);
	printf("\t alpha_destblend      = %" PRIu8 "\n", c.alpha_destblend);
	printf("\t separate_alpha_blend = %s\n", c.separate_alpha_blend ? "true" : "false");
	printf("\t enable               = %s\n", c.enable ? "true" : "false");
	printf("\t red                  = %f\n", color.red);
	printf("\t green                = %f\n", color.green);
	printf("\t blue                 = %f\n", color.blue);
	printf("\t alpha                = %f\n", color.alpha);
	printf("\t cc.mode              = %" PRIu8 "\n", cc.mode);
	printf("\t cc.op                = %" PRIu8 "\n", cc.op);
}

static void bc_check(const HW::BlendControl& /*c*/, const HW::BlendColor& color, const HW::ColorControl& cc)
{
	// EXIT_NOT_IMPLEMENTED(c.color_srcblend != 0);
	// EXIT_NOT_IMPLEMENTED(c.color_comb_fcn != 0);
	// EXIT_NOT_IMPLEMENTED(c.color_destblend != 0);
	// EXIT_NOT_IMPLEMENTED(c.alpha_srcblend != 0);
	// EXIT_NOT_IMPLEMENTED(c.alpha_comb_fcn != 0);
	// EXIT_NOT_IMPLEMENTED(c.alpha_destblend != 0);
	// EXIT_NOT_IMPLEMENTED(c.separate_alpha_blend != false);
	// EXIT_NOT_IMPLEMENTED(c.enable != false);
	EXIT_NOT_IMPLEMENTED(color.red != 0.0f);
	EXIT_NOT_IMPLEMENTED(color.green != 0.0f);
	EXIT_NOT_IMPLEMENTED(color.blue != 0.0f);
	EXIT_NOT_IMPLEMENTED(color.alpha != 0.0f);
	EXIT_NOT_IMPLEMENTED(cc.mode != 1 && cc.mode != 0);
	EXIT_NOT_IMPLEMENTED(cc.op != 0xCC);
}

static void d_print(const char* func, const HW::DepthControl& c, const HW::StencilControl& s, const HW::StencilMask& sm)
{
	printf("%s\n", func);

	printf("\t stencil_enable       = %s\n", c.stencil_enable ? "true" : "false");
	printf("\t z_enable             = %s\n", c.z_enable ? "true" : "false");
	printf("\t z_write_enable       = %s\n", c.z_write_enable ? "true" : "false");
	printf("\t depth_bounds_enable  = %s\n", c.depth_bounds_enable ? "true" : "false");
	printf("\t zfunc                = %" PRIu8 "\n", c.zfunc);
	printf("\t backface_enable      = %s\n", c.backface_enable ? "true" : "false");
	printf("\t stencilfunc          = %" PRIu8 "\n", c.stencilfunc);
	printf("\t stencilfunc_bf       = %" PRIu8 "\n", c.stencilfunc_bf);
	printf("\t color_writes_on_depth_fail_enable  = %s\n", c.color_writes_on_depth_fail_enable ? "true" : "false");
	printf("\t color_writes_on_depth_pass_disable = %s\n", c.color_writes_on_depth_pass_disable ? "true" : "false");
	printf("\t stencil_fail         = %" PRIu8 "\n", s.stencil_fail);
	printf("\t stencil_zpass        = %" PRIu8 "\n", s.stencil_zpass);
	printf("\t stencil_zfail        = %" PRIu8 "\n", s.stencil_zfail);
	printf("\t stencil_fail_bf      = %" PRIu8 "\n", s.stencil_fail_bf);
	printf("\t stencil_zpass_bf     = %" PRIu8 "\n", s.stencil_zpass_bf);
	printf("\t stencil_zfail_bf     = %" PRIu8 "\n", s.stencil_zfail_bf);
	printf("\t stencil_testval      = %" PRIu8 "\n", sm.stencil_testval);
	printf("\t stencil_mask         = %" PRIu8 "\n", sm.stencil_mask);
	printf("\t stencil_writemask    = %" PRIu8 "\n", sm.stencil_writemask);
	printf("\t stencil_opval        = %" PRIu8 "\n", sm.stencil_opval);
	printf("\t stencil_testval_bf   = %" PRIu8 "\n", sm.stencil_testval_bf);
	printf("\t stencil_mask_bf      = %" PRIu8 "\n", sm.stencil_mask_bf);
	printf("\t stencil_writemask_bf = %" PRIu8 "\n", sm.stencil_writemask_bf);
	printf("\t stencil_opval_bf     = %" PRIu8 "\n", sm.stencil_opval_bf);
}

static void d_check(const HW::DepthControl& c, const HW::StencilControl& s, const HW::StencilMask& /*sm*/)
{
	// EXIT_NOT_IMPLEMENTED(c.stencil_enable != false);
	// EXIT_NOT_IMPLEMENTED(c.z_enable != false);
	// EXIT_NOT_IMPLEMENTED(c.z_write_enable != false);
	EXIT_NOT_IMPLEMENTED(c.depth_bounds_enable != false);
	// EXIT_NOT_IMPLEMENTED(c.zfunc != 0);
	EXIT_NOT_IMPLEMENTED(c.backface_enable != false);
	// EXIT_NOT_IMPLEMENTED(c.stencilfunc != 0);
	// EXIT_NOT_IMPLEMENTED(c.stencilfunc_bf != 0);
	EXIT_NOT_IMPLEMENTED(c.color_writes_on_depth_fail_enable != false);
	EXIT_NOT_IMPLEMENTED(c.color_writes_on_depth_pass_disable != false);
	// EXIT_NOT_IMPLEMENTED(s.stencil_fail != 0);
	// EXIT_NOT_IMPLEMENTED(s.stencil_zpass != 0);
	// EXIT_NOT_IMPLEMENTED(s.stencil_zfail != 0);
	EXIT_NOT_IMPLEMENTED(s.stencil_fail_bf != 0);
	EXIT_NOT_IMPLEMENTED(s.stencil_zpass_bf != 0);
	EXIT_NOT_IMPLEMENTED(s.stencil_zfail_bf != 0);
	// EXIT_NOT_IMPLEMENTED(sm.stencil_testval != 0);
	// EXIT_NOT_IMPLEMENTED(sm.stencil_mask != 0);
	// EXIT_NOT_IMPLEMENTED(sm.stencil_writemask != 0);
	// EXIT_NOT_IMPLEMENTED(sm.stencil_opval != 0);
	// EXIT_NOT_IMPLEMENTED(sm.stencil_testval_bf != 0);
	// EXIT_NOT_IMPLEMENTED(sm.stencil_mask_bf != 0);
	// EXIT_NOT_IMPLEMENTED(sm.stencil_writemask_bf != 0);
	// EXIT_NOT_IMPLEMENTED(sm.stencil_opval_bf != 0);
}

static void eqaa_print(const char* func, const HW::EqaaControl& c)
{
	printf("%s\n", func);

	printf("\t max_anchor_samples         = %" PRIu8 "\n", c.max_anchor_samples);
	printf("\t ps_iter_samples            = %" PRIu8 "\n", c.ps_iter_samples);
	printf("\t mask_export_num_samples    = %" PRIu8 "\n", c.mask_export_num_samples);
	printf("\t alpha_to_mask_num_samples  = %" PRIu8 "\n", c.alpha_to_mask_num_samples);
	printf("\t high_quality_intersections = %s\n", c.high_quality_intersections ? "true" : "false");
	printf("\t incoherent_eqaa_reads      = %s\n", c.incoherent_eqaa_reads ? "true" : "false");
	printf("\t interpolate_comp_z         = %s\n", c.interpolate_comp_z ? "true" : "false");
	printf("\t static_anchor_associations = %s\n", c.static_anchor_associations ? "true" : "false");
}

static void eqaa_check(const HW::EqaaControl& c)
{
	EXIT_NOT_IMPLEMENTED(c.max_anchor_samples != 0);
	EXIT_NOT_IMPLEMENTED(c.ps_iter_samples != 0);
	EXIT_NOT_IMPLEMENTED(c.mask_export_num_samples != 0);
	EXIT_NOT_IMPLEMENTED(c.alpha_to_mask_num_samples != 0);
	EXIT_NOT_IMPLEMENTED(c.high_quality_intersections != false);
	EXIT_NOT_IMPLEMENTED(c.incoherent_eqaa_reads != false);
	EXIT_NOT_IMPLEMENTED(c.interpolate_comp_z != false);
	// EXIT_NOT_IMPLEMENTED(c.static_anchor_associations != false);
}

static void aa_print(const char* func, const HW::AaSampleControl& c, const HW::AaConfig& cf)
{
	printf("%s\n", func);

	printf("\t centroid_priority = %016" PRIx64 "\n", c.centroid_priority);
	for (int i = 0; i < 16; i++)
	{
		printf("\t locations[%d] = %08" PRIx32 "\n", i, c.locations[i]);
	}
	printf("\t msaa_num_samples      = %" PRIu8 "\n", cf.msaa_num_samples);
	printf("\t aa_mask_centroid_dtmn = %s\n", cf.aa_mask_centroid_dtmn ? "true" : "false");
	printf("\t max_sample_dist       = %" PRIu8 "\n", cf.max_sample_dist);
	printf("\t msaa_exposed_samples  = %" PRIu8 "\n", cf.msaa_exposed_samples);
}

static void aa_check(const HW::AaSampleControl& c, const HW::AaConfig& cf)
{
	EXIT_NOT_IMPLEMENTED(c.centroid_priority != 0);
	for (uint32_t l: c.locations)
	{
		EXIT_NOT_IMPLEMENTED(l != 0);
	}
	EXIT_NOT_IMPLEMENTED(cf.msaa_num_samples != 0);
	EXIT_NOT_IMPLEMENTED(cf.aa_mask_centroid_dtmn != false);
	EXIT_NOT_IMPLEMENTED(cf.max_sample_dist != 0);
	EXIT_NOT_IMPLEMENTED(cf.msaa_exposed_samples != 0);
}

static void vp_print(const char* func, const HW::ScreenViewport& vp, const HW::ScanModeControl& smc)
{
	printf("%s\n", func);

	printf("\t msaa_enable                    = %s\n", smc.msaa_enable ? "true" : "false");
	printf("\t vport_scissor_enable           = %s\n", smc.vport_scissor_enable ? "true" : "false");
	printf("\t line_stipple_enable            = %s\n", smc.line_stipple_enable ? "true" : "false");
	printf("\t vp[0].zmin                     = %f\n", vp.viewports[0].zmin);
	printf("\t vp[0].zmax                     = %f\n", vp.viewports[0].zmax);
	printf("\t vp[0].xscale                   = %f\n", vp.viewports[0].xscale);
	printf("\t vp[0].xoffset                  = %f\n", vp.viewports[0].xoffset);
	printf("\t vp[0].yscale                   = %f\n", vp.viewports[0].yscale);
	printf("\t vp[0].yoffset                  = %f\n", vp.viewports[0].yoffset);
	printf("\t vp[0].zscale                   = %f\n", vp.viewports[0].zscale);
	printf("\t vp[0].zoffset                  = %f\n", vp.viewports[0].zoffset);
	printf("\t vp[0].viewport_scissor_left    = %d\n", vp.viewports[0].viewport_scissor_left);
	printf("\t vp[0].viewport_scissor_top     = %d\n", vp.viewports[0].viewport_scissor_top);
	printf("\t vp[0].viewport_scissor_right   = %d\n", vp.viewports[0].viewport_scissor_right);
	printf("\t vp[0].viewport_scissor_bottom  = %d\n", vp.viewports[0].viewport_scissor_bottom);
	printf("\t transform_control              = 0x%08" PRIx32 "\n", vp.transform_control);
	printf("\t screen_scissor_left            = %d\n", vp.screen_scissor_left);
	printf("\t screen_scissor_top             = %d\n", vp.screen_scissor_top);
	printf("\t screen_scissor_right           = %d\n", vp.screen_scissor_right);
	printf("\t screen_scissor_bottom          = %d\n", vp.screen_scissor_bottom);
	printf("\t generic_scissor_left           = %d\n", vp.generic_scissor_left);
	printf("\t generic_scissor_top            = %d\n", vp.generic_scissor_top);
	printf("\t generic_scissor_right          = %d\n", vp.generic_scissor_right);
	printf("\t generic_scissor_bottom         = %d\n", vp.generic_scissor_bottom);
	printf("\t hw_offset_x                    = %u\n", vp.hw_offset_x);
	printf("\t hw_offset_y                    = %u\n", vp.hw_offset_y);
	printf("\t guard_band_horz_clip           = %f\n", vp.guard_band_horz_clip);
	printf("\t guard_band_vert_clip           = %f\n", vp.guard_band_vert_clip);
	printf("\t guard_band_horz_discard        = %f\n", vp.guard_band_horz_discard);
	printf("\t guard_band_vert_discard        = %f\n", vp.guard_band_vert_discard);
	printf("\t generic_scissor_window_offset_enable               = %s\n", vp.generic_scissor_window_offset_enable ? "true" : "false");
	printf("\t viewports[0].viewport_scissor_window_offset_enable = %s\n",
	       vp.viewports[0].viewport_scissor_window_offset_enable ? "true" : "false");
}

static void vp_check(const HW::ScreenViewport& vp, const HW::ScanModeControl& smc)
{
	bool ps5 = Config::IsNextGen();

	EXIT_NOT_IMPLEMENTED(smc.msaa_enable);
	// EXIT_NOT_IMPLEMENTED(smc.vport_scissor_enable);
	EXIT_NOT_IMPLEMENTED(smc.line_stipple_enable);

	bool generic_scissor =
	    (vp.generic_scissor_left != 0 || vp.generic_scissor_top != 0 || vp.generic_scissor_right != 0 || vp.generic_scissor_bottom != 0);
	bool screen_scissor =
	    (vp.screen_scissor_left != 0 || vp.screen_scissor_top != 0 || vp.screen_scissor_right != 0 || vp.screen_scissor_bottom != 0);
	bool viewport_scissor = (vp.viewports[0].viewport_scissor_left != 0 || vp.viewports[0].viewport_scissor_top != 0 ||
	                         vp.viewports[0].viewport_scissor_right != 0 || vp.viewports[0].viewport_scissor_bottom != 0);

	EXIT_NOT_IMPLEMENTED(viewport_scissor && (generic_scissor || screen_scissor));
	EXIT_NOT_IMPLEMENTED(viewport_scissor && (!smc.vport_scissor_enable));

	EXIT_NOT_IMPLEMENTED(vp.viewports[0].zmin > 0.000000);
	EXIT_NOT_IMPLEMENTED(vp.viewports[0].zmax != 1.000000);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].xscale != 960.000000);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].xoffset != 960.000000);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].yscale != -540.000000);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].yoffset != 540.000000);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].zscale != 0.500000);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].zoffset != 0.500000);
	EXIT_NOT_IMPLEMENTED(vp.transform_control != 1087);
	EXIT_NOT_IMPLEMENTED(generic_scissor && !(vp.screen_scissor_left <= vp.generic_scissor_left));
	EXIT_NOT_IMPLEMENTED(generic_scissor && !(vp.screen_scissor_top <= vp.generic_scissor_top));
	EXIT_NOT_IMPLEMENTED(generic_scissor && !(vp.screen_scissor_right >= vp.generic_scissor_right));
	EXIT_NOT_IMPLEMENTED(generic_scissor && !(vp.screen_scissor_bottom >= vp.generic_scissor_bottom));
	// EXIT_NOT_IMPLEMENTED(vp.hw_offset_x != 60);
	// EXIT_NOT_IMPLEMENTED(vp.hw_offset_y != 32);
	// EXIT_NOT_IMPLEMENTED(fabsf(vp.guard_band_horz_clip - 33.133327f) > 0.001f);
	// EXIT_NOT_IMPLEMENTED(fabsf(vp.guard_band_vert_clip - 59.629623f) > 0.001f);
	if (ps5)
	{
		EXIT_NOT_IMPLEMENTED(vp.guard_band_horz_discard != 0.0f);
		EXIT_NOT_IMPLEMENTED(vp.guard_band_vert_discard != 0.0f);
	} else
	{
		EXIT_NOT_IMPLEMENTED(vp.guard_band_horz_discard != 1.000000);
		EXIT_NOT_IMPLEMENTED(vp.guard_band_vert_discard != 1.000000);
	}
	EXIT_NOT_IMPLEMENTED(vp.generic_scissor_window_offset_enable != false);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].viewport_scissor_left != 0);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].viewport_scissor_top != 0);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].viewport_scissor_right != 0);
	// EXIT_NOT_IMPLEMENTED(vp.viewports[0].viewport_scissor_bottom != 0);
	EXIT_NOT_IMPLEMENTED(viewport_scissor && vp.viewports[0].viewport_scissor_window_offset_enable != true);
}

static void hw_check(const HW::Context& hw)
{
	const auto& rt   = hw.GetRenderTarget(0);
	const auto& bc   = hw.GetBlendControl(0);
	const auto& bclr = hw.GetBlendColor();
	const auto& vp   = hw.GetScreenViewport();
	const auto& z    = hw.GetDepthRenderTarget();
	const auto& c    = hw.GetClipControl();
	const auto& rc   = hw.GetRenderControl();
	const auto& d    = hw.GetDepthControl();
	const auto& s    = hw.GetStencilControl();
	const auto& sm   = hw.GetStencilMask();
	const auto& mc   = hw.GetModeControl();
	const auto& eqaa = hw.GetEqaaControl();
	const auto& cc   = hw.GetColorControl();
	const auto& smc  = hw.GetScanModeControl();
	const auto& aa   = hw.GetAaSampleControl();
	const auto& ac   = hw.GetAaConfig();

	rt_check(rt);
	vp_check(vp, smc);
	z_check(z);
	clip_check(c);
	rc_check(rc);
	d_check(d, s, sm);
	mc_check(mc);
	bc_check(bc, bclr, cc);
	eqaa_check(eqaa);
	aa_check(aa, ac);

	EXIT_NOT_IMPLEMENTED(hw.GetRenderTargetMask() != 0xF && hw.GetRenderTargetMask() != 0x0);
	EXIT_NOT_IMPLEMENTED(hw.GetDepthClearValue() != 0.0f && hw.GetDepthClearValue() != 1.0f);
	// EXIT_NOT_IMPLEMENTED(hw.GetStencilClearValue() != 0);
}

static void hw_print(const HW::Context& hw)
{
	const auto& rt   = hw.GetRenderTarget(0);
	const auto& bc   = hw.GetBlendControl(0);
	const auto& bclr = hw.GetBlendColor();
	const auto& vp   = hw.GetScreenViewport();
	const auto& z    = hw.GetDepthRenderTarget();
	const auto& c    = hw.GetClipControl();
	const auto& rc   = hw.GetRenderControl();
	const auto& d    = hw.GetDepthControl();
	const auto& s    = hw.GetStencilControl();
	const auto& sm   = hw.GetStencilMask();
	const auto& mc   = hw.GetModeControl();
	const auto& eqaa = hw.GetEqaaControl();
	const auto& cc   = hw.GetColorControl();
	const auto& smc  = hw.GetScanModeControl();
	const auto& aa   = hw.GetAaSampleControl();
	const auto& ac   = hw.GetAaConfig();

	if (Kyty::Log::GetDirection() != Kyty::Log::Direction::Silent)
	{
		printf("Context\n");
		printf("\t GetRenderTargetMask()   = 0x%08" PRIx32 "\n", hw.GetRenderTargetMask());
		printf("\t GetDepthClearValue()    = %f\n", hw.GetDepthClearValue());
		printf("\t GetStencilClearValue()  = %" PRIu8 "\n", hw.GetStencilClearValue());
		printf("\t GetLineWidth()          = %f\n", hw.GetLineWidth());

		printf("%s", rt_print("RenderTraget:", rt).Concat(U"").C_Str());

		z_print("DepthRenderTraget:", z);
		vp_print("ScreenViewport:", vp, smc);
		clip_print("ClipControl:", c);
		rc_print("RenderControl:", rc);
		d_print("DepthStencilControlMask:", d, s, sm);
		mc_print("ModeControl:", mc);
		bc_print("BlendColorControl:", bc, bclr, cc);
		eqaa_print("EqaaControl:", eqaa);
		aa_print("AaSampleControl:", aa, ac);
	}
}

void GraphicsRenderInit()
{
	EXIT_IF(g_render_ctx != nullptr);

	g_render_ctx = new RenderContext;
}

void GraphicsRenderCreateContext()
{
	EXIT_IF(g_render_ctx == nullptr);

	g_render_ctx->SetGraphicCtx(WindowGetGraphicContext());
}

void RenderContext::AddEopEq(LibKernel::EventQueue::KernelEqueue eq)
{
	Core::LockGuard lock(m_eop_mutex);

	EXIT_NOT_IMPLEMENTED(m_eop_eqs.Contains(eq));

	m_eop_eqs.Add(eq);
}

void RenderContext::DeleteEopEq(LibKernel::EventQueue::KernelEqueue eq)
{
	Core::LockGuard lock(m_eop_mutex);

	auto index = m_eop_eqs.Find(eq);
	EXIT_NOT_IMPLEMENTED(!m_eop_eqs.IndexValid(index));
	m_eop_eqs[index] = nullptr;
}

void RenderContext::TriggerEopEvent()
{
	Core::LockGuard lock(m_eop_mutex);

	for (auto& eop_eq: m_eop_eqs)
	{
		if (eop_eq != nullptr)
		{
			auto result =
			    LibKernel::EventQueue::KernelTriggerEvent(eop_eq, GRAPHICS_EVENT_EOP, LibKernel::EventQueue::KERNEL_EVFILT_GRAPHICS,
			                                              reinterpret_cast<void*>(LibKernel::KernelReadTsc()));
			EXIT_NOT_IMPLEMENTED(result != OK);
		}
	}
}

void GdsBuffer::Init(GraphicContext* ctx)
{
	if (m_buffer == nullptr)
	{
		m_buffer = new VulkanBuffer;

		m_buffer->usage           = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		m_buffer->memory.property = static_cast<uint32_t>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
		                            VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		m_buffer->buffer = nullptr;

		VulkanCreateBuffer(ctx, DW_SIZE * 4, m_buffer);
		EXIT_NOT_IMPLEMENTED(m_buffer->buffer == nullptr);
	}
}

void GdsBuffer::Clear(GraphicContext* ctx, uint64_t dw_offset, uint32_t dw_num, uint32_t clear_value)
{
	EXIT_IF(ctx == nullptr);

	Core::LockGuard lock(m_mutex);

	Init(ctx);

	EXIT_NOT_IMPLEMENTED(dw_offset >= DW_SIZE);
	EXIT_NOT_IMPLEMENTED(dw_offset + dw_num > DW_SIZE);

	EXIT_IF(m_buffer == nullptr);

	void* data = nullptr;
	VulkanMapMemory(ctx, &m_buffer->memory, &data);

	EXIT_IF(data == nullptr);

	for (uint32_t i = 0; i < dw_num; i++)
	{
		static_cast<uint32_t*>(data)[dw_offset + i] = clear_value;
	}

	VulkanUnmapMemory(ctx, &m_buffer->memory);
}

void GdsBuffer::Read(GraphicContext* ctx, uint32_t* dst, uint32_t dw_offset, uint32_t dw_size)
{
	EXIT_IF(dst == nullptr);

	Core::LockGuard lock(m_mutex);

	Init(ctx);

	EXIT_NOT_IMPLEMENTED(dw_offset >= DW_SIZE);
	EXIT_NOT_IMPLEMENTED(dw_offset + dw_size > DW_SIZE);

	EXIT_IF(m_buffer == nullptr);

	void* data = nullptr;
	VulkanMapMemory(ctx, &m_buffer->memory, &data);

	EXIT_IF(data == nullptr);

	for (uint32_t i = 0; i < dw_size; i++)
	{
		dst[i] = static_cast<uint32_t*>(data)[dw_offset + i];
	}

	VulkanUnmapMemory(ctx, &m_buffer->memory);
}

VulkanBuffer* GdsBuffer::GetBuffer(GraphicContext* ctx)
{
	Core::LockGuard lock(m_mutex);

	Init(ctx);

	return m_buffer;
}

void CommandPool::Create(int id)
{
	auto* ctx = g_render_ctx->GetGraphicCtx();

	EXIT_IF(id < 0 || id >= GraphicContext::QUEUES_NUM);
	EXIT_IF(m_pool[id] != nullptr);

	m_pool[id] = new VulkanCommandPool;

	EXIT_IF(ctx == nullptr);
	EXIT_IF(ctx->device == nullptr);
	EXIT_IF(ctx->queues[id].family == static_cast<uint32_t>(-1));
	EXIT_IF(m_pool[id]->pool != nullptr);
	EXIT_IF(m_pool[id]->buffers != nullptr);
	EXIT_IF(m_pool[id]->fences != nullptr);
	EXIT_IF(m_pool[id]->semaphores != nullptr);
	EXIT_IF(m_pool[id]->buffers_count != 0);

	VkCommandPoolCreateInfo pool_info {};
	pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.pNext            = nullptr;
	pool_info.queueFamilyIndex = ctx->queues[id].family;
	pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	vkCreateCommandPool(ctx->device, &pool_info, nullptr, &m_pool[id]->pool);

	EXIT_NOT_IMPLEMENTED(m_pool[id]->pool == nullptr);

	m_pool[id]->buffers_count = 4;
	m_pool[id]->buffers       = new VkCommandBuffer[m_pool[id]->buffers_count];
	m_pool[id]->fences        = new VkFence[m_pool[id]->buffers_count];
	m_pool[id]->semaphores    = new VkSemaphore[m_pool[id]->buffers_count];
	m_pool[id]->busy          = new bool[m_pool[id]->buffers_count];

	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool        = m_pool[id]->pool;
	alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = m_pool[id]->buffers_count;

	if (vkAllocateCommandBuffers(ctx->device, &alloc_info, m_pool[id]->buffers) != VK_SUCCESS)
	{
		EXIT("Can't allocate command buffers");
	}

	for (uint32_t i = 0; i < m_pool[id]->buffers_count; i++)
	{
		m_pool[id]->busy[i] = false;

		VkFenceCreateInfo fence_info {};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.pNext = nullptr;
		fence_info.flags = 0;

		if (vkCreateFence(ctx->device, &fence_info, nullptr, &m_pool[id]->fences[i]) != VK_SUCCESS)
		{
			EXIT("Can't create fence");
		}

		VkSemaphoreCreateInfo semaphore_info {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphore_info.pNext = nullptr;
		semaphore_info.flags = 0;

		if (vkCreateSemaphore(ctx->device, &semaphore_info, nullptr, &m_pool[id]->semaphores[i]) != VK_SUCCESS)
		{
			EXIT("Can't create semaphore");
		}

		EXIT_IF(m_pool[id]->buffers[i] == nullptr);
		EXIT_IF(m_pool[id]->fences[i] == nullptr);
		EXIT_IF(m_pool[id]->semaphores[i] == nullptr);
	}
}

void CommandPool::DeleteAll()
{
	auto* ctx = g_render_ctx->GetGraphicCtx();

	for (auto& pool: m_pool)
	{
		if (pool != nullptr)
		{
			EXIT_IF(ctx == nullptr);
			EXIT_IF(ctx->device == nullptr);

			for (uint32_t i = 0; i < pool->buffers_count; i++)
			{
				vkDestroySemaphore(ctx->device, pool->semaphores[i], nullptr);
				vkDestroyFence(ctx->device, pool->fences[i], nullptr);
			}

			vkFreeCommandBuffers(ctx->device, pool->pool, pool->buffers_count, pool->buffers);

			vkDestroyCommandPool(ctx->device, pool->pool, nullptr);

			delete[] pool->semaphores;
			delete[] pool->fences;
			delete[] pool->buffers;
			delete[] pool->busy;

			delete pool;
			pool = nullptr;
		}
	}
}

VulkanFramebuffer* FramebufferCache::CreateFramebuffer(RenderColorInfo* color, RenderDepthInfo* depth)
{
	KYTY_PROFILER_FUNCTION();

	static std::atomic<uint64_t> seq = 0;

	Core::LockGuard lock(m_mutex);

	EXIT_IF(color == nullptr);
	EXIT_IF(depth == nullptr);
	EXIT_IF(g_render_ctx == nullptr);

	bool with_depth = (depth->format != VK_FORMAT_UNDEFINED && depth->vulkan_buffer != nullptr);
	bool with_color = (color->vulkan_buffer != nullptr);

	for (auto& f: m_framebuffers)
	{
		if (f.framebuffer != nullptr && f.image_id == (with_color ? color->vulkan_buffer->memory.unique_id : 0) &&
		    f.depth_id == (with_depth ? depth->vulkan_buffer->memory.unique_id : 0) && f.depth_clear_enable == depth->depth_clear_enable &&
		    f.stencil_clear_enable == depth->stencil_clear_enable)
		{
			return f.framebuffer;
		}
	}

	auto* framebuffer        = new VulkanFramebuffer;
	framebuffer->render_pass = nullptr;
	framebuffer->framebuffer = nullptr;

	auto* gctx = g_render_ctx->GetGraphicCtx();

	EXIT_IF(gctx == nullptr);

	EXIT_NOT_IMPLEMENTED(!with_depth && !with_color);
	EXIT_NOT_IMPLEMENTED(with_depth && with_color &&
	                     (color->vulkan_buffer->extent.width != depth->vulkan_buffer->extent.width ||
	                      color->vulkan_buffer->extent.height != depth->vulkan_buffer->extent.height));

	VulkanImage* vulkan_buffer =
	    (with_color ? color->vulkan_buffer
	                : CreateDummyBuffer(VK_FORMAT_B8G8R8A8_SRGB, depth->vulkan_buffer->extent.width, depth->vulkan_buffer->extent.height));

	VkAttachmentDescription attachments[2];
	attachments[0].flags          = 0;
	attachments[0].format         = vulkan_buffer->format;
	attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[1].flags          = 0;
	attachments[1].format         = depth->format;
	attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp         = (depth->depth_clear_enable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD);
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp  = (depth->stencil_clear_enable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD);
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref {};
	color_attachment_ref.attachment = (with_color ? 0 : VK_ATTACHMENT_UNUSED);
	color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass {};
	subpass.flags                   = 0;
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount    = 0;
	subpass.pInputAttachments       = nullptr;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &color_attachment_ref;
	subpass.pResolveAttachments     = nullptr;
	subpass.pDepthStencilAttachment = (with_depth ? &depth_attachment_ref : nullptr);
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments    = nullptr;

	VkRenderPassCreateInfo render_pass_info {};
	render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.pNext           = nullptr;
	render_pass_info.flags           = 0;
	render_pass_info.attachmentCount = (with_depth ? 2 : 1);
	render_pass_info.pAttachments    = attachments;
	render_pass_info.subpassCount    = 1;
	render_pass_info.pSubpasses      = &subpass;
	render_pass_info.dependencyCount = 0;
	render_pass_info.pDependencies   = nullptr;

	vkCreateRenderPass(gctx->device, &render_pass_info, nullptr, &framebuffer->render_pass);

	framebuffer->render_pass_id = ++seq;

	EXIT_NOT_IMPLEMENTED(framebuffer->render_pass == nullptr);

	VkImageView views[] = {vulkan_buffer->image_view[VulkanImage::VIEW_DEFAULT],
	                       (with_depth ? depth->vulkan_buffer->image_view[VulkanImage::VIEW_DEFAULT] : nullptr)};

	VkFramebufferCreateInfo framebuffer_info {};
	framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.pNext           = nullptr;
	framebuffer_info.flags           = 0;
	framebuffer_info.renderPass      = framebuffer->render_pass;
	framebuffer_info.attachmentCount = with_depth ? 2 : 1;
	framebuffer_info.pAttachments    = views;
	framebuffer_info.width           = vulkan_buffer->extent.width;
	framebuffer_info.height          = vulkan_buffer->extent.height;
	framebuffer_info.layers          = 1;

	vkCreateFramebuffer(gctx->device, &framebuffer_info, nullptr, &framebuffer->framebuffer);

	EXIT_NOT_IMPLEMENTED(framebuffer->framebuffer == nullptr);

	Framebuffer fnew;
	fnew.framebuffer          = framebuffer;
	fnew.image_id             = (with_color ? color->vulkan_buffer->memory.unique_id : 0);
	fnew.depth_id             = (with_depth ? depth->vulkan_buffer->memory.unique_id : 0);
	fnew.depth_clear_enable   = depth->depth_clear_enable;
	fnew.stencil_clear_enable = depth->stencil_clear_enable;

	bool updated = false;

	for (auto& f: m_framebuffers)
	{
		if (f.framebuffer == nullptr)
		{
			f       = fnew;
			updated = true;
			break;
		}
	}

	if (!updated)
	{
		m_framebuffers.Add(fnew);
	}

	return framebuffer;
}

void FramebufferCache::FreeFramebufferByColor(VulkanImage* image)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(image == nullptr);

	Core::LockGuard lock(m_mutex);

	for (auto& f: m_framebuffers)
	{
		if (f.framebuffer != nullptr && f.image_id == image->memory.unique_id)
		{
			g_render_ctx->GetPipelineCache()->DeletePipelines(f.framebuffer);

			auto* gctx = g_render_ctx->GetGraphicCtx();

			EXIT_IF(gctx == nullptr);

			vkDestroyFramebuffer(gctx->device, f.framebuffer->framebuffer, nullptr);

			vkDestroyRenderPass(gctx->device, f.framebuffer->render_pass, nullptr);

			delete f.framebuffer;

			f.framebuffer = nullptr;

			break;
		}
	}
}

void FramebufferCache::FreeFramebufferByDepth(DepthStencilVulkanImage* image)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(image == nullptr);

	Core::LockGuard lock(m_mutex);

	for (auto& f: m_framebuffers)
	{
		if (f.framebuffer != nullptr && f.depth_id == image->memory.unique_id)
		{
			g_render_ctx->GetPipelineCache()->DeletePipelines(f.framebuffer);

			auto* gctx = g_render_ctx->GetGraphicCtx();

			EXIT_IF(gctx == nullptr);

			vkDestroyFramebuffer(gctx->device, f.framebuffer->framebuffer, nullptr);

			vkDestroyRenderPass(gctx->device, f.framebuffer->render_pass, nullptr);

			delete f.framebuffer;

			f.framebuffer = nullptr;

			break;
		}
	}
}

VideoOutVulkanImage* FramebufferCache::CreateDummyBuffer(VkFormat format, uint32_t width, uint32_t height)
{
	for (auto* b: m_dummy_buffers)
	{
		if (b->extent.width == width && b->extent.height == height && b->format == format)
		{
			return b;
		}
	}

	auto* ctx = g_render_ctx->GetGraphicCtx();

	EXIT_IF(ctx == nullptr);

	auto* vk_obj = new VideoOutVulkanImage;

	vk_obj->extent.width  = width;
	vk_obj->extent.height = height;
	vk_obj->format        = format;
	vk_obj->image         = nullptr;
	vk_obj->layout        = VK_IMAGE_LAYOUT_UNDEFINED;

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
	image_info.initialLayout = vk_obj->layout;
	image_info.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples       = VK_SAMPLE_COUNT_1_BIT;

	vkCreateImage(ctx->device, &image_info, nullptr, &vk_obj->image);

	EXIT_NOT_IMPLEMENTED(vk_obj->image == nullptr);

	VulkanMemory mem;

	vkGetImageMemoryRequirements(ctx->device, vk_obj->image, &mem.requirements);

	mem.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	bool allocated = VulkanAllocate(ctx, &mem);

	EXIT_NOT_IMPLEMENTED(!allocated);

	VulkanBindImageMemory(ctx, vk_obj, &mem);

	vk_obj->memory = mem;

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

	EXIT_NOT_IMPLEMENTED(vk_obj->image_view[VulkanImage::VIEW_DEFAULT] == nullptr);

	UtilSetImageLayoutOptimal(vk_obj);

	m_dummy_buffers.Add(vk_obj);

	return vk_obj;
}

VkSampler SamplerCache::GetSampler(uint64_t id)
{
	Core::LockGuard lock(m_mutex);
	if (id < m_samplers.Size())
	{
		return m_samplers.At(id).vk;
	}
	return nullptr;
}

uint64_t SamplerCache::GetSamplerId(const ShaderSamplerResource& r)
{
	Core::LockGuard lock(m_mutex);
	uint32_t        m_samplers_size = m_samplers.Size();
	for (uint32_t i = 0; i < m_samplers_size; i++)
	{
		const auto& s = m_samplers.At(i);
		if (s.r.fields[0] == r.fields[0] && s.r.fields[1] == r.fields[1] && s.r.fields[2] == r.fields[2] && s.r.fields[3] == r.fields[3])
		{
			return i;
		}
	}
	Sampler s;
	s.r  = r;
	s.vk = nullptr;

	bool  aniso       = false;
	float aniso_ratio = 1.0f;
	auto  mag_filter  = r.XyMagFilter();
	auto  min_filter  = r.XyMinFilter();

	if (mag_filter == 2 || mag_filter == 3 || min_filter == 2 || min_filter == 3)
	{
		aniso = true;
		switch (r.MaxAnisoRatio())
		{
			case 0: aniso_ratio = 1.0f; break;
			case 1: aniso_ratio = 2.0f; break;
			case 2: aniso_ratio = 4.0f; break;
			case 3: aniso_ratio = 8.0f; break;
			case 4: aniso_ratio = 16.0f; break;
			default: EXIT("unknown ratio: %d\n", static_cast<int>(r.MaxAnisoRatio()));
		}
	}

	auto  mip_filter = r.MipFilter();
	float min_lod    = 0.0f;
	float max_lod    = 0.0f;
	if (mip_filter != 0)
	{
		min_lod = static_cast<float>(r.MinLod()) / 256.0f;
		max_lod = static_cast<float>(r.MaxLod()) / 256.0f;
	}

	VkSamplerCreateInfo sampler_info {};

	auto get_warp = [](uint8_t clamp)
	{
		switch (clamp)
		{
			case 0: return VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
			case 1: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
			case 2: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
			case 6: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; break;
			default: EXIT("unknown clamp: %u\n", clamp);
		}
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	};

	VkBorderColor border = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	switch (r.BorderColorType())
	{
		case 0: border = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK; break;
		case 1: border = VK_BORDER_COLOR_INT_OPAQUE_BLACK; break;
		case 2: border = VK_BORDER_COLOR_INT_OPAQUE_WHITE; break;
		default: EXIT("unknown border color: %d", static_cast<int>(r.BorderColorType()));
	}

	sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.pNext                   = nullptr;
	sampler_info.flags                   = 0;
	sampler_info.magFilter               = (mag_filter == 0 || mag_filter == 2 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR);
	sampler_info.minFilter               = (min_filter == 0 || min_filter == 2 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR);
	sampler_info.mipmapMode              = (mip_filter == 2 ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST);
	sampler_info.addressModeU            = get_warp(r.ClampX());
	sampler_info.addressModeV            = get_warp(r.ClampY());
	sampler_info.addressModeW            = get_warp(r.ClampZ());
	sampler_info.mipLodBias              = static_cast<float>(static_cast<int16_t>((r.LodBias() ^ 0x2000u) - 0x2000u)) / 256.0f;
	sampler_info.anisotropyEnable        = (aniso ? VK_TRUE : VK_FALSE);
	sampler_info.maxAnisotropy           = aniso_ratio;
	sampler_info.compareEnable           = VK_TRUE;
	sampler_info.compareOp               = static_cast<VkCompareOp>(r.DepthCompareFunc());
	sampler_info.minLod                  = min_lod;
	sampler_info.maxLod                  = max_lod;
	sampler_info.borderColor             = border;
	sampler_info.unnormalizedCoordinates = (r.ForceUnormCoords() ? VK_TRUE : VK_FALSE);

	vkCreateSampler(g_render_ctx->GetGraphicCtx()->device, &sampler_info, nullptr, &s.vk);
	EXIT_NOT_IMPLEMENTED(s.vk == nullptr);

	m_samplers.Add(s);
	return m_samplers_size;
}

static void get_input_format(const ShaderBufferResource& res, VkFormat* format, uint32_t* size, bool ps5)
{
	EXIT_IF(format == nullptr);
	EXIT_IF(size == nullptr);
	if (ps5)
	{
		auto fmt = res.Format();
		if (fmt == 74)
		{
			*format = VK_FORMAT_R32G32B32_SFLOAT;
			*size   = 3;
		} else if (fmt == 64)
		{
			*format = VK_FORMAT_R32G32_SFLOAT;
			*size   = 2;
		} else
		{
			EXIT("unknown format: fmt = %u\n", fmt);
			*format = VK_FORMAT_UNDEFINED;
			*size   = 4;
		}
	} else
	{
		auto nfmt = res.Nfmt();
		auto dfmt = res.Dfmt();
		if (nfmt == 7 && dfmt == 14)
		{
			*format = VK_FORMAT_R32G32B32A32_SFLOAT;
			*size   = 4;
		} else if (nfmt == 7 && dfmt == 13)
		{
			*format = VK_FORMAT_R32G32B32_SFLOAT;
			*size   = 3;
		} else if (nfmt == 7 && dfmt == 11)
		{
			*format = VK_FORMAT_R32G32_SFLOAT;
			*size   = 2;
		} else if (nfmt == 0 && dfmt == 10)
		{
			*format = VK_FORMAT_R8G8B8A8_UNORM;
			*size   = 4;
		} else
		{
			EXIT("unknown format: nfmt = %u, dfmt = %u\n", nfmt, dfmt);
			*format = VK_FORMAT_UNDEFINED;
			*size   = 4;
		}
	}
}

static VkBlendFactor get_blend_factor(uint32_t factor)
{
	switch (factor)
	{
		case /* Zero */ 0x00000000: return VK_BLEND_FACTOR_ZERO;
		case /* One */ 0x00000001: return VK_BLEND_FACTOR_ONE;
		case /* SrcColor */ 0x00000002: return VK_BLEND_FACTOR_SRC_COLOR;
		case /* OneMinusSrcColor */ 0x00000003: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case /* SrcAlpha */ 0x00000004: return VK_BLEND_FACTOR_SRC_ALPHA;
		case /* OneMinusSrcAlpha */ 0x00000005: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case /* DestAlpha */ 0x00000006: return VK_BLEND_FACTOR_DST_ALPHA;
		case /* OneMinusDestAlpha */ 0x00000007: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case /* DestColor */ 0x00000008: return VK_BLEND_FACTOR_DST_COLOR;
		case /* OneMinusDestColor */ 0x00000009: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case /* SrcAlphaSaturate */ 0x0000000a: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case /* ConstantColor */ 0x0000000d: return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case /* OneMinusConstantColor */ 0x0000000e: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case /* Src1Color */ 0x0000000f: return VK_BLEND_FACTOR_SRC1_COLOR;
		case /* InverseSrc1Color */ 0x00000010: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case /* Src1Alpha */ 0x00000011: return VK_BLEND_FACTOR_SRC1_ALPHA;
		case /* InverseSrc1Alpha */ 0x00000012: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		case /* ConstantAlpha */ 0x00000013: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case /* OneMinusConstantAlpha */ 0x00000014: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		default: EXIT("unknown factor: %u\n", factor);
	}
	return VK_BLEND_FACTOR_ZERO;
}

static VkBlendOp get_blend_op(uint32_t op)
{
	switch (op)
	{
		case /* Add */ 0x00000000: return VK_BLEND_OP_ADD;
		case /* Subtract */ 0x00000001: return VK_BLEND_OP_SUBTRACT;
		case /* Min */ 0x00000002: return VK_BLEND_OP_MIN;
		case /* Max */ 0x00000003: return VK_BLEND_OP_MAX;
		case /* ReverseSubtract */ 0x00000004: return VK_BLEND_OP_REVERSE_SUBTRACT;
		default: EXIT("unknown op: %u\n", op);
	}
	return VK_BLEND_OP_ADD;
}

static void CreateLayout(VkDescriptorSetLayout* set_layouts, uint32_t* set_layouts_num, VkPushConstantRange* push_constant_info,
                         uint32_t* push_constant_info_num, const ShaderBindResources& bind, VkShaderStageFlags vk_stage,
                         DescriptorCache::Stage stage)
{
	EXIT_IF(set_layouts == nullptr);
	EXIT_IF(set_layouts_num == nullptr);
	EXIT_IF(push_constant_info == nullptr);
	EXIT_IF(push_constant_info_num == nullptr);

	bool need_descriptor = (bind.storage_buffers.buffers_num > 0 || bind.textures2D.textures_num > 0 || bind.samplers.samplers_num > 0 ||
	                        bind.gds_pointers.pointers_num > 0);

	EXIT_IF(need_descriptor && bind.push_constant_size == 0);

	if (bind.push_constant_size != 0)
	{
		auto index = *push_constant_info_num;

		push_constant_info[index].stageFlags = vk_stage;
		push_constant_info[index].offset     = bind.push_constant_offset;
		push_constant_info[index].size       = bind.push_constant_size;
		(*push_constant_info_num)++;
	}

	if (need_descriptor)
	{
		EXIT_IF(bind.descriptor_set_slot != *set_layouts_num);

		set_layouts[*set_layouts_num] = g_render_ctx->GetDescriptorCache()->GetDescriptorSetLayout(stage, bind /*, bind_params*/);
		(*set_layouts_num)++;
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static VulkanPipeline* CreatePipelineInternal(VkRenderPass render_pass, const ShaderVertexInputInfo* vs_input_info,
                                              const Vector<uint32_t>& vs_shader, const ShaderPixelInputInfo* ps_input_info,
                                              const Vector<uint32_t>& ps_shader, const PipelineStaticParameters* static_params,
                                              PipelineDynamicParameters* dynamic_params)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(render_pass == nullptr);
	EXIT_IF(static_params == nullptr);
	EXIT_IF(dynamic_params == nullptr);

	auto* pipeline           = new VulkanPipeline;
	pipeline->static_params  = static_params;
	pipeline->dynamic_params = dynamic_params;

	auto* gctx = g_render_ctx->GetGraphicCtx();

	EXIT_IF(gctx == nullptr);

	VkShaderModule vert_shader_module = nullptr;
	VkShaderModule frag_shader_module = nullptr;

	VkShaderModuleCreateInfo create_info {};

	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	create_info.codeSize = static_cast<size_t>(vs_shader.Size()) * 4;
	create_info.pCode    = vs_shader.GetDataConst();
	vkCreateShaderModule(gctx->device, &create_info, nullptr, &vert_shader_module);

	create_info.codeSize = static_cast<size_t>(ps_shader.Size()) * 4;
	create_info.pCode    = ps_shader.GetDataConst();
	vkCreateShaderModule(gctx->device, &create_info, nullptr, &frag_shader_module);

	EXIT_NOT_IMPLEMENTED(vert_shader_module == nullptr);
	EXIT_NOT_IMPLEMENTED(frag_shader_module == nullptr);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info {};
	vert_shader_stage_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.pNext               = nullptr;
	vert_shader_stage_info.flags               = 0;
	vert_shader_stage_info.stage               = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module              = vert_shader_module;
	vert_shader_stage_info.pName               = "main";
	vert_shader_stage_info.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo frag_shader_stage_info {};
	frag_shader_stage_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.pNext               = nullptr;
	frag_shader_stage_info.flags               = 0;
	frag_shader_stage_info.stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module              = frag_shader_module;
	frag_shader_stage_info.pName               = "main";
	frag_shader_stage_info.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

	VkVertexInputAttributeDescription input_attr[ShaderVertexInputInfo::RES_MAX];
	VkVertexInputBindingDescription   input_desc[ShaderVertexInputInfo::RES_MAX];

	bool gen5 = Config::IsNextGen();

	for (int bi = 0; bi < vs_input_info->buffers_num; bi++)
	{
		const auto& b            = vs_input_info->buffers[bi];
		input_desc[bi].binding   = bi;
		input_desc[bi].stride    = b.stride;
		input_desc[bi].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		for (int ai = 0; ai < b.attr_num; ai++)
		{
			auto index                 = b.attr_indices[ai];
			input_attr[index].binding  = bi;
			input_attr[index].location = index;
			input_attr[index].offset   = b.attr_offsets[ai];

			uint32_t attr_size = 4;
			get_input_format(vs_input_info->resources[index], &input_attr[index].format, &attr_size, gen5);

			auto registers_num = vs_input_info->resources_dst[index].registers_num;

			if (gen5)
			{
				EXIT_NOT_IMPLEMENTED(vs_input_info->resources[index].OutOfBounds() != 0);
			}
			EXIT_NOT_IMPLEMENTED(vs_input_info->resources[index].AddTid());
			EXIT_NOT_IMPLEMENTED(vs_input_info->resources[index].SwizzleEnabled());

			switch (registers_num)
			{
				case 1:
				{
					auto swizzle = vs_input_info->resources[index].DstSelX();
					EXIT_NOT_IMPLEMENTED(swizzle != DstSel(4));
					break;
				}
				case 2:
				{
					auto swizzle = vs_input_info->resources[index].DstSelXY();
					EXIT_NOT_IMPLEMENTED(attr_size == 1 && swizzle != DstSel(4, 0));
					EXIT_NOT_IMPLEMENTED(attr_size >= 2 && swizzle != DstSel(4, 5));
					break;
				}
				case 3:
				{
					auto swizzle = vs_input_info->resources[index].DstSelXYZ();
					EXIT_NOT_IMPLEMENTED(attr_size == 1 && swizzle != DstSel(4, 0, 0));
					EXIT_NOT_IMPLEMENTED(attr_size == 2 && swizzle != DstSel(4, 5, 0));
					EXIT_NOT_IMPLEMENTED(attr_size >= 3 && swizzle != DstSel(4, 5, 6));
					break;
				}
				case 4:
				{
					auto swizzle = vs_input_info->resources[index].DstSelXYZW();
					EXIT_NOT_IMPLEMENTED(attr_size == 1 && swizzle != DstSel(4, 0, 0, 1));
					EXIT_NOT_IMPLEMENTED(attr_size == 2 && swizzle != DstSel(4, 5, 0, 1));
					EXIT_NOT_IMPLEMENTED(attr_size == 3 && swizzle != DstSel(4, 5, 6, 1));
					EXIT_NOT_IMPLEMENTED(attr_size == 4 && swizzle != DstSel(4, 5, 6, 7));
					break;
				}
				default: EXIT("invalid registers_num");
			}
		}
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_info {};
	vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.pNext                           = nullptr;
	vertex_input_info.flags                           = 0;
	vertex_input_info.vertexBindingDescriptionCount   = vs_input_info->buffers_num;
	vertex_input_info.pVertexBindingDescriptions      = input_desc;
	vertex_input_info.vertexAttributeDescriptionCount = vs_input_info->resources_num;
	vertex_input_info.pVertexAttributeDescriptions    = input_attr;

	VkPipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.pNext                  = nullptr;
	input_assembly.flags                  = 0;
	input_assembly.topology               = static_params->topology;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport {};
	viewport.x        = static_params->viewport_offset[0] - static_params->viewport_scale[0];
	viewport.y        = static_params->viewport_offset[1] - static_params->viewport_scale[1];
	viewport.width    = static_params->viewport_scale[0] * 2.0f;
	viewport.height   = static_params->viewport_scale[1] * 2.0f;
	viewport.minDepth = static_params->viewport_offset[2];
	viewport.maxDepth = static_params->viewport_scale[2] + static_params->viewport_offset[2];

	VkRect2D scissor {};
	scissor.offset = {static_params->scissor_ltrb[0], static_params->scissor_ltrb[1]};
	scissor.extent = {static_cast<uint32_t>(static_params->scissor_ltrb[2] - static_params->scissor_ltrb[0]),
	                  static_cast<uint32_t>(static_params->scissor_ltrb[3] - static_params->scissor_ltrb[1])};

	VkPipelineViewportStateCreateInfo viewport_state {};
	viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext         = nullptr;
	viewport_state.flags         = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports    = &viewport;
	viewport_state.scissorCount  = 1;
	viewport_state.pScissors     = &scissor;

	VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
	cull_mode |= (static_params->cull_back ? VK_CULL_MODE_BACK_BIT : 0u);
	cull_mode |= (static_params->cull_front ? VK_CULL_MODE_FRONT_BIT : 0u);

	VkFrontFace front_face = (static_params->face ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineRasterizationDepthClipStateCreateInfoEXT clip_ext {};
	clip_ext.sType           = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
	clip_ext.pNext           = nullptr;
	clip_ext.flags           = 0;
	clip_ext.depthClipEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizer {};
	rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.pNext                   = &clip_ext;
	rasterizer.flags                   = 0;
	rasterizer.depthClampEnable        = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode                = cull_mode;
	rasterizer.frontFace               = front_face;
	rasterizer.depthBiasEnable         = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp          = 0.0f;
	rasterizer.depthBiasSlopeFactor    = 0.0f;
	rasterizer.lineWidth               = dynamic_params->line_width;

	VkPipelineMultisampleStateCreateInfo multisampling {};
	multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext                 = nullptr;
	multisampling.flags                 = 0;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading      = 1.0f;
	multisampling.pSampleMask           = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable      = VK_FALSE;

	VkColorComponentFlags color_write_mask = 0;

	if (static_params->color_mask == 0xF)
	{
		color_write_mask =
		    static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_R_BIT) | static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_G_BIT) |
		    static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_B_BIT) | static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_A_BIT);
	} else if (static_params->color_mask == 0x0)
	{
		color_write_mask = 0;
	} else
	{
		EXIT("unknown mask: %u\n", static_params->color_mask);
	}

	VkPipelineColorBlendAttachmentState color_blend_attachment {};
	color_blend_attachment.colorWriteMask      = color_write_mask;
	color_blend_attachment.blendEnable         = static_params->blend_enable ? VK_TRUE : VK_FALSE;
	color_blend_attachment.srcColorBlendFactor = get_blend_factor(static_params->color_srcblend);
	color_blend_attachment.dstColorBlendFactor = get_blend_factor(static_params->color_destblend);
	color_blend_attachment.colorBlendOp        = get_blend_op(static_params->color_comb_fcn);
	color_blend_attachment.srcAlphaBlendFactor = (static_params->separate_alpha_blend ? get_blend_factor(static_params->alpha_srcblend)
	                                                                                  : color_blend_attachment.srcColorBlendFactor);
	color_blend_attachment.dstAlphaBlendFactor = (static_params->separate_alpha_blend ? get_blend_factor(static_params->alpha_destblend)
	                                                                                  : color_blend_attachment.dstColorBlendFactor);
	color_blend_attachment.alphaBlendOp =
	    (static_params->separate_alpha_blend ? get_blend_op(static_params->alpha_comb_fcn) : color_blend_attachment.colorBlendOp);

	VkBool32 color_write_enable = (pipeline->dynamic_params->color_write_enable ? VK_TRUE : VK_FALSE);

	VkPipelineColorWriteCreateInfoEXT color_write {};
	color_write.sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT;
	color_write.pNext              = nullptr;
	color_write.attachmentCount    = 1;
	color_write.pColorWriteEnables = &color_write_enable;

	VkPipelineColorBlendStateCreateInfo color_blending {};
	color_blending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.pNext             = &color_write;
	color_blending.flags             = 0;
	color_blending.logicOpEnable     = VK_FALSE;
	color_blending.logicOp           = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount   = 1;
	color_blending.pAttachments      = &color_blend_attachment;
	color_blending.blendConstants[0] = static_params->blend_color_red;
	color_blending.blendConstants[1] = static_params->blend_color_green;
	color_blending.blendConstants[2] = static_params->blend_color_blue;
	color_blending.blendConstants[3] = static_params->blend_color_alpha;

	VkDescriptorSetLayout set_layouts[2]  = {};
	uint32_t              set_layouts_num = 0;

	VkPushConstantRange push_constant_info[2];
	uint32_t            push_constant_info_num = 0;

	CreateLayout(set_layouts, &set_layouts_num, push_constant_info, &push_constant_info_num, vs_input_info->bind,
	             /*additional_params->vs_bind,*/ VK_SHADER_STAGE_VERTEX_BIT, DescriptorCache::Stage::Vertex);
	CreateLayout(set_layouts, &set_layouts_num, push_constant_info, &push_constant_info_num, ps_input_info->bind,
	             /*additional_params->ps_bind,*/ VK_SHADER_STAGE_FRAGMENT_BIT, DescriptorCache::Stage::Pixel);

	VkPipelineLayoutCreateInfo pipeline_layout_info {};
	pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext                  = nullptr;
	pipeline_layout_info.flags                  = 0;
	pipeline_layout_info.setLayoutCount         = set_layouts_num;
	pipeline_layout_info.pSetLayouts            = (set_layouts_num > 0 ? set_layouts : nullptr);
	pipeline_layout_info.pushConstantRangeCount = push_constant_info_num;
	pipeline_layout_info.pPushConstantRanges    = push_constant_info_num > 0 ? push_constant_info : nullptr;

	EXIT_IF(pipeline->pipeline_layout != nullptr);

	vkCreatePipelineLayout(gctx->device, &pipeline_layout_info, nullptr, &pipeline->pipeline_layout);

	EXIT_NOT_IMPLEMENTED(pipeline->pipeline_layout == nullptr);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_info {};
	depth_stencil_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info.pNext                 = nullptr;
	depth_stencil_info.flags                 = 0;
	depth_stencil_info.depthTestEnable       = (static_params->depth_test_enable ? VK_TRUE : VK_FALSE);
	depth_stencil_info.depthWriteEnable      = (static_params->depth_write_enable ? VK_TRUE : VK_FALSE);
	depth_stencil_info.depthCompareOp        = static_params->depth_compare_op;
	depth_stencil_info.depthBoundsTestEnable = (static_params->depth_bounds_test_enable ? VK_TRUE : VK_FALSE);
	depth_stencil_info.stencilTestEnable     = (static_params->stencil_test_enable ? VK_TRUE : VK_FALSE);
	depth_stencil_info.front.failOp          = static_params->stencil_front.failOp;
	depth_stencil_info.front.passOp          = static_params->stencil_front.passOp;
	depth_stencil_info.front.depthFailOp     = static_params->stencil_front.depthFailOp;
	depth_stencil_info.front.compareOp       = static_params->stencil_front.compareOp;
	depth_stencil_info.front.compareMask     = dynamic_params->stencil_front.compareMask;
	depth_stencil_info.front.writeMask       = dynamic_params->stencil_front.writeMask;
	depth_stencil_info.front.reference       = dynamic_params->stencil_front.reference;
	depth_stencil_info.back.failOp           = static_params->stencil_back.failOp;
	depth_stencil_info.back.passOp           = static_params->stencil_back.passOp;
	depth_stencil_info.back.depthFailOp      = static_params->stencil_back.depthFailOp;
	depth_stencil_info.back.compareOp        = static_params->stencil_back.compareOp;
	depth_stencil_info.back.compareMask      = dynamic_params->stencil_back.compareMask;
	depth_stencil_info.back.writeMask        = dynamic_params->stencil_back.writeMask;
	depth_stencil_info.back.reference        = dynamic_params->stencil_back.reference;
	depth_stencil_info.minDepthBounds        = static_params->depth_min_bounds;
	depth_stencil_info.maxDepthBounds        = static_params->depth_max_bounds;

	VkDynamicState dynamic_states[8]    = {};
	uint32_t       dynamic_states_count = 0;
	if (dynamic_params->vk_dynamic_state_line_width)
	{
		dynamic_states[dynamic_states_count++] = VK_DYNAMIC_STATE_LINE_WIDTH;
	}
	if (dynamic_params->vk_dynamic_state_stencil_compare_mask)
	{
		dynamic_states[dynamic_states_count++] = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
	}
	if (dynamic_params->vk_dynamic_state_stencil_reference)
	{
		dynamic_states[dynamic_states_count++] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
	}
	if (dynamic_params->vk_dynamic_state_stencil_write_mask)
	{
		dynamic_states[dynamic_states_count++] = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
	}
	if (dynamic_params->vk_dynamic_state_color_write_enable_ext)
	{
		dynamic_states[dynamic_states_count++] = VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT;
	}

	VkPipelineDynamicStateCreateInfo dynamic_state {};
	dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext             = nullptr;
	dynamic_state.flags             = 0;
	dynamic_state.dynamicStateCount = dynamic_states_count;
	dynamic_state.pDynamicStates    = dynamic_states;

	VkGraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.pNext               = nullptr;
	pipeline_info.flags               = 0;
	pipeline_info.stageCount          = 2;
	pipeline_info.pStages             = shader_stages;
	pipeline_info.pVertexInputState   = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pTessellationState  = nullptr;
	pipeline_info.pViewportState      = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState   = &multisampling;
	pipeline_info.pDepthStencilState  = (static_params->with_depth ? &depth_stencil_info : nullptr);
	pipeline_info.pColorBlendState    = &color_blending;
	pipeline_info.pDynamicState       = &dynamic_state;
	pipeline_info.layout              = pipeline->pipeline_layout;
	pipeline_info.renderPass          = render_pass;
	pipeline_info.subpass             = 0;
	pipeline_info.basePipelineHandle  = nullptr;
	pipeline_info.basePipelineIndex   = -1;

	EXIT_IF(pipeline->pipeline != nullptr);

	vkCreateGraphicsPipelines(gctx->device, nullptr, 1, &pipeline_info, nullptr, &pipeline->pipeline);

	EXIT_NOT_IMPLEMENTED(pipeline->pipeline == nullptr);

	vkDestroyShaderModule(gctx->device, frag_shader_module, nullptr);
	vkDestroyShaderModule(gctx->device, vert_shader_module, nullptr);

	return pipeline;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static VulkanPipeline* CreatePipelineInternal(const ShaderComputeInputInfo* input_info, const Vector<uint32_t>& cs_shader,
                                              const PipelineStaticParameters* static_params, PipelineDynamicParameters* dynamic_params)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(static_params == nullptr);
	EXIT_IF(dynamic_params == nullptr);

	auto* pipeline           = new VulkanPipeline;
	pipeline->static_params  = static_params;
	pipeline->dynamic_params = dynamic_params;

	auto* gctx = g_render_ctx->GetGraphicCtx();

	EXIT_IF(gctx == nullptr);

	VkShaderModule comp_shader_module = nullptr;

	VkShaderModuleCreateInfo create_info {};

	create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext    = nullptr;
	create_info.flags    = 0;
	create_info.codeSize = static_cast<size_t>(cs_shader.Size()) * 4;
	create_info.pCode    = cs_shader.GetDataConst();
	vkCreateShaderModule(gctx->device, &create_info, nullptr, &comp_shader_module);

	EXIT_NOT_IMPLEMENTED(comp_shader_module == nullptr);

	VkPipelineShaderStageCreateInfo comp_shader_stage_info {};
	comp_shader_stage_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	comp_shader_stage_info.pNext               = nullptr;
	comp_shader_stage_info.flags               = 0;
	comp_shader_stage_info.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
	comp_shader_stage_info.module              = comp_shader_module;
	comp_shader_stage_info.pName               = "main";
	comp_shader_stage_info.pSpecializationInfo = nullptr;

	VkDescriptorSetLayout set_layouts[1]  = {};
	uint32_t              set_layouts_num = 0;

	VkPushConstantRange push_constant_info[1];
	uint32_t            push_constant_info_num = 0;

	CreateLayout(set_layouts, &set_layouts_num, push_constant_info, &push_constant_info_num,
	             input_info->bind, /*additional_params->cs_bind,*/
	             VK_SHADER_STAGE_COMPUTE_BIT, DescriptorCache::Stage::Compute);

	VkPipelineLayoutCreateInfo pipeline_layout_info {};
	pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext                  = nullptr;
	pipeline_layout_info.flags                  = 0;
	pipeline_layout_info.setLayoutCount         = set_layouts_num;
	pipeline_layout_info.pSetLayouts            = (set_layouts_num > 0 ? set_layouts : nullptr);
	pipeline_layout_info.pushConstantRangeCount = push_constant_info_num;
	pipeline_layout_info.pPushConstantRanges    = push_constant_info_num > 0 ? push_constant_info : nullptr;

	EXIT_IF(pipeline->pipeline_layout != nullptr);

	vkCreatePipelineLayout(gctx->device, &pipeline_layout_info, nullptr, &pipeline->pipeline_layout);

	EXIT_NOT_IMPLEMENTED(pipeline->pipeline_layout == nullptr);

	VkComputePipelineCreateInfo info {};
	info.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.pNext              = nullptr;
	info.flags              = 0;
	info.stage              = comp_shader_stage_info;
	info.layout             = pipeline->pipeline_layout;
	info.basePipelineHandle = nullptr;
	info.basePipelineIndex  = -1;

	EXIT_IF(pipeline->pipeline != nullptr);

	vkCreateComputePipelines(gctx->device, nullptr, 1, &info, nullptr, &pipeline->pipeline);

	EXIT_NOT_IMPLEMENTED(pipeline->pipeline == nullptr);

	vkDestroyShaderModule(gctx->device, comp_shader_module, nullptr);

	return pipeline;
}

bool PipelineStaticParameters::operator==(const PipelineStaticParameters& other) const
{
	// NOLINTNEXTLINE(bugprone-suspicious-memory-comparison,cert-exp42-c,cert-flp37-c)
	return (0 == memcmp(this, &other, sizeof(struct PipelineStaticParameters)));
}

bool PipelineDynamicParameters::operator==(const PipelineDynamicParameters& other) const
{
	EXIT_IF(vk_dynamic_state_line_width != other.vk_dynamic_state_line_width ||
	        vk_dynamic_state_stencil_compare_mask != other.vk_dynamic_state_stencil_compare_mask ||
	        vk_dynamic_state_stencil_write_mask != other.vk_dynamic_state_stencil_write_mask ||
	        vk_dynamic_state_stencil_reference != other.vk_dynamic_state_stencil_reference ||
	        vk_dynamic_state_color_write_enable_ext != other.vk_dynamic_state_color_write_enable_ext);

	if (!vk_dynamic_state_line_width)
	{
		if (line_width != other.line_width)
		{
			return false;
		}
	}
	if (!vk_dynamic_state_stencil_compare_mask)
	{
		if (stencil_front.compareMask != other.stencil_front.compareMask || stencil_back.compareMask != other.stencil_back.compareMask)
		{
			return false;
		}
	}
	if (!vk_dynamic_state_stencil_write_mask)
	{
		if (stencil_front.writeMask != other.stencil_front.writeMask || stencil_back.writeMask != other.stencil_back.writeMask)
		{
			return false;
		}
	}
	if (!vk_dynamic_state_stencil_reference)
	{
		if (stencil_front.reference != other.stencil_front.reference || stencil_back.reference != other.stencil_back.reference)
		{
			return false;
		}
	}
	if (!vk_dynamic_state_color_write_enable_ext)
	{
		if (color_write_enable != other.color_write_enable)
		{
			return false;
		}
	}
	return true;
}

void PipelineCache::DeletePipelineInternal(uint32_t id)
{
	EXIT_NOT_IMPLEMENTED(!m_pipelines.IndexValid(id));

	Pipeline& p = m_pipelines[id];

	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(p.pipeline == nullptr);
	EXIT_IF(p.pipeline->pipeline == nullptr);
	EXIT_IF(p.pipeline->pipeline_layout == nullptr);
	EXIT_IF(p.static_params == nullptr);
	EXIT_IF(p.dynamic_params == nullptr);

	EXIT_IF(p.pipeline->static_params != p.static_params);
	EXIT_IF(p.pipeline->dynamic_params != p.dynamic_params);

	DumpPipeline("delete", id);

	delete p.static_params;
	delete p.dynamic_params;

	p.static_params  = nullptr;
	p.dynamic_params = nullptr;

	auto* gctx = g_render_ctx->GetGraphicCtx();

	EXIT_IF(gctx == nullptr);

	vkDestroyPipeline(gctx->device, p.pipeline->pipeline, nullptr);
	vkDestroyPipelineLayout(gctx->device, p.pipeline->pipeline_layout, nullptr);

	delete p.pipeline;

	p.pipeline = nullptr;
}

VulkanPipeline* PipelineCache::Find(const Pipeline& p) const
{
	for (const auto& pn: m_pipelines)
	{
		if (pn.pipeline != nullptr && p.render_pass_id == pn.render_pass_id && p.vs_shader_id == pn.vs_shader_id &&
		    p.ps_shader_id == pn.ps_shader_id && p.cs_shader_id == pn.cs_shader_id && *p.static_params == *pn.static_params &&
		    *p.dynamic_params == *pn.dynamic_params)
		{
			return pn.pipeline;
		}
	}
	return nullptr;
}

VulkanPipeline* PipelineCache::CreatePipeline(VulkanFramebuffer* framebuffer, RenderColorInfo* color, RenderDepthInfo* depth,
                                              const ShaderVertexInputInfo* vs_input_info, HW::Context* ctx, HW::Shader* sh_ctx,
                                              const ShaderPixelInputInfo* ps_input_info, VkPrimitiveTopology topology)
{
	KYTY_PROFILER_BLOCK("PipelineCache::CreatePipeline(Gfx)", profiler::colors::DeepOrangeA200);

	EXIT_IF(framebuffer == nullptr);
	EXIT_IF(depth == nullptr);
	EXIT_IF(color == nullptr);

	Core::LockGuard lock(m_mutex);

	const auto&               vs_regs    = sh_ctx->GetVs();
	const auto&               ps_regs    = sh_ctx->GetPs();
	const auto&               sh_regs    = ctx->GetShaderRegisters();
	const HW::BlendColor&     bclr       = ctx->GetBlendColor();
	const HW::ScreenViewport& vp         = ctx->GetScreenViewport();
	const auto&               bc         = ctx->GetBlendControl(0);
	uint32_t                  color_mask = ctx->GetRenderTargetMask();
	const HW::ModeControl&    mc         = ctx->GetModeControl();
	const auto&               cc         = ctx->GetColorControl();

	if (Config::GetPrintfDirection() != Log::Direction::Silent)
	{
		ShaderDbgDumpInputInfo(vs_input_info);
		ShaderDbgDumpInputInfo(ps_input_info);
	}

	auto vs_id = ShaderGetIdVS(&vs_regs, vs_input_info);
	auto ps_id = ShaderGetIdPS(&ps_regs, ps_input_info);

	Pipeline p {};
	p.render_pass_id = framebuffer->render_pass_id;
	p.ps_shader_id   = ps_id;
	p.vs_shader_id   = vs_id;
	p.static_params  = new PipelineStaticParameters;
	p.dynamic_params = new PipelineDynamicParameters;

	p.dynamic_params->vk_dynamic_state_line_width           = true;
	p.dynamic_params->vk_dynamic_state_stencil_compare_mask = true;
	p.dynamic_params->vk_dynamic_state_stencil_reference    = true;
	p.dynamic_params->vk_dynamic_state_stencil_write_mask   = true;
	p.dynamic_params->color_write_enable                    = true;

	EXIT_NOT_IMPLEMENTED(depth->depth_test_enable && ps_input_info->ps_execute_on_noop);

	bool generic_scissor =
	    (vp.generic_scissor_left != 0 || vp.generic_scissor_top != 0 || vp.generic_scissor_right != 0 || vp.generic_scissor_bottom != 0);
	bool viewport_scissor = (vp.viewports[0].viewport_scissor_left != 0 || vp.viewports[0].viewport_scissor_top != 0 ||
	                         vp.viewports[0].viewport_scissor_right != 0 || vp.viewports[0].viewport_scissor_bottom != 0);

	p.static_params->viewport_scale[0]  = vp.viewports[0].xscale;
	p.static_params->viewport_scale[1]  = vp.viewports[0].yscale;
	p.static_params->viewport_scale[2]  = vp.viewports[0].zscale;
	p.static_params->viewport_offset[0] = vp.viewports[0].xoffset;
	p.static_params->viewport_offset[1] = vp.viewports[0].yoffset;
	p.static_params->viewport_offset[2] = vp.viewports[0].zoffset;
	p.static_params->scissor_ltrb[0] =
	    (viewport_scissor ? vp.viewports[0].viewport_scissor_left : (generic_scissor ? vp.generic_scissor_left : vp.screen_scissor_left));
	p.static_params->scissor_ltrb[1] =
	    (viewport_scissor ? vp.viewports[0].viewport_scissor_top : (generic_scissor ? vp.generic_scissor_top : vp.screen_scissor_top));
	p.static_params->scissor_ltrb[2]          = (viewport_scissor ? vp.viewports[0].viewport_scissor_right
	                                                              : (generic_scissor ? vp.generic_scissor_right : vp.screen_scissor_right));
	p.static_params->scissor_ltrb[3]          = (viewport_scissor ? vp.viewports[0].viewport_scissor_bottom
	                                                              : (generic_scissor ? vp.generic_scissor_bottom : vp.screen_scissor_bottom));
	p.static_params->topology                 = topology;
	p.static_params->with_depth               = (depth->format != VK_FORMAT_UNDEFINED && depth->vulkan_buffer != nullptr);
	p.static_params->depth_test_enable        = depth->depth_test_enable;
	p.static_params->depth_write_enable       = (depth->depth_write_enable && !depth->depth_clear_enable);
	p.static_params->depth_compare_op         = depth->depth_compare_op;
	p.static_params->depth_bounds_test_enable = depth->depth_bounds_test_enable;
	p.static_params->depth_min_bounds         = depth->depth_min_bounds;
	p.static_params->depth_max_bounds         = depth->depth_max_bounds;
	p.static_params->stencil_test_enable      = depth->stencil_test_enable;
	p.static_params->stencil_front            = depth->stencil_static_front;
	p.static_params->stencil_back             = depth->stencil_static_back;
	p.static_params->color_mask               = color_mask;
	p.static_params->cull_back                = mc.cull_back;
	p.static_params->cull_front               = mc.cull_front;
	p.static_params->face                     = mc.face;
	p.static_params->color_srcblend           = bc.color_srcblend;
	p.static_params->color_comb_fcn           = bc.color_comb_fcn;
	p.static_params->color_destblend          = bc.color_destblend;
	p.static_params->alpha_srcblend           = bc.alpha_srcblend;
	p.static_params->alpha_comb_fcn           = bc.alpha_comb_fcn;
	p.static_params->alpha_destblend          = bc.alpha_destblend;
	p.static_params->separate_alpha_blend     = bc.separate_alpha_blend;
	p.static_params->blend_enable             = bc.enable;
	p.static_params->blend_color_red          = bclr.red;
	p.static_params->blend_color_green        = bclr.green;
	p.static_params->blend_color_blue         = bclr.blue;
	p.static_params->blend_color_alpha        = bclr.alpha;

	p.dynamic_params->line_width         = ctx->GetLineWidth();
	p.dynamic_params->stencil_front      = depth->stencil_dynamic_front;
	p.dynamic_params->stencil_back       = depth->stencil_dynamic_back;
	p.dynamic_params->color_write_enable = (cc.mode == 1);

	auto* found = Find(p);

	if (found != nullptr)
	{
		*found->dynamic_params = *p.dynamic_params;
		delete p.static_params;
		delete p.dynamic_params;
		return found;
	}

	auto vs_code = ShaderParseVS(&vs_regs, &sh_regs);
	auto ps_code = ShaderParsePS(&ps_regs, &sh_regs);

	auto vs_shader = ShaderRecompileVS(vs_code, vs_input_info);
	auto ps_shader = ShaderRecompilePS(ps_code, ps_input_info);

	EXIT_IF(vs_shader.IsEmpty());
	EXIT_IF(ps_shader.IsEmpty());

	p.pipeline = CreatePipelineInternal(framebuffer->render_pass, vs_input_info, vs_shader, ps_input_info, ps_shader, p.static_params,
	                                    p.dynamic_params);

	EXIT_NOT_IMPLEMENTED(p.pipeline == nullptr);

	bool updated = false;
	int  index   = 0;
	for (auto& pn: m_pipelines)
	{
		if (pn.pipeline == nullptr)
		{
			pn      = p;
			updated = true;

			DumpPipeline("create", index);

			break;
		}
		index++;
	}

	if (!updated)
	{
		if (m_pipelines.Size() >= PipelineCache::MAX_PIPELINES)
		{
			EXIT_NOT_IMPLEMENTED(m_pipelines.Size() >= PipelineCache::MAX_PIPELINES);
			//			auto  index = Math::Rand::UintInclusiveRange(0, m_pipelines.Size() - 1);
			//			auto& pn    = m_pipelines[index];
			//			DeletePipelineInternal(index);
			//			pn = p;
		} else
		{
			EXIT_IF(m_pipelines.Size() != index);

			m_pipelines.Add(p);

			DumpPipeline("create", index);
		}
	}

	return p.pipeline;
}

VulkanPipeline* PipelineCache::CreatePipeline(const ShaderComputeInputInfo* input_info, const HW::ComputeShaderInfo* cs_regs,
                                              const HW::ShaderRegisters* sh_regs)
{
	KYTY_PROFILER_BLOCK("PipelineCache::CreatePipeline(Compute)", profiler::colors::RedA100);

	EXIT_IF(cs_regs == nullptr);
	EXIT_IF(sh_regs == nullptr);

	Core::LockGuard lock(m_mutex);

	ShaderDbgDumpInputInfo(input_info);

	auto cs_id = ShaderGetIdCS(cs_regs, input_info);

	Pipeline p {};
	p.cs_shader_id   = cs_id;
	p.static_params  = new PipelineStaticParameters;
	p.dynamic_params = new PipelineDynamicParameters;

	p.dynamic_params->vk_dynamic_state_line_width           = true;
	p.dynamic_params->vk_dynamic_state_stencil_compare_mask = true;
	p.dynamic_params->vk_dynamic_state_stencil_reference    = true;
	p.dynamic_params->vk_dynamic_state_stencil_write_mask   = true;
	p.dynamic_params->color_write_enable                    = true;

	auto* found = Find(p);

	if (found != nullptr)
	{
		*found->dynamic_params = *p.dynamic_params;
		delete p.static_params;
		delete p.dynamic_params;
		return found;
	}

	auto cs_code = ShaderParseCS(cs_regs, sh_regs);

	auto cs_shader = ShaderRecompileCS(cs_code, input_info);
	EXIT_IF(cs_shader.IsEmpty());

	p.pipeline = CreatePipelineInternal(input_info, cs_shader, p.static_params, p.dynamic_params /*, params2*/);

	EXIT_NOT_IMPLEMENTED(p.pipeline == nullptr);

	bool updated = false;
	for (auto& pn: m_pipelines)
	{
		if (pn.pipeline == nullptr)
		{
			pn      = p;
			updated = true;
			break;
		}
	}

	if (!updated)
	{
		if (m_pipelines.Size() >= PipelineCache::MAX_PIPELINES)
		{
			EXIT_NOT_IMPLEMENTED(m_pipelines.Size() >= PipelineCache::MAX_PIPELINES);
			//			auto  index = Math::Rand::UintInclusiveRange(0, m_pipelines.Size() - 1);
			//			auto& pn    = m_pipelines[index];
			//			DeletePipelineInternal(index);
			//			pn = p;
		} else
		{
			m_pipelines.Add(p);
		}
	}

	return p.pipeline;
}

void PipelineCache::DeletePipeline(VulkanPipeline* pipeline)
{
	Core::LockGuard lock(m_mutex);

	auto index = m_pipelines.Find(pipeline, [](auto r, auto p) { return p == r.pipeline; });

	EXIT_IF(!m_pipelines.IndexValid(index));

	if (m_pipelines.IndexValid(index))
	{
		DeletePipelineInternal(index);
	}
}

void PipelineCache::DeletePipelines(VulkanFramebuffer* framebuffer)
{
	EXIT_IF(framebuffer == nullptr);

	Core::LockGuard lock(m_mutex);

	int index = 0;
	for (auto& p: m_pipelines)
	{
		if (p.pipeline != nullptr && p.render_pass_id == framebuffer->render_pass_id)
		{
			DeletePipelineInternal(index);
		}
		index++;
	}
}

void PipelineCache::DeleteAllPipelines()
{
	Core::LockGuard lock(m_mutex);

	for (uint32_t index = 0; index < m_pipelines.Size(); index++)
	{
		DeletePipelineInternal(index);
	}
}

void PipelineCache::DumpToFile(Core::File* f, const Pipeline& p) {}

void PipelineCache::DumpPipeline(const char* action, uint32_t id)
{
	EXIT_IF(!m_pipelines.IndexValid(id));
	EXIT_IF(action == nullptr);

	static std::atomic_int dump_id = 0;

	if (Config::PipelineDumpEnabled())
	{
		Core::File f;
		String     file_name = Config::GetPipelineDumpFolder().FixDirectorySlash() +
		                   String::FromPrintf("%04d_%04d_pipeline_%u_%s.log", GraphicsRunGetFrameNum(), dump_id++, id, action);
		Core::File::CreateDirectories(file_name.DirectoryWithoutFilename());
		f.Create(file_name);
		if (f.IsInvalid())
		{
			printf(FG_BRIGHT_RED "Can't create file: %s\n" FG_DEFAULT, file_name.C_Str());
			return;
		}
		Pipeline& p = m_pipelines[id];
		DumpToFile(&f, p);
		f.Close();
	}
}

static void create_layout(GraphicContext* gctx, int storage_buffers_num, int textures2d_sampled_num, int textures2d_storage_num,
                          int samplers_num, int gds_buffers_num, VkShaderStageFlags stage, VkDescriptorSetLayout* dst)
{
	uint32_t binding_num = 0;

	ShaderBindResources tmp {};
	tmp.storage_buffers.buffers_num = storage_buffers_num;
	tmp.textures2D.textures_num     = textures2d_sampled_num + textures2d_storage_num;
	tmp.samplers.samplers_num       = samplers_num;
	tmp.gds_pointers.pointers_num   = gds_buffers_num;

	ShaderCalcBindingIndices(&tmp);

	constexpr uint32_t B_MAX = 5;

	VkDescriptorSetLayoutBinding ubo_layout_binding[B_MAX] = {};

	if (storage_buffers_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		ubo_layout_binding[binding_num].binding            = tmp.storage_buffers.binding_index;
		ubo_layout_binding[binding_num].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ubo_layout_binding[binding_num].descriptorCount    = storage_buffers_num;
		ubo_layout_binding[binding_num].stageFlags         = stage;
		ubo_layout_binding[binding_num].pImmutableSamplers = nullptr;
		binding_num++;
	}

	if (textures2d_sampled_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		ubo_layout_binding[binding_num].binding            = tmp.textures2D.binding_sampled_index;
		ubo_layout_binding[binding_num].descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		ubo_layout_binding[binding_num].descriptorCount    = textures2d_sampled_num;
		ubo_layout_binding[binding_num].stageFlags         = stage;
		ubo_layout_binding[binding_num].pImmutableSamplers = nullptr;
		binding_num++;
	}

	if (textures2d_storage_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		ubo_layout_binding[binding_num].binding            = tmp.textures2D.binding_storage_index;
		ubo_layout_binding[binding_num].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		ubo_layout_binding[binding_num].descriptorCount    = textures2d_storage_num;
		ubo_layout_binding[binding_num].stageFlags         = stage;
		ubo_layout_binding[binding_num].pImmutableSamplers = nullptr;
		binding_num++;
	}

	if (samplers_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		ubo_layout_binding[binding_num].binding            = tmp.samplers.binding_index;
		ubo_layout_binding[binding_num].descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLER;
		ubo_layout_binding[binding_num].descriptorCount    = samplers_num;
		ubo_layout_binding[binding_num].stageFlags         = stage;
		ubo_layout_binding[binding_num].pImmutableSamplers = nullptr;
		binding_num++;
	}

	if (gds_buffers_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		ubo_layout_binding[binding_num].binding            = tmp.gds_pointers.binding_index;
		ubo_layout_binding[binding_num].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ubo_layout_binding[binding_num].descriptorCount    = gds_buffers_num;
		ubo_layout_binding[binding_num].stageFlags         = stage;
		ubo_layout_binding[binding_num].pImmutableSamplers = nullptr;
		binding_num++;
	}

	if (binding_num > 0)
	{
		VkDescriptorSetLayoutCreateInfo layout_info {};
		layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.pNext        = nullptr;
		layout_info.flags        = 0;
		layout_info.bindingCount = binding_num;
		layout_info.pBindings    = ubo_layout_binding;

		EXIT_IF(*dst != nullptr);

		vkCreateDescriptorSetLayout(gctx->device, &layout_info, nullptr, dst);

		EXIT_NOT_IMPLEMENTED(*dst == nullptr);
	} else
	{
		*dst = nullptr;
	}
};

void DescriptorCache::Init()
{
	if (m_initialized)
	{
		return;
	}

	auto* gctx = g_render_ctx->GetGraphicCtx();
	EXIT_IF(gctx == nullptr);

	EXIT_IF(m_descriptor_set_layout_vertex[0][0][0][0][0] != nullptr);
	EXIT_IF(m_descriptor_set_layout_pixel[0][0][0][0][0] != nullptr);
	EXIT_IF(m_descriptor_set_layout_compute[0][0][0][0][0] != nullptr);

	for (int buffers_num_i = 0; buffers_num_i <= BUFFERS_MAX; buffers_num_i++)
	{
		for (int buffers_num_j = 0; buffers_num_j <= TEXTURES_SAMPLED_MAX; buffers_num_j++)
		{
			for (int buffers_num_j2 = 0; buffers_num_j2 <= TEXTURES_STORAGE_MAX; buffers_num_j2++)
			{
				for (int buffers_num_k = 0; buffers_num_k <= SAMPLERS_MAX; buffers_num_k++)
				{
					for (int buffers_num_l = 0; buffers_num_l <= GDS_BUFFER_MAX; buffers_num_l++)
					{
						create_layout(
						    gctx, buffers_num_i, buffers_num_j, buffers_num_j2, buffers_num_k, buffers_num_l, VK_SHADER_STAGE_FRAGMENT_BIT,
						    &m_descriptor_set_layout_pixel[buffers_num_i][buffers_num_j][buffers_num_j2][buffers_num_k][buffers_num_l]);
						create_layout(
						    gctx, buffers_num_i, buffers_num_j, buffers_num_j2, buffers_num_k, buffers_num_l, VK_SHADER_STAGE_VERTEX_BIT,
						    &m_descriptor_set_layout_vertex[buffers_num_i][buffers_num_j][buffers_num_j2][buffers_num_k][buffers_num_l]);
						create_layout(
						    gctx, buffers_num_i, buffers_num_j, buffers_num_j2, buffers_num_k, buffers_num_l, VK_SHADER_STAGE_COMPUTE_BIT,
						    &m_descriptor_set_layout_compute[buffers_num_i][buffers_num_j][buffers_num_j2][buffers_num_k][buffers_num_l]);
					}
				}
			}
		}
	}

	m_initialized = true;
}

void DescriptorCache::CreatePool()
{
	KYTY_PROFILER_BLOCK("DescriptorCache::CreatePool");

	auto* gctx = g_render_ctx->GetGraphicCtx();
	EXIT_IF(gctx == nullptr);

	static const uint32_t max_sets = 512;

	VkDescriptorPoolSize pool_size[4];
	pool_size[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size[0].descriptorCount = max_sets * (BUFFERS_MAX + GDS_BUFFER_MAX);
	pool_size[1].type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	pool_size[1].descriptorCount = max_sets * TEXTURES_SAMPLED_MAX;
	pool_size[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pool_size[2].descriptorCount = max_sets * TEXTURES_STORAGE_MAX;
	pool_size[3].type            = VK_DESCRIPTOR_TYPE_SAMPLER;
	pool_size[3].descriptorCount = max_sets * SAMPLERS_MAX;

	VkDescriptorPoolCreateInfo pool_info {};
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.pNext         = nullptr;
	pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.poolSizeCount = 4;
	pool_info.pPoolSizes    = pool_size;
	pool_info.maxSets       = max_sets;

	Pool pool {};

	vkCreateDescriptorPool(gctx->device, &pool_info, nullptr, &pool.pool);

	EXIT_NOT_IMPLEMENTED(pool.pool == nullptr);

	pool.free           = true;
	pool.next_free_pool = m_first_free_pool;
	m_first_free_pool   = static_cast<int>(m_pools.Size());

	m_pools.Add(pool);
}

VulkanDescriptorSet* DescriptorCache::Allocate(Stage stage, int storage_buffers_num, int textures2d_sampled_num, int textures2d_storage_num,
                                               int samplers_num, int gds_buffers_num)
{
	KYTY_PROFILER_BLOCK("DescriptorCache::Allocate");

	EXIT_IF(storage_buffers_num < 0 || storage_buffers_num > BUFFERS_MAX);
	EXIT_IF(textures2d_sampled_num < 0 || textures2d_sampled_num > TEXTURES_SAMPLED_MAX);
	EXIT_IF(textures2d_storage_num < 0 || textures2d_storage_num > TEXTURES_STORAGE_MAX);
	EXIT_IF(samplers_num < 0 || samplers_num > SAMPLERS_MAX);
	EXIT_IF(gds_buffers_num < 0 || gds_buffers_num > GDS_BUFFER_MAX);

	Core::LockGuard lock(m_mutex);

	auto* gctx = g_render_ctx->GetGraphicCtx();
	EXIT_IF(gctx == nullptr);

	Init();

	auto* ret = new VulkanDescriptorSet;

	ret->set = nullptr;

	for (int try_num = 0; try_num < 2; try_num++)
	{
		int pool_id = m_first_free_pool;
		while (pool_id != -1)
		{
			auto& pool = m_pools[pool_id];

			VkDescriptorSetAllocateInfo alloc_info {};
			alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.pNext              = nullptr;
			alloc_info.descriptorPool     = pool.pool;
			alloc_info.descriptorSetCount = 1;

			switch (stage)
			{
				case Stage::Vertex:
					alloc_info.pSetLayouts = &m_descriptor_set_layout_vertex[storage_buffers_num][textures2d_sampled_num]
					                                                        [textures2d_storage_num][samplers_num][gds_buffers_num];
					break;
				case Stage::Pixel:
					alloc_info.pSetLayouts = &m_descriptor_set_layout_pixel[storage_buffers_num][textures2d_sampled_num]
					                                                       [textures2d_storage_num][samplers_num][gds_buffers_num];
					break;
				case Stage::Compute:
					alloc_info.pSetLayouts = &m_descriptor_set_layout_compute[storage_buffers_num][textures2d_sampled_num]
					                                                         [textures2d_storage_num][samplers_num][gds_buffers_num];
					break;
				default: EXIT("unknown stage\n");
			}

			ret->pool_id = pool_id;
			ret->layout  = *alloc_info.pSetLayouts;

			EXIT_IF(!pool.free);

			{
				KYTY_PROFILER_BLOCK("vkAllocateDescriptorSets");

				auto result = vkAllocateDescriptorSets(gctx->device, &alloc_info, &ret->set);

				if (result == VK_SUCCESS)
				{
					return ret;
				}
			}

			pool.free         = false;
			m_first_free_pool = pool.next_free_pool;
			pool_id           = pool.next_free_pool;
		}

		CreatePool();
	}

	delete ret;
	return nullptr;
}

void DescriptorCache::Free(VulkanDescriptorSet* set)
{
	EXIT_IF(set == nullptr);

	Core::LockGuard lock(m_mutex);

	auto* gctx = g_render_ctx->GetGraphicCtx();
	EXIT_IF(gctx == nullptr);
	EXIT_IF(!m_pools.IndexValid(set->pool_id));

	auto& pool = m_pools[set->pool_id];

	vkFreeDescriptorSets(gctx->device, pool.pool, 1, &set->set);

	if (!pool.free)
	{
		pool.free           = true;
		pool.next_free_pool = m_first_free_pool;
		m_first_free_pool   = set->pool_id;
	}

	delete set;
}

uint32_t DescriptorCache::CalcHash(const Set& s)
{
	uint32_t hash = 0;
	hash += Core::hash8(static_cast<uint8_t>(s.stage));
	hash ^= Core::hash8(static_cast<uint8_t>(s.storage_buffers_num));
	hash += Core::hash8(static_cast<uint8_t>(s.textures2d_sampled_num));
	hash ^= Core::hash8(static_cast<uint8_t>(s.textures2d_storage_num));
	hash += Core::hash8(static_cast<uint8_t>(s.samplers_num));
	hash ^= Core::hash8(static_cast<uint8_t>(s.gds_buffers_num));
	for (int i = 0; i < s.storage_buffers_num; i++)
	{
		hash += Core::hash64(s.storage_buffers_id[i]);
	}
	for (int i = 0; i < s.textures2d_sampled_num; i++)
	{
		hash ^= Core::hash64(s.textures2d_sampled_id[i]);
	}
	for (int i = 0; i < s.textures2d_storage_num; i++)
	{
		hash += Core::hash64(s.textures2d_storage_id[i]);
	}
	for (int i = 0; i < s.samplers_num; i++)
	{
		hash ^= Core::hash64(s.samplers_id[i]);
	}
	for (int i = 0; i < s.gds_buffers_num; i++)
	{
		hash += Core::hash64(s.gds_buffers_id[i]);
	}
	return hash;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
VulkanDescriptorSet* DescriptorCache::FindSet(const Set& s)
{
	const auto* list = m_sets_map.Find(s.hash);
	if (list != nullptr)
	{
		for (int index: *list)
		{
			auto& set = m_sets[index];

			if (set.set != nullptr && set.stage == s.stage && set.storage_buffers_num == s.storage_buffers_num &&
			    set.textures2d_sampled_num == s.textures2d_sampled_num && set.textures2d_storage_num == s.textures2d_storage_num &&
			    set.samplers_num == s.samplers_num && set.gds_buffers_num == s.gds_buffers_num)
			{
				bool match = true;
				for (int i = 0; i < s.storage_buffers_num; i++)
				{
					if (match && s.storage_buffers_id[i] != set.storage_buffers_id[i])
					{
						match = false;
						break;
					}
				}
				if (match)
				{
					for (int i = 0; i < s.textures2d_sampled_num; i++)
					{
						if (s.textures2d_sampled_id[i] != set.textures2d_sampled_id[i])
						{
							match = false;
							break;
						}
					}
				}
				if (match)
				{
					for (int i = 0; i < s.textures2d_storage_num; i++)
					{
						if (s.textures2d_storage_id[i] != set.textures2d_storage_id[i])
						{
							match = false;
							break;
						}
					}
				}
				if (match)
				{
					for (int i = 0; i < s.samplers_num; i++)
					{
						if (s.samplers_id[i] != set.samplers_id[i])
						{
							match = false;
							break;
						}
					}
				}
				if (match)
				{
					for (int i = 0; i < s.gds_buffers_num; i++)
					{
						if (s.gds_buffers_id[i] != set.gds_buffers_id[i])
						{
							match = false;
							break;
						}
					}
				}
				if (match)
				{
					return set.set;
				}
			}
		}
	}
	return nullptr;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
VulkanDescriptorSet* DescriptorCache::GetDescriptor(Stage stage, VulkanBuffer** storage_buffers, VulkanImage** textures2d_sampled,
                                                    const int* textures2d_sampled_view, VulkanImage** textures2d_storage,
                                                    uint64_t* samplers, VulkanBuffer** gds_buffers, const ShaderBindResources& bind)
{
	KYTY_PROFILER_BLOCK("DescriptorCache::GetDescriptor::search");

	int storage_buffers_num    = bind.storage_buffers.buffers_num;
	int textures2d_sampled_num = bind.textures2D.textures2d_sampled_num;
	int textures2d_storage_num = bind.textures2D.textures2d_storage_num;
	int samplers_num           = bind.samplers.samplers_num;
	int gds_buffers_num        = bind.gds_pointers.pointers_num;

	EXIT_IF(storage_buffers_num < 0 || storage_buffers_num > BUFFERS_MAX);
	EXIT_IF(textures2d_sampled_num < 0 || textures2d_sampled_num > TEXTURES_SAMPLED_MAX);
	EXIT_IF(textures2d_storage_num < 0 || textures2d_storage_num > TEXTURES_STORAGE_MAX);
	EXIT_IF(samplers_num < 0 || samplers_num > SAMPLERS_MAX);
	EXIT_IF(storage_buffers == nullptr);

	Core::LockGuard lock(m_mutex);

	auto* gctx = g_render_ctx->GetGraphicCtx();
	EXIT_IF(gctx == nullptr);

	Set nset;
	nset.set                    = nullptr;
	nset.storage_buffers_num    = storage_buffers_num;
	nset.textures2d_sampled_num = textures2d_sampled_num;
	nset.textures2d_storage_num = textures2d_storage_num;
	nset.samplers_num           = samplers_num;
	nset.gds_buffers_num        = gds_buffers_num;
	nset.stage                  = stage;
	for (int i = 0; i < storage_buffers_num; i++)
	{
		nset.storage_buffers_id[i] = storage_buffers[i]->memory.unique_id;
	}
	for (int i = 0; i < textures2d_sampled_num; i++)
	{
		nset.textures2d_sampled_id[i] = textures2d_sampled[i]->memory.unique_id;
	}
	for (int i = 0; i < textures2d_storage_num; i++)
	{
		nset.textures2d_storage_id[i] = textures2d_storage[i]->memory.unique_id;
	}
	for (int i = 0; i < samplers_num; i++)
	{
		nset.samplers_id[i] = samplers[i];
	}
	for (int i = 0; i < gds_buffers_num; i++)
	{
		nset.gds_buffers_id[i] = gds_buffers[i]->memory.unique_id;
	}
	nset.hash = CalcHash(nset);

	if (auto* f = FindSet(nset); f != nullptr)
	{
		return f;
	}

	KYTY_PROFILER_END_BLOCK;

	KYTY_PROFILER_BLOCK("DescriptorCache::GetDescriptor::create");

	auto* new_set = Allocate(stage, storage_buffers_num, textures2d_sampled_num, textures2d_storage_num, samplers_num, gds_buffers_num);
	EXIT_NOT_IMPLEMENTED(new_set == nullptr);

	VkDescriptorBufferInfo buffer_info[BUFFERS_MAX] {};
	for (int i = 0; i < storage_buffers_num; i++)
	{
		buffer_info[i].buffer = storage_buffers[i]->buffer;
		buffer_info[i].offset = 0;
		buffer_info[i].range  = VK_WHOLE_SIZE;
	}

	VkDescriptorImageInfo texture2d_sampled_info[TEXTURES_SAMPLED_MAX] {};
	for (int i = 0; i < textures2d_sampled_num; i++)
	{
		texture2d_sampled_info[i].sampler   = nullptr;
		texture2d_sampled_info[i].imageView = textures2d_sampled[i]->image_view[textures2d_sampled_view[i]];
		texture2d_sampled_info[i].imageLayout =
		    (textures2d_sampled[i]->type == VulkanImageType::DepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		                                                                  : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	VkDescriptorImageInfo texture2d_storage_info[TEXTURES_STORAGE_MAX] {};
	for (int i = 0; i < textures2d_storage_num; i++)
	{
		texture2d_storage_info[i].sampler     = nullptr;
		texture2d_storage_info[i].imageView   = textures2d_storage[i]->image_view[VulkanImage::VIEW_DEFAULT];
		texture2d_storage_info[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	VkDescriptorImageInfo sampler_info[SAMPLERS_MAX] {};
	for (int i = 0; i < samplers_num; i++)
	{
		sampler_info[i].sampler     = g_render_ctx->GetSamplerCache()->GetSampler(samplers[i]);
		sampler_info[i].imageView   = nullptr;
		sampler_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkDescriptorBufferInfo gds_buffer_info[GDS_BUFFER_MAX] {};
	for (int i = 0; i < gds_buffers_num; i++)
	{
		gds_buffer_info[i].buffer = gds_buffers[i]->buffer;
		gds_buffer_info[i].offset = 0;
		gds_buffer_info[i].range  = VK_WHOLE_SIZE;
	}

	uint32_t binding_num = 0;

	constexpr uint32_t B_MAX = 5;

	VkWriteDescriptorSet descriptor_write[B_MAX] = {};

	if (storage_buffers_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		descriptor_write[binding_num].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write[binding_num].pNext            = nullptr;
		descriptor_write[binding_num].dstSet           = new_set->set;
		descriptor_write[binding_num].dstBinding       = bind.storage_buffers.binding_index;
		descriptor_write[binding_num].dstArrayElement  = 0;
		descriptor_write[binding_num].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_write[binding_num].descriptorCount  = storage_buffers_num;
		descriptor_write[binding_num].pBufferInfo      = buffer_info;
		descriptor_write[binding_num].pImageInfo       = nullptr;
		descriptor_write[binding_num].pTexelBufferView = nullptr;
		binding_num++;
	}

	if (textures2d_sampled_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		descriptor_write[binding_num].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write[binding_num].pNext            = nullptr;
		descriptor_write[binding_num].dstSet           = new_set->set;
		descriptor_write[binding_num].dstBinding       = bind.textures2D.binding_sampled_index;
		descriptor_write[binding_num].dstArrayElement  = 0;
		descriptor_write[binding_num].descriptorType   = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptor_write[binding_num].descriptorCount  = textures2d_sampled_num;
		descriptor_write[binding_num].pBufferInfo      = nullptr;
		descriptor_write[binding_num].pImageInfo       = texture2d_sampled_info;
		descriptor_write[binding_num].pTexelBufferView = nullptr;
		binding_num++;
	}

	if (textures2d_storage_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		descriptor_write[binding_num].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write[binding_num].pNext            = nullptr;
		descriptor_write[binding_num].dstSet           = new_set->set;
		descriptor_write[binding_num].dstBinding       = bind.textures2D.binding_storage_index;
		descriptor_write[binding_num].dstArrayElement  = 0;
		descriptor_write[binding_num].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptor_write[binding_num].descriptorCount  = textures2d_storage_num;
		descriptor_write[binding_num].pBufferInfo      = nullptr;
		descriptor_write[binding_num].pImageInfo       = texture2d_storage_info;
		descriptor_write[binding_num].pTexelBufferView = nullptr;
		binding_num++;
	}

	if (samplers_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		descriptor_write[binding_num].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write[binding_num].pNext            = nullptr;
		descriptor_write[binding_num].dstSet           = new_set->set;
		descriptor_write[binding_num].dstBinding       = bind.samplers.binding_index;
		descriptor_write[binding_num].dstArrayElement  = 0;
		descriptor_write[binding_num].descriptorType   = VK_DESCRIPTOR_TYPE_SAMPLER;
		descriptor_write[binding_num].descriptorCount  = samplers_num;
		descriptor_write[binding_num].pBufferInfo      = nullptr;
		descriptor_write[binding_num].pImageInfo       = sampler_info;
		descriptor_write[binding_num].pTexelBufferView = nullptr;
		binding_num++;
	}

	if (gds_buffers_num > 0)
	{
		EXIT_IF(binding_num >= B_MAX);
		descriptor_write[binding_num].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write[binding_num].pNext            = nullptr;
		descriptor_write[binding_num].dstSet           = new_set->set;
		descriptor_write[binding_num].dstBinding       = bind.gds_pointers.binding_index;
		descriptor_write[binding_num].dstArrayElement  = 0;
		descriptor_write[binding_num].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_write[binding_num].descriptorCount  = gds_buffers_num;
		descriptor_write[binding_num].pBufferInfo      = gds_buffer_info;
		descriptor_write[binding_num].pImageInfo       = nullptr;
		descriptor_write[binding_num].pTexelBufferView = nullptr;
		binding_num++;
	}

	vkUpdateDescriptorSets(gctx->device, binding_num, descriptor_write, 0, nullptr);

	nset.set = new_set;

	int index = 0;

	if (m_first_free_set != -1)
	{
		index            = m_first_free_set;
		auto& set        = m_sets[m_first_free_set];
		m_first_free_set = set.next_free_set;
		set              = nset;
	} else
	{
		index = static_cast<int>(m_sets.Size());
		m_sets.Add(nset);
	}

	auto& ids = m_sets_map[nset.hash];
	if (!ids.Contains(index))
	{
		ids.Add(index);
	}

	return new_set;
}

void DescriptorCache::FreeDescriptor(VulkanBuffer* buffer)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(buffer == nullptr);

	Core::LockGuard lock(m_mutex);

	int index = 0;
	for (auto& set: m_sets)
	{
		if (set.set != nullptr)
		{
			for (int i = 0; i < set.storage_buffers_num; i++)
			{
				if (set.storage_buffers_id[i] == buffer->memory.unique_id)
				{
					Free(set.set);
					set.set           = nullptr;
					set.next_free_set = m_first_free_set;
					m_first_free_set  = index;
					auto& ids         = m_sets_map[set.hash];
					ids.Remove(index);
					if (ids.IsEmpty())
					{
						m_sets_map.Remove(set.hash);
					}
					break;
				}
			}
		}
		index++;
	}
}

void DescriptorCache::FreeDescriptor(VulkanImage* image)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(image == nullptr);

	Core::LockGuard lock(m_mutex);

	int index = 0;
	for (auto& set: m_sets)
	{
		if (set.set != nullptr)
		{
			for (int i = 0; i < set.textures2d_sampled_num; i++)
			{
				if (set.textures2d_sampled_id[i] == image->memory.unique_id)
				{
					Free(set.set);
					set.set           = nullptr;
					set.next_free_set = m_first_free_set;
					m_first_free_set  = index;
					auto& ids         = m_sets_map[set.hash];
					ids.Remove(index);
					if (ids.IsEmpty())
					{
						m_sets_map.Remove(set.hash);
					}
					break;
				}
			}
		}
		index++;
	}
}

VkDescriptorSetLayout DescriptorCache::GetDescriptorSetLayout(Stage stage, const ShaderBindResources& bind)
{
	int storage_buffers_num    = bind.storage_buffers.buffers_num;
	int textures2d_sampled_num = bind.textures2D.textures2d_sampled_num;
	int textures2d_storage_num = bind.textures2D.textures2d_storage_num;
	int samplers_num           = bind.samplers.samplers_num;
	int gds_buffers_num        = bind.gds_pointers.pointers_num;

	EXIT_IF(storage_buffers_num < 0 || storage_buffers_num > BUFFERS_MAX);
	EXIT_IF(textures2d_sampled_num < 0 || textures2d_sampled_num > TEXTURES_SAMPLED_MAX);
	EXIT_IF(textures2d_storage_num < 0 || textures2d_storage_num > TEXTURES_STORAGE_MAX);
	EXIT_IF(samplers_num < 0 || samplers_num > SAMPLERS_MAX);
	EXIT_IF(gds_buffers_num < 0 || gds_buffers_num > GDS_BUFFER_MAX);

	EXIT_NOT_IMPLEMENTED(stage != Stage::Pixel && stage != Stage::Compute &&
	                     (textures2d_sampled_num > 0 || textures2d_storage_num > 0 || samplers_num > 0));

	Core::LockGuard lock(m_mutex);
	Init();
	switch (stage)
	{
		case Stage::Vertex:
			return m_descriptor_set_layout_vertex[storage_buffers_num][textures2d_sampled_num][textures2d_storage_num][samplers_num]
			                                     [gds_buffers_num];
		case Stage::Pixel:
			return m_descriptor_set_layout_pixel[storage_buffers_num][textures2d_sampled_num][textures2d_storage_num][samplers_num]
			                                    [gds_buffers_num];
		case Stage::Compute:
			return m_descriptor_set_layout_compute[storage_buffers_num][textures2d_sampled_num][textures2d_storage_num][samplers_num]
			                                      [gds_buffers_num];
		default: EXIT("unknown stage\n");
	}
	return nullptr;
}

void DeleteFramebuffer(VideoOutVulkanImage* image)
{
	g_render_ctx->GetFramebufferCache()->FreeFramebufferByColor(image);
}

void DeleteFramebuffer(RenderTextureVulkanImage* image)
{
	g_render_ctx->GetFramebufferCache()->FreeFramebufferByColor(image);
}

void DeleteFramebuffer(DepthStencilVulkanImage* image)
{
	g_render_ctx->GetFramebufferCache()->FreeFramebufferByDepth(image);
}

void DeleteDescriptor(VulkanBuffer* buffer)
{
	g_render_ctx->GetDescriptorCache()->FreeDescriptor(buffer);
}

void DeleteDescriptor(TextureVulkanImage* image)
{
	g_render_ctx->GetDescriptorCache()->FreeDescriptor(image);
}

void DeleteDescriptor(StorageTextureVulkanImage* image)
{
	g_render_ctx->GetDescriptorCache()->FreeDescriptor(image);
}

void DeleteDescriptor(RenderTextureVulkanImage* image)
{
	g_render_ctx->GetDescriptorCache()->FreeDescriptor(image);
}

static bool get_stencil_op(VkStencilOp* f, uint32_t* ref, uint8_t op, uint8_t testval, uint8_t opval)
{
	VkStencilOp vk      = VK_STENCIL_OP_KEEP;
	uint32_t    r       = opval;
	bool        use_ref = true;

	switch (op)
	{
		case 0x00:
			vk      = VK_STENCIL_OP_KEEP;
			use_ref = false;
			break; /* Keep           */
		case 0x01:
			vk      = VK_STENCIL_OP_ZERO;
			use_ref = false;
			break; /* Zero           */
		case 0x02:
			vk = VK_STENCIL_OP_REPLACE;
			r  = 0xFF;
			break; /* Ones           */
		case 0x03:
			vk = VK_STENCIL_OP_REPLACE;
			r  = testval;
			break;                                                /* ReplaceTest    */
		case 0x04: vk = VK_STENCIL_OP_REPLACE; break;             /* ReplaceOp      */
		case 0x05: vk = VK_STENCIL_OP_INCREMENT_AND_CLAMP; break; /* AddClamp       */
		case 0x06: vk = VK_STENCIL_OP_DECREMENT_AND_CLAMP; break; /* SubClamp       */
		case 0x07: vk = VK_STENCIL_OP_INVERT; break;              /* Invert         */
		case 0x08: vk = VK_STENCIL_OP_INCREMENT_AND_WRAP; break;  /* AddWrap        */
		case 0x09: vk = VK_STENCIL_OP_DECREMENT_AND_WRAP; break;  /* SubWrap        */
		case 0x0a:                                                /* And            */
		case 0x0b:                                                /* Or             */
		case 0x0c:                                                /* Xor            */
		case 0x0d:                                                /* Nand           */
		case 0x0e:                                                /* Nor            */
		case 0x0f:                                                /* Xnor           */
		default: EXIT("invalid op");
	}
	*f   = vk;
	*ref = r;
	return use_ref;
}

static void get_stencil_state(PipelineStencilStaticState* s, PipelineStencilDynamicState* d, uint8_t func, uint8_t fail, uint8_t zpass,
                              uint8_t zfail, uint8_t testval, uint8_t mask, uint8_t writemask, uint8_t opval)
{
	EXIT_IF(s == nullptr || d == nullptr);

	uint32_t ref[3]     = {};
	bool     use_ref[3] = {};

	use_ref[0]     = get_stencil_op(&s->failOp, ref + 0, fail, testval, opval);
	use_ref[1]     = get_stencil_op(&s->passOp, ref + 1, zpass, testval, opval);
	use_ref[2]     = get_stencil_op(&s->depthFailOp, ref + 2, zfail, testval, opval);
	s->compareOp   = static_cast<VkCompareOp>(func);
	d->compareMask = mask;
	d->writeMask   = writemask;

	if (use_ref[0])
	{
		EXIT_NOT_IMPLEMENTED((ref[0] != ref[1] && use_ref[1]) || (ref[0] != ref[2] && use_ref[2]));
		d->reference = ref[0];
	} else if (use_ref[1])
	{
		EXIT_NOT_IMPLEMENTED(ref[1] != ref[2] && use_ref[2]);
		d->reference = ref[1];
	} else if (use_ref[2])
	{
		d->reference = ref[2];
	} else
	{
		d->reference = 0;
	}
}

static Vector<RenderTextureVulkanImage*> FindRenderTexture(uint64_t vaddr, uint64_t size, bool exact)
{
	KYTY_PROFILER_FUNCTION();

	Vector<RenderTextureVulkanImage*> ret;

	auto objects = GpuMemoryFindObjects(vaddr, size, GpuMemoryObjectType::RenderTexture, exact, false);

	for (const auto& obj: objects)
	{
		if (obj.type == GpuMemoryObjectType::RenderTexture)
		{
			ret.Add(static_cast<RenderTextureVulkanImage*>(obj.obj));
		}
	}

	return ret;
}

static Vector<DepthStencilVulkanImage*> FindDepthStencil(uint64_t vaddr, uint64_t size, bool exact)
{
	KYTY_PROFILER_FUNCTION();

	Vector<DepthStencilVulkanImage*> ret;

	auto objects = GpuMemoryFindObjects(vaddr, size, GpuMemoryObjectType::DepthStencilBuffer, exact, true);

	for (const auto& obj: objects)
	{
		if (obj.type == GpuMemoryObjectType::DepthStencilBuffer)
		{
			ret.Add(static_cast<DepthStencilVulkanImage*>(obj.obj));
		}
	}

	return ret;
}

void GraphicsRenderMemoryBarrier(CommandBuffer* buffer)
{
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	VkMemoryBarrier mem_barrier {};
	mem_barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	mem_barrier.pNext         = nullptr;
	mem_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	mem_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	vkCmdPipelineBarrier(vk_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	                     VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &mem_barrier, 0, nullptr, 0,
	                     nullptr);
}

static void GraphicsRenderRenderTextureBarrier(VkCommandBuffer vk_buffer, VulkanImage* image)
{
	EXIT_IF(image == nullptr);

	if (image->layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		VkImageMemoryBarrier image_memory_barrier {};
		image_memory_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext                           = nullptr;
		image_memory_barrier.srcAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
		image_memory_barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
		image_memory_barrier.oldLayout                       = image->layout;
		image_memory_barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image                           = image->image;
		image_memory_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseMipLevel   = 0;
		image_memory_barrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount     = 1;

		vkCmdPipelineBarrier(vk_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                     VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
		                     &image_memory_barrier);

		image->layout = image_memory_barrier.newLayout;
	}
}

static void GraphicsRenderDepthStencilBarrier(VkCommandBuffer vk_buffer, VulkanImage* image)
{
	EXIT_IF(image == nullptr);

	if (image->layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		VkImageMemoryBarrier image_memory_barrier {};
		image_memory_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext                           = nullptr;
		image_memory_barrier.srcAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
		image_memory_barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
		image_memory_barrier.oldLayout                       = image->layout;
		image_memory_barrier.newLayout                       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image                           = image->image;
		image_memory_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
		image_memory_barrier.subresourceRange.baseMipLevel   = 0;
		image_memory_barrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount     = 1;

		vkCmdPipelineBarrier(vk_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                     VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
		                     &image_memory_barrier);

		image->layout = image_memory_barrier.newLayout;
	}
}

void GraphicsRenderRenderTextureBarrier(CommandBuffer* buffer, uint64_t vaddr, uint64_t size)
{
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	auto images = FindRenderTexture(vaddr, size, false);

	for (auto* image: images)
	{
		GraphicsRenderRenderTextureBarrier(vk_buffer, image);
	}
}

void GraphicsRenderDepthStencilBarrier(CommandBuffer* buffer, uint64_t vaddr, uint64_t size)
{
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	auto images = FindDepthStencil(vaddr, size, false);

	for (auto* image: images)
	{
		GraphicsRenderDepthStencilBarrier(vk_buffer, image);
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void FindRenderDepthInfo(uint64_t submit_id, CommandBuffer* /*buffer*/, const HW::Context& hw, RenderDepthInfo* r)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(r == nullptr);

	const auto& z  = hw.GetDepthRenderTarget();
	const auto& rc = hw.GetRenderControl();
	const auto& dc = hw.GetDepthControl();
	const auto& sc = hw.GetStencilControl();
	const auto& sm = hw.GetStencilMask();
	const auto& cc = hw.GetColorControl();

	TileSizeAlign stencil_size {};
	TileSizeAlign htile_size {};
	TileSizeAlign depth_size {};

	bool neo        = Config::IsNeo();
	bool ps5        = Config::IsNextGen();
	bool htile      = z.z_info.tile_surface_enable;
	bool decompress = htile && (rc.depth_compress_disable || rc.stencil_compress_disable);

	if (dc.z_enable || dc.z_write_enable || dc.stencil_enable || decompress)
	{
		switch (z.z_info.format * 2 + z.stencil_info.format)
		{
			case 0: r->format = VK_FORMAT_UNDEFINED; break;
			case 1: /*r->format = VK_FORMAT_S8_UINT*/ EXIT("Unsupported format: VK_FORMAT_S8_UINT\n"); break;
			case 2: r->format = VK_FORMAT_D16_UNORM; break;
			case 3: r->format = VK_FORMAT_D24_UNORM_S8_UINT; break;
			case 6: r->format = VK_FORMAT_D32_SFLOAT; break;
			case 7: r->format = VK_FORMAT_D32_SFLOAT_S8_UINT; break;
			default: EXIT("unknown z and stencil format: %u, %u\n", z.z_info.format, z.stencil_info.format);
		}
	}

	if (r->format != VK_FORMAT_UNDEFINED)
	{
		if (ps5)
		{
			bool size_found = TileGetDepthSize(z.size.x_max + 1, z.size.y_max + 1, 0, z.z_info.format, z.stencil_info.format, htile, true,
			                                   true, &stencil_size, &htile_size, &depth_size);
			EXIT_NOT_IMPLEMENTED(!size_found);
		} else
		{
			uint32_t size  = 0;
			uint32_t pitch = (z.pitch_div8_minus1 + 1) * 8;

			if (z.z_info.format == 3)
			{
				size = (z.slice_div64_minus1 + 1) * 64 * 4;
			} else if (z.z_info.format == 1)
			{
				size = (z.slice_div64_minus1 + 1) * 64 * 2;
			}

			if (!TileGetDepthSize(z.width, z.height, pitch, z.z_info.format, z.stencil_info.format, htile, neo, false, &stencil_size,
			                      &htile_size, &depth_size))
			{
				depth_size.size  = size;
				depth_size.align = neo ? 65536 : 32768;
			} else
			{
				EXIT_NOT_IMPLEMENTED(depth_size.size != size);
			}
		}
	}

	auto stencil_addr_mask = static_cast<uint64_t>(stencil_size.align) - 1;
	auto depth_addr_mask   = static_cast<uint64_t>(depth_size.align) - 1;
	auto htile_addr_mask   = static_cast<uint64_t>(htile_size.align) - 1;

	r->htile                = htile;
	r->neo                  = neo;
	r->depth_buffer_size    = depth_size.size;
	r->depth_buffer_vaddr   = z.z_read_base_addr & ~depth_addr_mask;
	r->depth_tile_swizzle   = z.z_read_base_addr & depth_addr_mask;
	r->stencil_buffer_size  = stencil_size.size;
	r->stencil_buffer_vaddr = z.stencil_read_base_addr & ~stencil_addr_mask;
	r->stencil_tile_swizzle = z.stencil_read_base_addr & stencil_addr_mask;
	r->htile_buffer_size    = htile_size.size;
	r->htile_buffer_vaddr   = z.htile_data_base_addr & ~htile_addr_mask;
	r->htile_tile_swizzle   = z.htile_data_base_addr & htile_addr_mask;
	r->width                = (ps5 ? z.size.x_max + 1 : z.width);
	r->height               = (ps5 ? z.size.y_max + 1 : z.height);
	r->depth_clear_enable   = rc.depth_clear_enable;
	r->depth_clear_value    = hw.GetDepthClearValue();
	r->depth_test_enable    = dc.z_enable;
	r->depth_write_enable   = dc.z_write_enable;
	r->depth_compare_op     = static_cast<VkCompareOp>(dc.zfunc);

	r->depth_bounds_test_enable = dc.depth_bounds_enable;
	r->depth_min_bounds         = 0.0f;
	r->depth_max_bounds         = 0.0f;
	EXIT_NOT_IMPLEMENTED(r->depth_bounds_test_enable);

	r->stencil_clear_enable = rc.stencil_clear_enable;
	r->stencil_clear_value  = hw.GetStencilClearValue();
	// EXIT_NOT_IMPLEMENTED(r->stencil_clear_enable);

	r->stencil_test_enable = dc.stencil_enable;
	if (r->stencil_test_enable)
	{
		get_stencil_state(&r->stencil_static_front, &r->stencil_dynamic_front, dc.stencilfunc, sc.stencil_fail, sc.stencil_zpass,
		                  sc.stencil_zfail, sm.stencil_testval, sm.stencil_mask, sm.stencil_writemask, sm.stencil_opval);
		if (dc.backface_enable)
		{
			get_stencil_state(&r->stencil_static_back, &r->stencil_dynamic_back, dc.stencilfunc_bf, sc.stencil_fail_bf, sc.stencil_zpass_bf,
			                  sc.stencil_zfail_bf, sm.stencil_testval_bf, sm.stencil_mask_bf, sm.stencil_writemask_bf, sm.stencil_opval_bf);
		} else
		{
			r->stencil_static_back  = r->stencil_static_front;
			r->stencil_dynamic_back = r->stencil_dynamic_front;
		}
	} else
	{
		r->stencil_static_front  = {};
		r->stencil_static_back   = {};
		r->stencil_dynamic_front = {};
		r->stencil_dynamic_back  = {};
	}
	// EXIT_NOT_IMPLEMENTED(r->stencil_test_enable);
	EXIT_NOT_IMPLEMENTED(dc.backface_enable);

	r->vulkan_buffer = nullptr;

	if (r->format != VK_FORMAT_UNDEFINED)
	{
		bool sampled = ((z.stencil_info.format == 0 && z.z_info.tile_mode_index != 0) || z.stencil_info.texture_compatible_stencil);

		DepthStencilBufferObject vulkan_buffer_info(r->format, r->width, r->height, htile, neo, sampled);

		EXIT_NOT_IMPLEMENTED(z.z_info.tile_mode_index != 0 && r->depth_tile_swizzle != 0);
		EXIT_NOT_IMPLEMENTED(r->stencil_tile_swizzle != 0);
		EXIT_NOT_IMPLEMENTED(r->htile_tile_swizzle != 0);

		r->vaddr_num = 0;

		if (r->depth_buffer_size > 0)
		{
			r->vaddr[r->vaddr_num] = r->depth_buffer_vaddr;
			r->size[r->vaddr_num]  = r->depth_buffer_size;
			r->vaddr_num++;
		}

		if (r->stencil_buffer_size > 0)
		{
			r->vaddr[r->vaddr_num] = r->stencil_buffer_vaddr;
			r->size[r->vaddr_num]  = r->stencil_buffer_size;
			r->vaddr_num++;
		}

		if (r->htile_buffer_size > 0)
		{
			r->vaddr[r->vaddr_num] = r->htile_buffer_vaddr;
			r->size[r->vaddr_num]  = r->htile_buffer_size;
			r->vaddr_num++;
		}

		EXIT_NOT_IMPLEMENTED(r->vaddr_num == 0);

		r->vulkan_buffer = static_cast<DepthStencilVulkanImage*>(
		    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, r->vaddr, r->size, r->vaddr_num, vulkan_buffer_info));

		EXIT_NOT_IMPLEMENTED(r->vulkan_buffer == nullptr);

		if ((cc.mode == 0 && cc.op == 0xCC) || (dc.z_enable || dc.z_write_enable))
		{
			r->vulkan_buffer->compressed = htile && !decompress;
		}
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void FindRenderColorInfo(uint64_t submit_id, CommandBuffer* buffer, const HW::Context& hw, RenderColorInfo* r)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(r == nullptr);

	const auto& rt   = hw.GetRenderTarget(0);
	auto        mask = hw.GetRenderTargetMask();

	if (rt.base.addr == 0 || mask == 0)
	{
		// No color output
		r->type          = RenderColorType::NoColorOutput;
		r->base_addr     = 0;
		r->vulkan_buffer = nullptr;
		r->buffer_size   = 0;
		return;
	}

	bool ps5 = Config::IsNextGen();

	uint32_t width      = 0;
	uint32_t height     = 0;
	uint32_t pitch      = 0;
	uint32_t size       = 0;
	bool     tile       = false;
	bool     write_back = false;

	if (ps5)
	{
		switch (rt.attrib3.tile_mode)
		{
			case 0x1b:
				tile       = true;
				write_back = false;
				break;
			default: EXIT("unknown tile mode: %u\n", rt.attrib3.tile_mode);
		}

		width  = rt.attrib2.width + 1;
		height = rt.attrib2.height + 1;
		pitch  = width;

		Graphics::TileSizeAlign size32 {};
		Graphics::TileGetVideoOutSize(width, height, pitch, tile, true, &size32);

		size = size32.size;

		EXIT_NOT_IMPLEMENTED(size == 0);
	} else
	{
		switch (rt.attrib.tile_mode)
		{
			case 0x8:
				tile       = false;
				write_back = false;
				break;

			case 0x1f:
				tile       = false;
				write_back = true;
				break;

			case 0xa:
			case 0xd:
			case 0xe:
				tile       = true;
				write_back = false;
				break;

			default: EXIT("unknown tile mode: %u\n", rt.attrib.tile_mode);
		}

		width  = rt.size.width;
		height = rt.size.height;
		pitch  = (rt.pitch.pitch_div8_minus1 + 1) * 8;
		size   = (rt.slice.slice_div64_minus1 + 1) * 64 * 4;
	}

	auto video_image       = VideoOut::VideoOutGetImage(rt.base.addr);
	bool render_to_texture = (video_image.image == nullptr);

	if (render_to_texture)
	{
		RenderTextureFormat rt_format = RenderTextureFormat::Unknown;

		if (rt.info.format == 0xa && rt.info.channel_type == 0x0 && rt.info.channel_order == 0x0)
		{
			rt_format = RenderTextureFormat::R8G8B8A8Unorm;
		} else if (rt.info.format == 0xa && rt.info.channel_type == 0x6 && rt.info.channel_order == 0x0)
		{
			rt_format = RenderTextureFormat::R8G8B8A8Srgb;
		} else if (rt.info.format == 0xa && rt.info.channel_type == 0x0 && rt.info.channel_order == 0x1)
		{
			rt_format = RenderTextureFormat::B8G8R8A8Unorm;
		} else if (rt.info.format == 0xa && rt.info.channel_type == 0x6 && rt.info.channel_order == 0x1)
		{
			rt_format = RenderTextureFormat::B8G8R8A8Srgb;
		} else if (rt.info.format == 0x1 && rt.info.channel_type == 0x0 && rt.info.channel_order == 0x0)
		{
			rt_format = RenderTextureFormat::R8Unorm;
		} else
		{
			EXIT("%s\n unknown format\n", rt_print("RenderTarget", rt).Concat(U"").C_Str());
		}

		// Render to texture
		RenderTextureObject vulkan_buffer_info(rt_format, width, height, tile, rt.info.neo_mode, pitch, write_back);
		auto*               buffer_vulkan = static_cast<Graphics::RenderTextureVulkanImage*>(
            Graphics::GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), buffer, rt.base.addr, size, vulkan_buffer_info));
		EXIT_NOT_IMPLEMENTED(buffer_vulkan == nullptr);
		r->type          = RenderColorType::RenderTexture;
		r->base_addr     = rt.base.addr;
		r->vulkan_buffer = buffer_vulkan;
		r->buffer_size   = size;
	} else
	{
		EXIT_NOT_IMPLEMENTED(!(rt.info.format == 0xa && (rt.info.channel_type == 0x6 || rt.info.channel_type == 0x0) &&
		                       (rt.info.channel_order == 0x0 || rt.info.channel_order == 0x1)));

		// Display buffer
		EXIT_NOT_IMPLEMENTED(video_image.buffer_size != size);
		EXIT_NOT_IMPLEMENTED(video_image.buffer_pitch != pitch);
		r->type          = RenderColorType::DisplayBuffer;
		r->base_addr     = rt.base.addr;
		r->vulkan_buffer = video_image.image;
		r->buffer_size   = video_image.buffer_size;
	}
}

static void InvalidateMemoryObject(const RenderColorInfo& r)
{
	bool with_color = (r.vulkan_buffer != nullptr);

	if (with_color)
	{
		if (r.type == RenderColorType::RenderTexture)
		{
			GpuMemoryResetHash(&r.base_addr, &r.buffer_size, 1, GpuMemoryObjectType::RenderTexture);
		} else if (r.type == RenderColorType::DisplayBuffer /*|| r.type == RenderColorType::OffscreenBuffer*/)
		{
			GpuMemoryResetHash(&r.base_addr, &r.buffer_size, 1, GpuMemoryObjectType::VideoOutBuffer);
		}
	}
}

static void InvalidateMemoryObject(const RenderDepthInfo& r)
{
	bool with_depth = (r.format != VK_FORMAT_UNDEFINED && r.vulkan_buffer != nullptr);

	if (with_depth)
	{
		GpuMemoryResetHash(r.vaddr, r.size, r.vaddr_num, GpuMemoryObjectType::DepthStencilBuffer);
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void PrepareStorageBuffers(uint64_t submit_id, CommandBuffer* buffer, const ShaderStorageResources& storage_buffers,
                                  VulkanBuffer** buffers, uint32_t** sgprs)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(buffers == nullptr);
	EXIT_IF(sgprs == nullptr);
	EXIT_IF(*sgprs == nullptr);

	bool gen5 = Config::IsNextGen();

	for (int i = 0; i < storage_buffers.buffers_num; i++)
	{
		auto r = storage_buffers.buffers[i];

		EXIT_NOT_IMPLEMENTED(r.AddTid());
		EXIT_NOT_IMPLEMENTED(r.SwizzleEnabled());

		if (gen5)
		{
			EXIT_NOT_IMPLEMENTED(r.OutOfBounds() != 0);
			EXIT_NOT_IMPLEMENTED(!((r.Stride() == 16 && r.DstSelXYZW() == DstSel(4, 5, 6, 7) && r.Format() == 77)));
		} else
		{
			EXIT_NOT_IMPLEMENTED(!((r.Stride() == 4 && r.DstSelXYZW() == DstSel(4, 0, 0, 0) && r.Dfmt() == 4 && r.Nfmt() == 4) ||
			                       (r.Stride() == 4 && r.DstSelXYZW() == DstSel(4, 0, 0, 1) && r.Dfmt() == 4 && r.Nfmt() == 7) ||
			                       (r.Stride() == 8 && r.DstSelXYZW() == DstSel(4, 5, 0, 0) && r.Dfmt() == 11 && r.Nfmt() == 4) ||
			                       (r.Stride() == 16 && r.DstSelXYZW() == DstSel(4, 5, 6, 7) && r.Dfmt() == 14 && r.Nfmt() == 7)));
			EXIT_NOT_IMPLEMENTED(!(r.MemoryType() == 0x00 || r.MemoryType() == 0x10 || r.MemoryType() == 0x6d));
		}

		auto addr        = (gen5 ? r.Base48() : r.Base44());
		auto stride      = r.Stride();
		auto num_records = r.NumRecords();
		auto size        = stride * num_records;
		EXIT_NOT_IMPLEMENTED(size == 0);
		EXIT_NOT_IMPLEMENTED((size & 0x3u) != 0);

		bool read_only = (gen5 ? false : (r.MemoryType() == 0x10));

		EXIT_NOT_IMPLEMENTED(read_only && !(storage_buffers.usages[i] == ShaderStorageUsage::ReadOnly ||
		                                    storage_buffers.usages[i] == ShaderStorageUsage::Constant));

		StorageBufferGpuObject buf_info(stride, num_records, read_only);

		auto* buf = static_cast<StorageVulkanBuffer*>(
		    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, addr, size, buf_info));

		EXIT_NOT_IMPLEMENTED(buf == nullptr);

		StorageBufferSet(buffer, buf);

		buffers[i] = buf;

		if (gen5)
		{
			r.UpdateAddress48(i);
		} else
		{
			r.UpdateAddress44(i);
		}

		EXIT_NOT_IMPLEMENTED(((gen5 ? r.Base48() : r.Base44()) >> 32u) != 0);

		(*sgprs)[0] = r.fields[0];
		(*sgprs)[1] = r.fields[1];
		(*sgprs)[2] = r.fields[2];
		(*sgprs)[3] = r.fields[3];

		(*sgprs) += 4;
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void PrepareTextures(uint64_t submit_id, CommandBuffer* buffer, const ShaderTextureResources& textures, VulkanImage** images_sampled,
                            VulkanImage** images_storage, int* images_sampled_view, uint32_t** sgprs)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(images_sampled == nullptr);
	EXIT_IF(images_storage == nullptr);
	EXIT_IF(sgprs == nullptr);
	EXIT_IF(*sgprs == nullptr);

	int index_sampled = 0;
	int index_storage = 0;

	bool gen5 = Config::IsNextGen();

	for (int i = 0; i < textures.textures_num; i++)
	{
		auto r = textures.desc[i].texture;

		if (gen5)
		{
			EXIT_NOT_IMPLEMENTED(!(r.TileMode() == 0));
			EXIT_NOT_IMPLEMENTED(r.Format() != 56);
			EXIT_NOT_IMPLEMENTED(r.PerfMod5() != 7 && r.PerfMod5() != 0);
			EXIT_NOT_IMPLEMENTED(r.BCSwizzle() != 0);
			EXIT_NOT_IMPLEMENTED(r.BaseArray5() != 0);
			EXIT_NOT_IMPLEMENTED(r.ArrayPitch() != 0);
			EXIT_NOT_IMPLEMENTED(r.MaxMip() != 0);
			EXIT_NOT_IMPLEMENTED(r.MinLodWarn5() != 0);
			EXIT_NOT_IMPLEMENTED(r.MipStatsCntId() != 0);
			EXIT_NOT_IMPLEMENTED(r.MipStatsCntEn() != false);
			EXIT_NOT_IMPLEMENTED(r.CornerSample() != false);
			EXIT_NOT_IMPLEMENTED(r.PrtDefColor() != false);
			EXIT_NOT_IMPLEMENTED(r.MsaaDepth() != false);
			EXIT_NOT_IMPLEMENTED(r.MaxUncompBlkSize() != 0);
			EXIT_NOT_IMPLEMENTED(r.MaxCompBlkSize() != 0);
			EXIT_NOT_IMPLEMENTED(r.MetaPipeAligned() != false);
			EXIT_NOT_IMPLEMENTED(r.WriteCompress() != false);
			EXIT_NOT_IMPLEMENTED(r.MetaCompress() != false);
			EXIT_NOT_IMPLEMENTED(r.DccAlphaPos() != false);
			EXIT_NOT_IMPLEMENTED(r.DccColorTransf() != false);
			EXIT_NOT_IMPLEMENTED(r.MetaAddr() != 0);
		} else
		{
			EXIT_NOT_IMPLEMENTED(r.Dfmt() != 1 && r.Dfmt() != 10 && r.Dfmt() != 37 && r.Dfmt() != 4 && r.Dfmt() != 35 && r.Dfmt() != 3 &&
			                     r.Dfmt() != 36);
			EXIT_NOT_IMPLEMENTED(r.Nfmt() != 9 && r.Nfmt() != 0 && r.Nfmt() != 7);
			EXIT_NOT_IMPLEMENTED(r.PerfMod() != 7 && r.PerfMod() != 0);
			EXIT_NOT_IMPLEMENTED(r.Interlaced() != false);
			EXIT_NOT_IMPLEMENTED(!(r.TileMode() == 8 || r.TileMode() == 13 || r.TileMode() == 14 || r.TileMode() == 2 ||
			                       r.TileMode() == 10 || r.TileMode() == 31));
			EXIT_NOT_IMPLEMENTED(r.BaseArray() != 0);
			EXIT_NOT_IMPLEMENTED(r.LastArray() != 0);
			EXIT_NOT_IMPLEMENTED(r.MinLodWarn() != 0);
			EXIT_NOT_IMPLEMENTED(r.CounterBankId() != 0);
			EXIT_NOT_IMPLEMENTED(r.LodHdwCntEn() != false);
			EXIT_NOT_IMPLEMENTED(r.MemoryType() != 0x10 && r.MemoryType() != 0x6d);
		}
		EXIT_NOT_IMPLEMENTED((gen5 ? r.Base40() : r.Base38()) == 0);
		EXIT_NOT_IMPLEMENTED(r.MinLod() != 0);
		EXIT_NOT_IMPLEMENTED(r.Type() != 9);
		EXIT_NOT_IMPLEMENTED(r.Depth() != 0);

		bool read_only = (gen5 ? false : (r.MemoryType() == 0x10));

		EXIT_NOT_IMPLEMENTED(read_only && !(textures.desc[i].usage == ShaderTextureUsage::ReadOnly));

		TileSizeAlign size {};
		auto          addr       = (gen5 ? r.Base40() : r.Base38());
		bool          neo        = Config::IsNeo();
		auto          width      = (gen5 ? r.Width5() : r.Width4()) + 1;
		auto          height     = (gen5 ? r.Height5() : r.Height4()) + 1;
		auto          pitch      = (gen5 ? width : r.Pitch() + 1);
		auto          base_level = r.BaseLevel();
		auto          levels     = r.LastLevel() + 1;
		auto          tile       = r.TileMode();
		auto          dfmt       = (gen5 ? 0 : r.Dfmt());
		auto          nfmt       = (gen5 ? 0 : r.Nfmt());
		auto          fmt        = (gen5 ? r.Format() : 0);
		uint32_t      swizzle    = r.DstSelXYZW();

		bool check_depth_texture = (!gen5 && tile == 2);

		if (gen5)
		{
			TileGetTextureSize2(fmt, width, height, pitch, levels, tile, &size, nullptr, nullptr);
		} else
		{
			TileGetTextureSize(dfmt, nfmt, width, height, pitch, levels, tile, neo, &size, nullptr, nullptr);
		}

		EXIT_NOT_IMPLEMENTED(size.size == 0);
		EXIT_NOT_IMPLEMENTED((addr & (static_cast<uint64_t>(size.align) - 1u)) != 0);

		VulkanImage* tex            = nullptr;
		bool         render_texture = false;
		bool         depth_texture  = false;
		int          view_type      = VulkanImage::VIEW_DEFAULT;

		if (check_depth_texture)
		{
			auto dtex     = FindDepthStencil(addr, size.size, true);
			depth_texture = !dtex.IsEmpty();
			if (depth_texture)
			{
				EXIT_NOT_IMPLEMENTED(dtex.Size() > 1);
				EXIT_NOT_IMPLEMENTED(swizzle != DstSel(4, 4, 4, 4));
				EXIT_NOT_IMPLEMENTED(dtex.At(0)->compressed);
				tex = dtex.At(0);
			}
		} else
		{
			auto rtex      = FindRenderTexture(addr, size.size, true);
			render_texture = !rtex.IsEmpty();
			if (render_texture)
			{
				EXIT_NOT_IMPLEMENTED(rtex.Size() > 1);
				EXIT_NOT_IMPLEMENTED(swizzle != DstSel(4, 5, 6, 7) && swizzle != DstSel(6, 5, 4, 7));
				tex = rtex.At(0);
				if (swizzle == DstSel(6, 5, 4, 7))
				{
					view_type = VulkanImage::VIEW_BGRA;
				}
			}
		}

		if (!render_texture && !depth_texture)
		{
			if (textures.desc[i].textures2d_without_sampler)
			{
				EXIT_NOT_IMPLEMENTED(textures.desc[i].usage != ShaderTextureUsage::ReadWrite);

				StorageTextureObject vulkan_texture_info(dfmt, nfmt, fmt, width, height, pitch, base_level, levels, tile, neo, swizzle);
				tex = static_cast<StorageTextureVulkanImage*>(
				    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), buffer, addr, size.size, vulkan_texture_info));
			} else
			{
				EXIT_NOT_IMPLEMENTED(textures.desc[i].usage != ShaderTextureUsage::ReadOnly);

				if (!gen5 && tile == 10)
				{
					RenderTextureFormat rt_format = RenderTextureFormat::Unknown;
					if (dfmt == 10 && nfmt == 0)
					{
						rt_format = RenderTextureFormat::R8G8B8A8Unorm;
					}

					RenderTextureObject vulkan_buffer_info(rt_format, width, height, true, neo, pitch, false);
					tex = static_cast<Graphics::RenderTextureVulkanImage*>(Graphics::GpuMemoryCreateObject(
					    submit_id, g_render_ctx->GetGraphicCtx(), buffer, addr, size.size, vulkan_buffer_info));
				} else
				{
					TextureObject vulkan_texture_info(dfmt, nfmt, fmt, width, height, pitch, base_level, levels, tile, neo, swizzle);
					tex = static_cast<TextureVulkanImage*>(
					    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), buffer, addr, size.size, vulkan_texture_info));
				}
			}
		}

		EXIT_NOT_IMPLEMENTED(tex == nullptr);

		if (textures.desc[i].textures2d_without_sampler)
		{
			images_storage[index_storage] = tex;
			if (gen5)
			{
				r.UpdateAddress40(index_storage);
			} else
			{
				r.UpdateAddress38(index_storage);
			}
			index_storage++;
		} else
		{
			images_sampled[index_sampled]      = tex;
			images_sampled_view[index_sampled] = (depth_texture ? VulkanImage::VIEW_DEPTH_TEXTURE : view_type);
			if (gen5)
			{
				r.UpdateAddress40(index_sampled);
			} else
			{
				r.UpdateAddress38(index_sampled);
			}
			index_sampled++;
		}

		EXIT_NOT_IMPLEMENTED(((gen5 ? r.Base40() : r.Base38()) >> 32u) != 0);

		(*sgprs)[0] = r.fields[0];
		(*sgprs)[1] = r.fields[1];
		(*sgprs)[2] = r.fields[2];
		(*sgprs)[3] = r.fields[3];
		(*sgprs)[4] = r.fields[4];
		(*sgprs)[5] = r.fields[5];
		(*sgprs)[6] = r.fields[6];
		(*sgprs)[7] = r.fields[7];

		(*sgprs) += 8;
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void PrepareSamplers(const ShaderSamplerResources& samplers, uint64_t* sampler_ids, uint32_t** sgprs)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(sampler_ids == nullptr);
	EXIT_IF(sgprs == nullptr);
	EXIT_IF(*sgprs == nullptr);

	bool gen5 = Config::IsNextGen();

	for (int i = 0; i < samplers.samplers_num; i++)
	{
		auto r = samplers.samplers[i];

		// EXIT_NOT_IMPLEMENTED(r.ClampX() != 0);
		// EXIT_NOT_IMPLEMENTED(r.ClampY() != 0);
		// EXIT_NOT_IMPLEMENTED(r.ClampZ() != 0);
		// EXIT_NOT_IMPLEMENTED(r.MaxAnisoRatio() != 0);
		EXIT_NOT_IMPLEMENTED(r.DepthCompareFunc() != 0);
		EXIT_NOT_IMPLEMENTED(r.ForceUnormCoords() != false);
		EXIT_NOT_IMPLEMENTED(r.AnisoThreshold() != 0);
		EXIT_NOT_IMPLEMENTED(!gen5 && r.McCoordTrunc() != false);
		EXIT_NOT_IMPLEMENTED(r.ForceDegamma() != false);
		EXIT_NOT_IMPLEMENTED(gen5 && r.SkipDegamma() != false);
		EXIT_NOT_IMPLEMENTED(gen5 && r.PointPreclamp() != false);
		EXIT_NOT_IMPLEMENTED(gen5 && r.AnisoOverride() != false);
		EXIT_NOT_IMPLEMENTED(gen5 && r.BlendZeroPrt() != false);
		EXIT_NOT_IMPLEMENTED(r.AnisoBias() != 0);
		EXIT_NOT_IMPLEMENTED(r.TruncCoord() != false);
		EXIT_NOT_IMPLEMENTED(r.DisableCubeWrap() != false);
		EXIT_NOT_IMPLEMENTED(r.FilterMode() != 0);
		// EXIT_NOT_IMPLEMENTED(r.MinLod() != 0);
		// EXIT_NOT_IMPLEMENTED(r.MaxLod() != 4095);
		EXIT_NOT_IMPLEMENTED(r.PerfMip() != 0);
		EXIT_NOT_IMPLEMENTED(r.PerfZ() != 0);
		// EXIT_NOT_IMPLEMENTED(r.LodBias() != 0);
		EXIT_NOT_IMPLEMENTED(r.LodBiasSec() != 0);
		// EXIT_NOT_IMPLEMENTED(r.XyMagFilter() != 1);
		// EXIT_NOT_IMPLEMENTED(r.XyMinFilter() != 1);
		EXIT_NOT_IMPLEMENTED(r.ZFilter() != 1);
		// EXIT_NOT_IMPLEMENTED(r.MipFilter() != 0 && r.MipFilter() != 2);
		EXIT_NOT_IMPLEMENTED(r.BorderColorPtr() != 0);
		EXIT_NOT_IMPLEMENTED(r.BorderColorType() != 0);

		sampler_ids[i] = g_render_ctx->GetSamplerCache()->GetSamplerId(r);

		r.UpdateIndex(i);

		(*sgprs)[0] = r.fields[0];
		(*sgprs)[1] = r.fields[1];
		(*sgprs)[2] = r.fields[2];
		(*sgprs)[3] = r.fields[3];

		(*sgprs) += 4;
	}
}

static void PrepareGdsPointers(const ShaderGdsResources& gds_pointers, uint32_t** sgprs)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(sgprs == nullptr);
	EXIT_IF(*sgprs == nullptr);

	for (int i = 0; i < gds_pointers.pointers_num; i++)
	{
		auto r = gds_pointers.pointers[i];

		EXIT_NOT_IMPLEMENTED(r.Size() != 4);

		(*sgprs)[i] = r.field;
	}

	if (gds_pointers.pointers_num > 0)
	{
		(*sgprs) += static_cast<ptrdiff_t>(4 * ((gds_pointers.pointers_num - 1) / 4 + 1));
	}
}

static void PrepareDirectSgprs(const ShaderDirectSgprsResources& direct_sgprs, uint32_t** sgprs)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(sgprs == nullptr);
	EXIT_IF(*sgprs == nullptr);

	for (int i = 0; i < direct_sgprs.sgprs_num; i++)
	{
		auto r = direct_sgprs.sgprs[i];

		(*sgprs)[i] = r.field;
	}

	if (direct_sgprs.sgprs_num > 0)
	{
		(*sgprs) += static_cast<ptrdiff_t>(4 * ((direct_sgprs.sgprs_num - 1) / 4 + 1));
	}
}

static void BindDescriptors(uint64_t submit_id, CommandBuffer* buffer, VkPipelineBindPoint pipeline_bind_point, VkPipelineLayout layout,
                            const ShaderBindResources& bind, VkShaderStageFlags vk_stage, DescriptorCache::Stage stage)
{
	KYTY_PROFILER_FUNCTION();

	if (bind.push_constant_size > 0)
	{
		EXIT_NOT_IMPLEMENTED(bind.push_constant_size > DescriptorCache::PUSH_CONSTANTS_MAX * 4);
		EXIT_NOT_IMPLEMENTED(bind.storage_buffers.buffers_num > DescriptorCache::BUFFERS_MAX);
		EXIT_NOT_IMPLEMENTED((bind.textures2D.textures2d_storage_num > DescriptorCache::TEXTURES_STORAGE_MAX) ||
		                     (bind.textures2D.textures2d_sampled_num > DescriptorCache::TEXTURES_SAMPLED_MAX));
		EXIT_NOT_IMPLEMENTED(bind.textures2D.textures2d_storage_num + bind.textures2D.textures2d_sampled_num !=
		                     bind.textures2D.textures_num);
		EXIT_NOT_IMPLEMENTED(bind.samplers.samplers_num > DescriptorCache::SAMPLERS_MAX);

		bool need_descriptor = false;

		VulkanBuffer* storage_buffers[DescriptorCache::BUFFERS_MAX];
		VulkanImage*  textures2d_sampled[DescriptorCache::TEXTURES_SAMPLED_MAX];
		int           textures2d_sampled_view[DescriptorCache::TEXTURES_SAMPLED_MAX];
		VulkanImage*  textures2d_storage[DescriptorCache::TEXTURES_STORAGE_MAX];
		uint64_t      samplers[DescriptorCache::SAMPLERS_MAX];
		uint32_t      sgprs[DescriptorCache::PUSH_CONSTANTS_MAX];

		uint32_t* sgprs_ptr = sgprs;

		VulkanBuffer* gds_buffer = nullptr;

		if (bind.storage_buffers.buffers_num > 0)
		{
			PrepareStorageBuffers(submit_id, buffer, bind.storage_buffers, storage_buffers, &sgprs_ptr);
			need_descriptor = true;
		}
		if (bind.textures2D.textures_num > 0)
		{
			PrepareTextures(submit_id, buffer, bind.textures2D, textures2d_sampled, textures2d_storage, textures2d_sampled_view,
			                &sgprs_ptr);
			need_descriptor = true;
		}
		if (bind.samplers.samplers_num > 0)
		{
			PrepareSamplers(bind.samplers, samplers, &sgprs_ptr);
			need_descriptor = true;
		}
		if (bind.gds_pointers.pointers_num > 0)
		{
			PrepareGdsPointers(bind.gds_pointers, &sgprs_ptr);
			gds_buffer      = g_render_ctx->GetGdsBuffer()->GetBuffer(g_render_ctx->GetGraphicCtx());
			need_descriptor = true;
		}
		if (bind.direct_sgprs.sgprs_num > 0)
		{
			PrepareDirectSgprs(bind.direct_sgprs, &sgprs_ptr);
		}

		EXIT_IF(bind.push_constant_size != (sgprs_ptr - sgprs) * 4);

		auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

		if (bind.textures2D.textures_num > 0)
		{
			for (int i = 0; i < bind.textures2D.textures2d_sampled_num; i++)
			{
				if (textures2d_sampled[i]->type == VulkanImageType::DepthStencil)
				{
					GraphicsRenderDepthStencilBarrier(vk_buffer, textures2d_sampled[i]);
				} else if (textures2d_sampled[i]->type == VulkanImageType::RenderTexture)
				{
					GraphicsRenderRenderTextureBarrier(vk_buffer, textures2d_sampled[i]);
				}
			}
		}

		if (need_descriptor)
		{
			auto* descriptor_set = g_render_ctx->GetDescriptorCache()->GetDescriptor(
			    stage, storage_buffers, textures2d_sampled, textures2d_sampled_view, textures2d_storage, samplers, &gds_buffer, bind);

			EXIT_IF(descriptor_set == nullptr);

			vkCmdBindDescriptorSets(vk_buffer, pipeline_bind_point, layout, bind.descriptor_set_slot, 1, &descriptor_set->set, 0, nullptr);
		}
		vkCmdPushConstants(vk_buffer, layout, vk_stage, bind.push_constant_offset, bind.push_constant_size, sgprs);
	}
}

static void VulkanCmdSetColorWriteEnableEXT(GraphicContext* ctx, VkCommandBuffer command_buffer, uint32_t attachment_count,
                                            const VkBool32* p_color_write_enables)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(ctx->instance == nullptr);

	static auto func =
	    reinterpret_cast<PFN_vkCmdSetColorWriteEnableEXT>(vkGetInstanceProcAddr(ctx->instance, "vkCmdSetColorWriteEnableEXT"));

	if (func != nullptr)
	{
		func(command_buffer, attachment_count, p_color_write_enables);
	} else
	{
		EXIT("vkCmdSetColorWriteEnableEXT not present\n");
	}
}

static void SetDynamicParams(VkCommandBuffer vk_buffer, VulkanPipeline* pipeline)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(pipeline == nullptr);
	EXIT_IF(pipeline->static_params == nullptr);
	EXIT_IF(pipeline->dynamic_params == nullptr);

	if (pipeline->dynamic_params->vk_dynamic_state_line_width)
	{
		vkCmdSetLineWidth(vk_buffer, pipeline->dynamic_params->line_width);
	}

	if (pipeline->static_params->stencil_test_enable)
	{
		if (pipeline->dynamic_params->vk_dynamic_state_stencil_compare_mask)
		{
			vkCmdSetStencilCompareMask(vk_buffer, VK_STENCIL_FACE_FRONT_BIT, pipeline->dynamic_params->stencil_front.compareMask);
			vkCmdSetStencilCompareMask(vk_buffer, VK_STENCIL_FACE_BACK_BIT, pipeline->dynamic_params->stencil_back.compareMask);
		}
		if (pipeline->dynamic_params->vk_dynamic_state_stencil_write_mask)
		{
			vkCmdSetStencilWriteMask(vk_buffer, VK_STENCIL_FACE_FRONT_BIT, pipeline->dynamic_params->stencil_front.writeMask);
			vkCmdSetStencilWriteMask(vk_buffer, VK_STENCIL_FACE_BACK_BIT, pipeline->dynamic_params->stencil_back.writeMask);
		}
		if (pipeline->dynamic_params->vk_dynamic_state_stencil_reference)
		{
			vkCmdSetStencilReference(vk_buffer, VK_STENCIL_FACE_FRONT_BIT, pipeline->dynamic_params->stencil_front.reference);
			vkCmdSetStencilReference(vk_buffer, VK_STENCIL_FACE_BACK_BIT, pipeline->dynamic_params->stencil_back.reference);
		}
	}

	if (pipeline->dynamic_params->vk_dynamic_state_color_write_enable_ext)
	{
		VkBool32 enable = (pipeline->dynamic_params->color_write_enable ? VK_TRUE : VK_FALSE);
		VulkanCmdSetColorWriteEnableEXT(g_render_ctx->GetGraphicCtx(), vk_buffer, 1, &enable);
	}
}

static bool shader_is_disabled(HW::Shader* sh_ctx)
{
	if (const auto& vs = sh_ctx->GetVs();
	    !vs.vs_embedded && ((vs.vs_regs.data_addr != 0 && ShaderIsDisabled(vs.vs_regs.data_addr)) ||
	                        (vs.vs_regs.data_addr == 0 && vs.gs_regs.data_addr == 0 && vs.es_regs.data_addr != 0 &&
	                         vs.gs_regs.chksum != 0 && ShaderIsDisabled2(vs.es_regs.data_addr, vs.gs_regs.chksum))))
	{
		return true;
	}

	if (const auto& ps = sh_ctx->GetPs();
	    !ps.ps_embedded && ((ps.ps_regs.chksum == 0 && ShaderIsDisabled(ps.ps_regs.data_addr)) ||
	                        (ps.ps_regs.chksum != 0 && ShaderIsDisabled2(ps.ps_regs.data_addr, ps.ps_regs.chksum))))
	{
		return true;
	}

	return false;
}

void GraphicsRenderDrawIndex(uint64_t submit_id, CommandBuffer* buffer, HW::Context* ctx, HW::UserConfig* ucfg, HW::Shader* sh_ctx,
                             uint32_t index_type_and_size, uint32_t index_count, const void* index_addr, uint32_t flags, uint32_t type)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(ctx == nullptr);
	EXIT_IF(ucfg == nullptr);
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	if (shader_is_disabled(sh_ctx))
	{
		return;
	}

	sh_print("GraphicsRenderDrawIndex():Shader:", *sh_ctx);
	sh_check(*sh_ctx);

	uc_print("GraphicsRenderDrawIndex():UserConfig:", *ucfg);
	uc_check(*ucfg);

	hw_print(*ctx);
	hw_check(*ctx);

	EXIT_NOT_IMPLEMENTED(ctx->GetShaderStages() != 0 && ctx->GetShaderStages() != 0x02002000);

	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

	switch (ucfg->GetPrimType())
	{
		case 4: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;  // kPrimitiveTypeTriList
		case 5: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;   // kPrimitiveTypeTriFan
		case 6: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break; // kPrimitiveTypeTriStrip
		case 19: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;  // kPrimitiveTypeQuadList
		default: EXIT("unknown primitive type: %u\n", ucfg->GetPrimType());
	}

	printf("GraphicsRenderDrawIndex():Parameters:\n");
	printf("\t index_type_and_size = 0x%08" PRIx32 "\n", index_type_and_size);
	printf("\t index_count         = 0x%08" PRIx32 "\n", index_count);
	printf("\t index_addr          = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(index_addr));
	printf("\t flags               = 0x%08" PRIx32 "\n", flags);
	printf("\t type                = 0x%08" PRIx32 "\n", type);

	VkIndexType index_type = VK_INDEX_TYPE_UINT16;
	uint64_t    index_size = 0;

	switch (index_type_and_size)
	{
		case 0:
			index_type = VK_INDEX_TYPE_UINT16;
			index_size = 2 * static_cast<uint64_t>(index_count);
			break;
		case 1:
			index_type = VK_INDEX_TYPE_UINT32;
			index_size = 4 * static_cast<uint64_t>(index_count);
			break;
		default: EXIT("unknown index_type_and_size: %u\n", index_type_and_size);
	}

	EXIT_NOT_IMPLEMENTED(flags != 0);
	EXIT_NOT_IMPLEMENTED(type != 1);

	RenderDepthInfo depth_info;
	FindRenderDepthInfo(submit_id, buffer, *ctx, &depth_info);

	RenderColorInfo color_info;
	FindRenderColorInfo(submit_id, buffer, *ctx, &color_info);

	auto* framebuffer = g_render_ctx->GetFramebufferCache()->CreateFramebuffer(&color_info, &depth_info);

	EXIT_NOT_IMPLEMENTED(framebuffer == nullptr);
	EXIT_NOT_IMPLEMENTED(framebuffer->render_pass == nullptr);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	ShaderVertexInputInfo vs_input_info;
	ShaderGetInputInfoVS(&sh_ctx->GetVs(), &ctx->GetShaderRegisters(), &vs_input_info);

	ShaderPixelInputInfo ps_input_info;
	ShaderGetInputInfoPS(&sh_ctx->GetPs(), &ctx->GetShaderRegisters(), &vs_input_info, &ps_input_info);

	auto* pipeline = g_render_ctx->GetPipelineCache()->CreatePipeline(framebuffer, &color_info, &depth_info, &vs_input_info, ctx, sh_ctx,
	                                                                  &ps_input_info, topology);

	// EXIT_NOT_IMPLEMENTED(vs_input_info.buffers_num > 1);

	vkCmdBindPipeline(vk_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

	SetDynamicParams(vk_buffer, pipeline);

	for (int i = 0; i < vs_input_info.buffers_num; i++)
	{
		const auto& b    = vs_input_info.buffers[i];
		uint64_t    addr = b.addr;
		uint64_t    size = static_cast<uint64_t>(b.stride) * b.num_records;

		auto* vertices = static_cast<VulkanBuffer*>(
		    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, addr, size, VertexBufferGpuObject()));

		VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(vk_buffer, i, 1, &vertices->buffer, &offset);
	}

	BindDescriptors(submit_id, buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, vs_input_info.bind,
	                VK_SHADER_STAGE_VERTEX_BIT, DescriptorCache::Stage::Vertex);

	BindDescriptors(submit_id, buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, ps_input_info.bind,
	                VK_SHADER_STAGE_FRAGMENT_BIT, DescriptorCache::Stage::Pixel);

	VulkanBuffer* indices = static_cast<VulkanBuffer*>(GpuMemoryCreateObject(
	    submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(index_addr), index_size, IndexBufferGpuObject()));

	EXIT_NOT_IMPLEMENTED(indices == nullptr);

	vkCmdBindIndexBuffer(vk_buffer, indices->buffer, 0, index_type);

	buffer->BeginRenderPass(framebuffer, &color_info, &depth_info);

	switch (ucfg->GetPrimType())
	{
		case 4:
		case 5:
		case 6: vkCmdDrawIndexed(vk_buffer, index_count, 1, 0, 0, 0); break;
		case 19:
			EXIT_NOT_IMPLEMENTED((index_count & 0x3u) != 0);
			for (uint32_t i = 0; i < index_count; i += 4)
			{
				vkCmdDrawIndexed(vk_buffer, 4, 1, i, 0, 0);
			}
			break;
		default: EXIT("unknown primitive type: %u\n", ucfg->GetPrimType());
	}

	buffer->EndRenderPass();

	InvalidateMemoryObject(color_info);
	InvalidateMemoryObject(depth_info);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void GraphicsRenderDrawIndexAuto(uint64_t submit_id, CommandBuffer* buffer, HW::Context* ctx, HW::UserConfig* ucfg, HW::Shader* sh_ctx,
                                 uint32_t index_count, uint32_t flags)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(ctx == nullptr || ucfg == nullptr);
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(buffer == nullptr || buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	if (shader_is_disabled(sh_ctx))
	{
		return;
	}

	sh_print("GraphicsRenderDrawIndexAuto():Shader:", *sh_ctx);
	sh_check(*sh_ctx);

	uc_print("GraphicsRenderDrawIndexAuto():UserConfig:", *ucfg);
	uc_check(*ucfg);

	hw_print(*ctx);
	hw_check(*ctx);

	printf("GraphicsRenderDrawIndex():Parameters:\n");
	printf("\t index_count         = 0x%08" PRIx32 "\n", index_count);
	printf("\t flags               = 0x%08" PRIx32 "\n", flags);

	EXIT_NOT_IMPLEMENTED(flags != 0);
	EXIT_NOT_IMPLEMENTED(ctx->GetShaderStages() != 0 && ctx->GetShaderStages() != 0x02002000);

	RenderDepthInfo depth_info;
	FindRenderDepthInfo(submit_id, buffer, *ctx, &depth_info);

	RenderColorInfo color_info;
	FindRenderColorInfo(submit_id, buffer, *ctx, &color_info);

	auto* framebuffer = g_render_ctx->GetFramebufferCache()->CreateFramebuffer(&color_info, &depth_info);

	EXIT_NOT_IMPLEMENTED(framebuffer == nullptr);
	EXIT_NOT_IMPLEMENTED(framebuffer->render_pass == nullptr);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

	switch (ucfg->GetPrimType())
	{
		case 4: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;   // kPrimitiveTypeTriList
		case 17: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break; // kPrimitiveTypeRectList
		case 19: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;   // kPrimitiveTypeQuadList
		default: EXIT("unknown primitive type: %u\n", ucfg->GetPrimType());
	}

	const auto& vertex_shader_info = sh_ctx->GetVs();
	const auto& pixel_shader_info  = sh_ctx->GetPs();
	const auto& shader_regs        = ctx->GetShaderRegisters();

	ShaderVertexInputInfo vs_input_info;
	ShaderGetInputInfoVS(&vertex_shader_info, &shader_regs, &vs_input_info);

	ShaderPixelInputInfo ps_input_info;
	ShaderGetInputInfoPS(&pixel_shader_info, &shader_regs, &vs_input_info, &ps_input_info);

	auto* pipeline = g_render_ctx->GetPipelineCache()->CreatePipeline(framebuffer, &color_info, &depth_info, &vs_input_info, ctx, sh_ctx,
	                                                                  &ps_input_info, topology);

	// EXIT_NOT_IMPLEMENTED(vs_input_info.buffers_num > 1);

	vkCmdBindPipeline(vk_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

	SetDynamicParams(vk_buffer, pipeline);

	for (int i = 0; i < vs_input_info.buffers_num; i++)
	{
		const auto& b    = vs_input_info.buffers[i];
		uint64_t    addr = b.addr;
		uint64_t    size = static_cast<uint64_t>(b.stride) * b.num_records;

		auto* vertices = static_cast<VulkanBuffer*>(
		    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, addr, size, VertexBufferGpuObject()));

		VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(vk_buffer, i, 1, &vertices->buffer, &offset);
	}

	BindDescriptors(submit_id, buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, vs_input_info.bind,
	                VK_SHADER_STAGE_VERTEX_BIT, DescriptorCache::Stage::Vertex);

	BindDescriptors(submit_id, buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, ps_input_info.bind,
	                VK_SHADER_STAGE_FRAGMENT_BIT, DescriptorCache::Stage::Pixel);

	buffer->BeginRenderPass(framebuffer, &color_info, &depth_info);

	switch (ucfg->GetPrimType())
	{
		case 4: vkCmdDraw(vk_buffer, index_count, 1, 0, 0); break;
		case 17:
			EXIT_NOT_IMPLEMENTED(!(index_count == 3 && vs_input_info.buffers_num == 0));
			vkCmdDraw(vk_buffer, 4, 1, 0, 0);
			break;
		case 19:
			EXIT_NOT_IMPLEMENTED((index_count & 0x3u) != 0);
			for (uint32_t i = 0; i < index_count; i += 4)
			{
				vkCmdDraw(vk_buffer, 4, 1, i, 0);
			}
			break;
		default: EXIT("unknown primitive type: %u\n", ucfg->GetPrimType());
	}

	buffer->EndRenderPass();

	InvalidateMemoryObject(color_info);
	InvalidateMemoryObject(depth_info);
}

void GraphicsRenderDispatchDirect(uint64_t submit_id, CommandBuffer* buffer, HW::Context* ctx, HW::Shader* sh_ctx, uint32_t thread_group_x,
                                  uint32_t thread_group_y, uint32_t thread_group_z, uint32_t mode)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	if (ShaderIsDisabled(sh_ctx->GetCs().cs_regs.data_addr))
	{
		return;
	}

	EXIT_NOT_IMPLEMENTED(mode != 0);

	const auto& cs_regs = sh_ctx->GetCs();
	const auto& sh_regs = ctx->GetShaderRegisters();

	ShaderComputeInputInfo input_info;
	ShaderGetInputInfoCS(&cs_regs, &sh_regs, &input_info);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	auto* pipeline = g_render_ctx->GetPipelineCache()->CreatePipeline(&input_info, &sh_ctx->GetCs(), &ctx->GetShaderRegisters());

	vkCmdBindPipeline(vk_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);

	SetDynamicParams(vk_buffer, pipeline);

	BindDescriptors(submit_id, buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline_layout, input_info.bind,
	                VK_SHADER_STAGE_COMPUTE_BIT, DescriptorCache::Stage::Compute);

	vkCmdDispatch(vk_buffer, thread_group_x, thread_group_y, thread_group_z);
}

void GraphicsRenderWriteAtEndOfPipe32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	Graphics::LabelGpuObject label_info(value, nullptr, nullptr);

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 4, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeGds32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t dw_offset,
                                         uint32_t dw_num)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	uint64_t args[LABEL_ARGS_MAX] = {static_cast<uint64_t>(dw_offset), static_cast<uint64_t>(dw_num),
	                                 reinterpret_cast<uint64_t>(dst_gpu_addr), 0};

	Graphics::LabelGpuObject label_info(
	    0,
	    [](const uint64_t* args)
	    {
		    auto  dw_offset    = static_cast<uint32_t>(args[0]);
		    auto  dw_num       = static_cast<uint32_t>(args[1]);
		    auto* dst_gpu_addr = reinterpret_cast<uint32_t*>(args[2]);
		    g_render_ctx->GetGdsBuffer()->Read(g_render_ctx->GetGraphicCtx(), dst_gpu_addr, dw_offset, dw_num);
		    return false;
	    },
	    nullptr, args);

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 4, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipe64(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	Graphics::LabelGpuObject label_info(value, nullptr, nullptr);

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 8, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeClockCounter(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	uint64_t args[LABEL_ARGS_MAX] = {reinterpret_cast<uint64_t>(dst_gpu_addr), 0, 0, 0};

	Graphics::LabelGpuObject label_info(
	    0,
	    [](const uint64_t* args)
	    {
		    auto* dst_gpu_addr = reinterpret_cast<uint64_t*>(args[0]);
		    EXIT_IF(dst_gpu_addr == nullptr);
		    *dst_gpu_addr = LibKernel::KernelReadTsc();
		    printf(FG_BRIGHT_GREEN "EndOfPipe Signal!!! [0x%016" PRIx64 "] <- Clock: 0x%016" PRIx64 "\n" FG_DEFAULT,
		           reinterpret_cast<uint64_t>(dst_gpu_addr), *dst_gpu_addr);
		    return false;
	    },
	    nullptr, args);

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 8, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeWithWriteBack64(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	uint64_t args[LABEL_ARGS_MAX] = {reinterpret_cast<uint64_t>(buffer->GetParent())};

	Graphics::LabelGpuObject label_info(
	    value,
	    [](const uint64_t* args)
	    {
		    EXIT_IF(g_render_ctx == nullptr);

		    auto* cp = reinterpret_cast<CommandProcessor*>(args[0]);

		    GpuMemoryWriteBack(g_render_ctx->GetGraphicCtx(), cp);
		    return true;
	    },
	    nullptr, args);

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 8, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeWithInterruptWriteBack64(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr,
                                                            uint64_t value)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	uint64_t args[LABEL_ARGS_MAX] = {reinterpret_cast<uint64_t>(buffer->GetParent())};

	Graphics::LabelGpuObject label_info(
	    value,
	    [](const uint64_t* args)
	    {
		    EXIT_IF(g_render_ctx == nullptr);

		    auto* cp = reinterpret_cast<CommandProcessor*>(args[0]);

		    GpuMemoryWriteBack(g_render_ctx->GetGraphicCtx(), cp);
		    return true;
	    },
	    [](const uint64_t* /*args*/)
	    {
		    EXIT_IF(g_render_ctx == nullptr);
		    g_render_ctx->TriggerEopEvent();
		    return true;
	    },
	    args);

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 8, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeWithInterrupt64(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	Graphics::LabelGpuObject label_info(value, nullptr,
	                                    [](const uint64_t* /*args*/)
	                                    {
		                                    EXIT_IF(g_render_ctx == nullptr);
		                                    g_render_ctx->TriggerEopEvent();
		                                    return true;
	                                    });

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 8, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeWithInterrupt32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	Graphics::LabelGpuObject label_info(value, nullptr,
	                                    [](const uint64_t* /*args*/)
	                                    {
		                                    EXIT_IF(g_render_ctx == nullptr);
		                                    g_render_ctx->TriggerEopEvent();
		                                    return true;
	                                    });

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 4, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeWithInterruptWriteBackFlip32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr,
                                                                uint32_t value, int handle, int index, int flip_mode, int64_t flip_arg)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	uint64_t args[LABEL_ARGS_MAX] = {static_cast<uint64_t>(handle), static_cast<uint64_t>(index), static_cast<uint64_t>(flip_mode),
	                                 static_cast<uint64_t>(flip_arg), reinterpret_cast<uint64_t>(buffer->GetParent())};

	Graphics::LabelGpuObject label_info(
	    value,
	    [](const uint64_t* args)
	    {
		    EXIT_IF(g_render_ctx == nullptr);

		    auto* cp = reinterpret_cast<CommandProcessor*>(args[4]);

		    GpuMemoryWriteBack(g_render_ctx->GetGraphicCtx(), cp);

		    int     handle    = static_cast<int>(args[0]);
		    int     index     = static_cast<int>(args[1]);
		    int     flip_mode = static_cast<int>(args[2]);
		    int64_t flip_arg  = static_cast<int>(args[3]);

		    VideoOut::VideoOutSubmitFlip(handle, index, flip_mode, flip_arg);
		    return true;
	    },
	    [](const uint64_t* /*args*/)
	    {
		    EXIT_IF(g_render_ctx == nullptr);
		    g_render_ctx->TriggerEopEvent();
		    return true;
	    },
	    args);

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 4, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeWithFlip32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value, int handle,
                                              int index, int flip_mode, int64_t flip_arg)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	uint64_t args[LABEL_ARGS_MAX] = {static_cast<uint64_t>(handle), static_cast<uint64_t>(index), static_cast<uint64_t>(flip_mode),
	                                 static_cast<uint64_t>(flip_arg)};

	Graphics::LabelGpuObject label_info(
	    value,
	    [](const uint64_t* args)
	    {
		    int     handle    = static_cast<int>(args[0]);
		    int     index     = static_cast<int>(args[1]);
		    int     flip_mode = static_cast<int>(args[2]);
		    int64_t flip_arg  = static_cast<int>(args[3]);

		    VideoOut::VideoOutSubmitFlip(handle, index, flip_mode, flip_arg);
		    return true;
	    },
	    nullptr, args);

	auto* label = static_cast<Label*>(
	    GpuMemoryCreateObject(submit_id, g_render_ctx->GetGraphicCtx(), nullptr, reinterpret_cast<uint64_t>(dst_gpu_addr), 4, label_info));

	LabelSet(buffer, label);
}

void GraphicsRenderWriteAtEndOfPipeOnlyFlip(uint64_t /*submit_id*/, CommandBuffer* buffer, int handle, int index, int flip_mode,
                                            int64_t flip_arg)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());

	Core::LockGuard lock(g_render_ctx->GetMutex());

	uint64_t args[LABEL_ARGS_MAX] = {static_cast<uint64_t>(handle), static_cast<uint64_t>(index), static_cast<uint64_t>(flip_mode),
	                                 static_cast<uint64_t>(flip_arg)};

	auto* label = LabelCreate32(
	    g_render_ctx->GetGraphicCtx(), nullptr, 0,
	    [](const uint64_t* args)
	    {
		    int     handle    = static_cast<int>(args[0]);
		    int     index     = static_cast<int>(args[1]);
		    int     flip_mode = static_cast<int>(args[2]);
		    int64_t flip_arg  = static_cast<int>(args[3]);

		    VideoOut::VideoOutSubmitFlip(handle, index, flip_mode, flip_arg);
		    return true;
	    },
	    nullptr, args);

	LabelSet(buffer, label);

	LabelDelete(label);
}

void GraphicsRenderWriteBack(CommandProcessor* cp)
{
	Core::LockGuard lock(g_render_ctx->GetMutex());

	EXIT_IF(g_render_ctx == nullptr);

	GpuMemoryWriteBack(g_render_ctx->GetGraphicCtx(), cp);
}

static void eop_event_reset_func(LibKernel::EventQueue::KernelEqueueEvent* event)
{
	EXIT_IF(event == nullptr);
	event->triggered    = false;
	event->event.fflags = 0;
	event->event.data   = 0;
}

static void eop_event_delete_func(LibKernel::EventQueue::KernelEqueue eq, LibKernel::EventQueue::KernelEqueueEvent* event)
{
	EXIT_IF(event == nullptr);
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_NOT_IMPLEMENTED(event->event.ident != GRAPHICS_EVENT_EOP);
	EXIT_NOT_IMPLEMENTED(event->event.filter != LibKernel::EventQueue::KERNEL_EVFILT_GRAPHICS);
	g_render_ctx->DeleteEopEq(eq);
}

static void eop_event_trigger_func(LibKernel::EventQueue::KernelEqueueEvent* event, void* trigger_data)
{
	EXIT_IF(event == nullptr);
	event->triggered = true;
	event->event.fflags++;
	event->event.data = reinterpret_cast<intptr_t>(trigger_data);
}

int GraphicsRenderAddEqEvent(LibKernel::EventQueue::KernelEqueue eq, int id, void* udata)
{
	EXIT_IF(g_render_ctx == nullptr);

	EXIT_NOT_IMPLEMENTED(id != GRAPHICS_EVENT_EOP);

	LibKernel::EventQueue::KernelEqueueEvent event;
	event.triggered                = false;
	event.event.ident              = GRAPHICS_EVENT_EOP;
	event.event.filter             = LibKernel::EventQueue::KERNEL_EVFILT_GRAPHICS;
	event.event.udata              = udata;
	event.event.fflags             = 0;
	event.event.data               = 0;
	event.filter.delete_event_func = eop_event_delete_func;
	event.filter.reset_func        = eop_event_reset_func;
	event.filter.trigger_func      = eop_event_trigger_func;
	event.filter.data              = nullptr;

	int result = LibKernel::EventQueue::KernelAddEvent(eq, event);

	g_render_ctx->AddEopEq(eq);

	return result;
}

int GraphicsRenderDeleteEqEvent(LibKernel::EventQueue::KernelEqueue eq, int id)
{
	EXIT_IF(g_render_ctx == nullptr);

	EXIT_NOT_IMPLEMENTED(id != GRAPHICS_EVENT_EOP);

	int result = LibKernel::EventQueue::KernelDeleteEvent(eq, GRAPHICS_EVENT_EOP, LibKernel::EventQueue::KERNEL_EVFILT_GRAPHICS);

	g_render_ctx->DeleteEopEq(eq);

	return result;
}

void GraphicsRenderClearGds(uint64_t dw_offset, uint32_t dw_num, uint32_t clear_value)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(g_render_ctx->GetGdsBuffer() == nullptr);

	g_render_ctx->GetGdsBuffer()->Clear(g_render_ctx->GetGraphicCtx(), dw_offset, dw_num, clear_value);
}

void GraphicsRenderReadGds(uint32_t* dst, uint32_t dw_offset, uint32_t dw_size)
{
	EXIT_IF(g_render_ctx == nullptr);
	EXIT_IF(g_render_ctx->GetGdsBuffer() == nullptr);

	g_render_ctx->GetGdsBuffer()->Read(g_render_ctx->GetGraphicCtx(), dst, dw_offset, dw_size);
}

void GraphicsRenderMemoryFree(uint64_t vaddr, uint64_t size)
{
	GpuMemoryFree(g_render_ctx->GetGraphicCtx(), vaddr, size, false);
}

void GraphicsRenderDeleteIndexBuffers()
{
	IndexBufferDeleteAll(g_render_ctx->GetGraphicCtx());
}

void GraphicsRenderMemoryFlush(uint64_t vaddr, uint64_t size)
{
	GpuMemoryFlush(g_render_ctx->GetGraphicCtx(), vaddr, size);
}

bool CommandBuffer::IsInvalid() const
{
	EXIT_IF(g_render_ctx == nullptr);

	if (m_pool != nullptr)
	{
		Core::LockGuard lock(m_pool->mutex);

		return (m_index == static_cast<uint32_t>(-1) || m_index >= m_pool->buffers_count);
	}

	return true;
}

void CommandBuffer::Allocate()
{
	EXIT_IF(!IsInvalid());

	m_pool = g_command_pool.GetPool(m_queue);

	Core::LockGuard lock(m_pool->mutex);

	for (uint32_t i = 0; i < m_pool->buffers_count; i++)
	{
		if (!m_pool->busy[i])
		{
			m_pool->busy[i] = true;
			vkResetCommandBuffer(m_pool->buffers[i], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
			m_index = i;
			break;
		}
	}

	EXIT_NOT_IMPLEMENTED(IsInvalid());
}

void CommandBuffer::Free()
{
	EXIT_IF(IsInvalid());

	Core::LockGuard lock(m_pool->mutex);

	WaitForFence();

	m_pool->busy[m_index] = false;
	vkResetCommandBuffer(m_pool->buffers[m_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	m_index = static_cast<uint32_t>(-1);

	EXIT_NOT_IMPLEMENTED(!IsInvalid());
}

void CommandBuffer::Begin() const
{
	EXIT_IF(IsInvalid());

	auto* buffer = m_pool->buffers[m_index];

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext            = nullptr;
	begin_info.flags            = 0;
	begin_info.pInheritanceInfo = nullptr;

	auto result = vkBeginCommandBuffer(buffer, &begin_info);

	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);
}

void CommandBuffer::End() const
{
	EXIT_IF(IsInvalid());

	auto* buffer = m_pool->buffers[m_index];

	auto result = vkEndCommandBuffer(buffer);

	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);
}

void CommandBuffer::Execute()
{
	EXIT_IF(IsInvalid());

	auto* buffer = m_pool->buffers[m_index];
	auto* fence  = m_pool->fences[m_index];

	VkSubmitInfo submit_info {};
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext                = nullptr;
	submit_info.waitSemaphoreCount   = 0;
	submit_info.pWaitSemaphores      = nullptr;
	submit_info.pWaitDstStageMask    = nullptr;
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &buffer;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores    = nullptr;

	EXIT_IF(m_queue < 0 || m_queue >= GraphicContext::QUEUES_NUM);

	const auto& queue = g_render_ctx->GetGraphicCtx()->queues[m_queue];

	if (queue.mutex != nullptr)
	{
		queue.mutex->Lock();
	}

	auto result = vkQueueSubmit(queue.vk_queue, 1, &submit_info, fence);

	if (queue.mutex != nullptr)
	{
		queue.mutex->Unlock();
	}

	m_execute = true;

	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);
}

void CommandBuffer::ExecuteWithSemaphore()
{
	EXIT_IF(IsInvalid());

	auto* buffer = m_pool->buffers[m_index];
	auto* fence  = m_pool->fences[m_index];

	VkSubmitInfo submit_info {};
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext                = nullptr;
	submit_info.waitSemaphoreCount   = 0;
	submit_info.pWaitSemaphores      = nullptr;
	submit_info.pWaitDstStageMask    = nullptr;
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &buffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &m_pool->semaphores[m_index];

	EXIT_IF(m_queue < 0 || m_queue >= GraphicContext::QUEUES_NUM);

	const auto& queue = g_render_ctx->GetGraphicCtx()->queues[m_queue];

	if (queue.mutex != nullptr)
	{
		queue.mutex->Lock();
	}

	auto result = vkQueueSubmit(queue.vk_queue, 1, &submit_info, fence);

	if (queue.mutex != nullptr)
	{
		queue.mutex->Lock();
	}

	m_execute = true;

	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);
}

void CommandBuffer::WaitForFence()
{
	EXIT_IF(IsInvalid());

	if (m_execute)
	{
		auto* device = g_render_ctx->GetGraphicCtx()->device;

		vkWaitForFences(device, 1, &m_pool->fences[m_index], VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &m_pool->fences[m_index]);

		m_execute = false;
	}
}

void CommandBuffer::WaitForFenceAndReset()
{
	EXIT_IF(IsInvalid());

	if (m_execute)
	{
		auto* device = g_render_ctx->GetGraphicCtx()->device;

		vkWaitForFences(device, 1, &m_pool->fences[m_index], VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &m_pool->fences[m_index]);
		vkResetCommandBuffer(m_pool->buffers[m_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

		m_execute = false;
	}
}

void CommandBuffer::BeginRenderPass(VulkanFramebuffer* framebuffer, RenderColorInfo* color, RenderDepthInfo* depth) const
{
	EXIT_IF(IsInvalid());

	auto* buffer = m_pool->buffers[m_index];

	EXIT_IF(framebuffer == nullptr);

	bool with_depth = (depth->format != VK_FORMAT_UNDEFINED && depth->vulkan_buffer != nullptr);
	bool with_color = (color->vulkan_buffer != nullptr);

	EXIT_NOT_IMPLEMENTED(!with_depth && !with_color);

	VkClearValue clears[2];
	clears[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clears[1].depthStencil = {depth->depth_clear_value, depth->stencil_clear_value};

	VkExtent2D extent = (with_color ? color->vulkan_buffer->extent : depth->vulkan_buffer->extent);

	VkRenderPassBeginInfo render_pass_info {};
	render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.pNext             = nullptr;
	render_pass_info.renderPass        = framebuffer->render_pass;
	render_pass_info.framebuffer       = framebuffer->framebuffer;
	render_pass_info.renderArea.offset = {0, 0};
	render_pass_info.renderArea.extent = extent;
	render_pass_info.clearValueCount   = with_depth ? 2 : 1;
	render_pass_info.pClearValues      = clears;

	if (with_color && color->vulkan_buffer->layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		VkImageMemoryBarrier image_memory_barrier {};
		image_memory_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext                           = nullptr;
		image_memory_barrier.srcAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
		image_memory_barrier.dstAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
		image_memory_barrier.oldLayout                       = color->vulkan_buffer->layout;
		image_memory_barrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image                           = color->vulkan_buffer->image;
		image_memory_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseMipLevel   = 0;
		image_memory_barrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount     = 1;

		vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                     VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
		                     &image_memory_barrier);

		color->vulkan_buffer->layout = image_memory_barrier.newLayout;
	}

	if (with_depth && depth->vulkan_buffer->layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		VkImageMemoryBarrier image_memory_barrier {};
		image_memory_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext                           = nullptr;
		image_memory_barrier.srcAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
		image_memory_barrier.dstAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
		image_memory_barrier.oldLayout                       = depth->vulkan_buffer->layout;
		image_memory_barrier.newLayout                       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image                           = depth->vulkan_buffer->image;
		image_memory_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
		image_memory_barrier.subresourceRange.baseMipLevel   = 0;
		image_memory_barrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount     = 1;

		vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                     VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
		                     &image_memory_barrier);

		depth->vulkan_buffer->layout = image_memory_barrier.newLayout;
	}

	vkCmdBeginRenderPass(buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::EndRenderPass() const
{
	EXIT_IF(IsInvalid());

	auto* buffer = m_pool->buffers[m_index];

	vkCmdEndRenderPass(buffer);
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
