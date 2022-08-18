#include "Emulator/Graphics/GraphicsRun.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/LinkList.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/AsyncJob.h"
#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/Graphics.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/HardwareContext.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"
#include "Emulator/Graphics/Pm4.h"
#include "Emulator/Graphics/VideoOut.h"
#include "Emulator/Graphics/Window.h"
#include "Emulator/Profiler.h"

#include <atomic>

#ifdef KYTY_EMU_ENABLED

#define KYTY_HW_CTX_PARSER_ARGS                                                                                                            \
	[[maybe_unused]] CommandProcessor *cp, uint32_t cmd_id, [[maybe_unused]] uint32_t cmd_offset, const uint32_t *buffer,                  \
	    [[maybe_unused]] uint32_t dw
#define KYTY_HW_UC_PARSER_ARGS                                                                                                             \
	[[maybe_unused]] CommandProcessor *cp, uint32_t cmd_id, [[maybe_unused]] uint32_t cmd_offset, const uint32_t *buffer,                  \
	    [[maybe_unused]] uint32_t dw
#define KYTY_HW_SH_PARSER_ARGS                                                                                                             \
	[[maybe_unused]] CommandProcessor *cp, uint32_t cmd_id, [[maybe_unused]] uint32_t cmd_offset, const uint32_t *buffer,                  \
	    [[maybe_unused]] uint32_t dw
#define KYTY_HW_CTX_INDIRECT_ARGS CommandProcessor *cp, [[maybe_unused]] uint32_t cmd_offset, uint32_t value
#define KYTY_HW_UC_INDIRECT_ARGS  CommandProcessor *cp, [[maybe_unused]] uint32_t cmd_offset, uint32_t value
#define KYTY_HW_SH_INDIRECT_ARGS  CommandProcessor *cp, [[maybe_unused]] uint32_t cmd_offset, uint32_t value

#define KYTY_HW_CTX_PARSER(f) static uint32_t f(KYTY_HW_CTX_PARSER_ARGS)
#define KYTY_HW_UC_PARSER(f)  static uint32_t f(KYTY_HW_UC_PARSER_ARGS)
#define KYTY_HW_SH_PARSER(f)  static uint32_t f(KYTY_HW_SH_PARSER_ARGS)

#define KYTY_CP_OP_PARSER_ARGS                                                                                                             \
	[[maybe_unused]] CommandProcessor *cp, [[maybe_unused]] uint32_t cmd_id, [[maybe_unused]] const uint32_t *buffer,                      \
	    [[maybe_unused]] uint32_t dw, [[maybe_unused]] uint32_t num_dw
#define KYTY_CP_OP_PARSER(f) static uint32_t f(KYTY_CP_OP_PARSER_ARGS)

namespace Kyty::Libs::Graphics {

class CommandProcessor
{
public:
	struct FlipInfo
	{
		int     handle    = 0;
		int     index     = 0;
		int     flip_mode = 0;
		int64_t flip_arg  = 0;
	};

	CommandProcessor() = default;
	virtual ~CommandProcessor() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(CommandProcessor);

	void Reset();

	void BufferInit();
	void BufferFlush();
	void BufferWait();

	void RunLock() { m_run_mutex.Lock(); }
	void RunUnlock() { m_run_mutex.Unlock(); }
	void Lock() { m_mutex.Lock(); }
	void Unlock() { m_mutex.Unlock(); }

	HW::Context*    GetCtx() { return &m_ctx; }
	HW::UserConfig* GetUcfg() { return &m_ucfg; }
	HW::Shader*     GetShCtx() { return &m_sh_ctx; }

	void SetIndexType(uint32_t index_type_and_size);
	void SetNumInstances(uint32_t num_instances);
	void DrawIndex(uint32_t index_count, const void* index_addr, uint32_t flags, uint32_t type);
	void DrawIndexAuto(uint32_t index_count, uint32_t flags);
	void WriteAtEndOfPipe32(uint32_t cache_policy, uint32_t event_write_dest, uint32_t eop_event_type, uint32_t cache_action,
	                        uint32_t event_index, uint32_t event_write_source, void* dst_gpu_addr, uint32_t value,
	                        uint32_t interrupt_selector);
	void WriteAtEndOfPipe64(uint32_t cache_policy, uint32_t event_write_dest, uint32_t eop_event_type, uint32_t cache_action,
	                        uint32_t event_index, uint32_t event_write_source, void* dst_gpu_addr, uint64_t value,
	                        uint32_t interrupt_selector);
	void Flip();
	void Flip(void* dst_gpu_addr, uint32_t value);
	void FlipWithInterrupt(uint32_t eop_event_type, uint32_t cache_action, void* dst_gpu_addr, uint32_t value);
	void WriteBack();
	void MemoryBarrier();
	void RenderTextureBarrier(uint64_t vaddr, uint64_t size);
	void DepthStencilBarrier(uint64_t vaddr, uint64_t size);
	void DispatchDirect(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z, uint32_t mode);
	void WaitFlipDone(uint32_t video_out_handle, uint32_t display_buffer_index);
	void TriggerEvent(uint32_t event_type, uint32_t event_index);

	void                           SetUserDataMarker(HW::UserSgprType type) { m_user_data_marker = type; }
	[[nodiscard]] HW::UserSgprType GetUserDataMarker() const { return m_user_data_marker; }
	void                           SetEmbeddedDataMarker(const uint32_t* buffer, uint32_t num_dw, uint32_t align) {}
	void                           PushMarker(const char* str) {}
	void                           PopMarker() {}

	void PrefetchL2(void* addr, uint32_t size) {}
	void ClearGds(uint64_t dw_offset, uint32_t dw_num, uint32_t clear_value);
	void ReadGds(uint32_t* dst, uint32_t dw_offset, uint32_t dw_size);

	void ResetDeCe();
	void WaitCe();
	void WaitDeDiff(uint32_t diff);
	void IncremenetDe();
	void IncremenetCe();

	void WriteConstRam(uint32_t offset, const uint32_t* src, uint32_t dw_num);
	void DumpConstRam(uint32_t* dst, uint32_t offset, uint32_t dw_num);

	void WaitRegMem32(uint32_t func, const uint32_t* addr, uint32_t ref, uint32_t mask, uint32_t poll);
	void WaitRegMem64(uint32_t func, const uint64_t* addr, uint64_t ref, uint64_t mask, uint32_t poll);
	void WriteData(uint32_t* dst, const uint32_t* src, uint32_t dw_num, uint32_t write_control, bool custom);

	void Run(uint32_t* data, uint32_t num_dw);

	void SetQueue(int queue) { m_queue = queue; }

	[[nodiscard]] const FlipInfo& GetFlip() const { return m_flip; }
	void                          SetFlip(const FlipInfo& flip) { m_flip = flip; }

	[[nodiscard]] uint64_t GetSumbitId() const { return m_sumbit_id; }
	void                   SetSumbitId(uint64_t sumbit_id) { m_sumbit_id = sumbit_id; }

private:
	static constexpr int VK_BUFFERS_NUM = 4;

	struct Counter
	{
		Core::Mutex   mutex;
		Core::CondVar cond_var;
		uint32_t      value = 0;
	};

	HW::Context      m_ctx;
	HW::UserConfig   m_ucfg;
	HW::Shader       m_sh_ctx;
	HW::UserSgprType m_user_data_marker    = HW::UserSgprType::Unknown;
	uint32_t         m_index_type_and_size = 0;
	uint32_t         m_num_instances       = 1;

	Core::Mutex m_mutex;
	Core::Mutex m_run_mutex;

	CommandBuffer* m_buffer[VK_BUFFERS_NUM] = {};
	int            m_current_buffer         = -1;
	int            m_queue                  = -1;

	Counter m_de_counter;
	Counter m_ce_counter;

	uint32_t m_const_ram[0x3000] = {0};

	FlipInfo m_flip;
	uint64_t m_sumbit_id = 0;
};

class GraphicsRing
{
public:
	GraphicsRing(): m_job1("Thread_Gfx_Draw"), m_job2("Thread_Gfx_Const") { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~GraphicsRing() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(GraphicsRing);

	void Submit(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw, int handle, int index,
	            int flip_mode, int64_t flip_arg);
	void Done();
	void WaitForIdle();
	bool IsIdle();

	void SetCp(CommandProcessor* cp)
	{
		m_cp = cp;
		Start();
	}

private:
	void Start()
	{
		Core::Thread t(ThreadBatchRun, this);
		t.Detach();
	}

	struct CmdBuffer
	{
		uint32_t* data   = nullptr;
		uint32_t  num_dw = 0;
	};

	struct CmdBatch
	{
		CmdBuffer draw_buffer;
		CmdBuffer const_buffer;

		CommandProcessor::FlipInfo flip;
	};

	static void ThreadBatchRun(void* data);

	CmdBatch GetCmdBatch();

	Core::Mutex          m_mutex;
	Core::CondVar        m_cond_var;
	Core::CondVar        m_idle_cond_var;
	Core::List<CmdBatch> m_cmd_batches;
	bool                 m_done = true;
	bool                 m_idle = true;

	AsyncJob m_job1;
	AsyncJob m_job2;

	CommandProcessor* m_cp = nullptr;
};

class ComputeRing
{
public:
	ComputeRing() = default;
	virtual ~ComputeRing() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(ComputeRing);

	void MapComputeQueue(uint32_t* ring_addr, uint32_t ring_size_dw, uint32_t* read_ptr_addr);
	void DingDong(uint32_t offset_dw);
	void Done();
	void WaitForIdle();
	bool IsIdle();

	void SetCp(CommandProcessor* cp)
	{
		m_cp = cp;
		Start();
	}

	[[nodiscard]] int GetQueueId() const { return m_queue_id; }
	void              SetQueueId(int id) { m_queue_id = id; }

	void SetActive(bool flag);
	bool IsActive();

private:
	void Start()
	{
		Core::Thread t(ThreadRun, this);
		t.Detach();
	}

	static void ThreadRun(void* data);

	Core::Mutex   m_mutex;
	Core::CondVar m_cond_var;
	Core::CondVar m_idle_cond_var;
	bool          m_done = true;
	bool          m_idle = true;

	CommandProcessor* m_cp       = nullptr;
	int               m_queue_id = -1;

	bool      m_active                  = false;
	uint32_t* m_ring_addr               = nullptr;
	uint32_t* m_read_ptr_addr           = nullptr;
	uint32_t* m_internal_buffer         = nullptr;
	uint32_t  m_ring_size_dw            = 0;
	uint32_t  m_run_offset_dw           = 0;
	uint32_t  m_last_dingdong_offset_dw = 0;
};

class Gpu
{
public:
	Gpu()
	{
		EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
		Init();
	}
	virtual ~Gpu() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(Gpu);

	void     Submit(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw);
	void     SubmitAndFlip(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw, int handle,
	                       int index, int flip_mode, int64_t flip_arg);
	uint32_t MapComputeQueue(uint32_t pipe_id, uint32_t queue_id, uint32_t* ring_addr, uint32_t ring_size_dw, uint32_t* read_ptr_addr);
	void     Unmap(uint32_t ring_id);
	void     DingDong(uint32_t ring_id, uint32_t offset_dw);
	void     Done();
	void     Wait();
	int      GetFrameNum();
	bool     AreSubmitsAllowed();

private:
	void Init();

	ComputeRing* GetRing(uint32_t ring_id);

	Core::Mutex m_mutex;

	CommandProcessor* m_gfx_cp   = nullptr;
	GraphicsRing*     m_gfx_ring = nullptr;

	CommandProcessor* m_compute_cp[8]    = {};
	ComputeRing*      m_compute_ring[64] = {};

	std::atomic_int m_done_num = 0;
};

using hw_ctx_parser_func_t   = uint32_t (*)(KYTY_HW_CTX_PARSER_ARGS);
using hw_uc_parser_func_t    = uint32_t (*)(KYTY_HW_UC_PARSER_ARGS);
using hw_sh_parser_func_t    = uint32_t (*)(KYTY_HW_SH_PARSER_ARGS);
using cp_op_parser_func_t    = uint32_t (*)(KYTY_CP_OP_PARSER_ARGS);
using hw_ctx_indirect_func_t = void (*)(KYTY_HW_CTX_INDIRECT_ARGS);
using hw_uc_indirect_func_t  = void (*)(KYTY_HW_UC_INDIRECT_ARGS);
using hw_sh_indirect_func_t  = void (*)(KYTY_HW_SH_INDIRECT_ARGS);

static hw_ctx_parser_func_t   g_hw_ctx_func[Pm4::CX_NUM]          = {};
static hw_ctx_indirect_func_t g_hw_ctx_indirect_func[Pm4::CX_NUM] = {};
static hw_sh_parser_func_t    g_hw_sh_func[Pm4::SH_NUM]           = {};
static hw_sh_indirect_func_t  g_hw_sh_indirect_func[Pm4::SH_NUM]  = {};
static hw_uc_parser_func_t    g_hw_uc_func[Pm4::UC_NUM]           = {};
static hw_uc_indirect_func_t  g_hw_uc_indirect_func[Pm4::UC_NUM]  = {};
static hw_sh_parser_func_t    g_hw_sh_custom_func[Pm4::R_NUM]     = {};
static cp_op_parser_func_t    g_cp_op_func[256]                   = {};
static cp_op_parser_func_t    g_cp_op_custom_func[Pm4::R_NUM]     = {};

static Gpu* g_gpu = nullptr;

static void graphics_init_jmp_tables();

void GraphicsRunInit()
{
	EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	EXIT_IF(g_gpu != nullptr);

	graphics_init_jmp_tables();

	g_gpu = new Gpu;
}

void Gpu::Submit(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw)
{
	Core::LockGuard lock(m_mutex);

	m_gfx_ring->Submit(cmd_draw_buffer, num_draw_dw, cmd_const_buffer, num_const_dw, 0, 0, 0, 0);
}

void Gpu::SubmitAndFlip(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw, int handle,
                        int index, int flip_mode, int64_t flip_arg)
{
	m_gfx_ring->Submit(cmd_draw_buffer, num_draw_dw, cmd_const_buffer, num_const_dw, handle, index, flip_mode, flip_arg);
}

uint32_t Gpu::MapComputeQueue(uint32_t pipe_id, uint32_t queue_id, uint32_t* ring_addr, uint32_t ring_size_dw, uint32_t* read_ptr_addr)
{
	EXIT_NOT_IMPLEMENTED(ring_addr == nullptr);
	EXIT_NOT_IMPLEMENTED(ring_size_dw == 0);
	EXIT_NOT_IMPLEMENTED(read_ptr_addr == nullptr);

	Core::LockGuard lock(m_mutex);

	auto v = pipe_id * 8 + queue_id;

	EXIT_NOT_IMPLEMENTED(v >= 64);

	//	for (int i = 0; i < 8; i++)
	//	{
	//		EXIT_NOT_IMPLEMENTED(m_compute_ring[pipe_id * 8 + i] != nullptr && m_compute_ring[pipe_id * 8 + i]->IsActive());
	//	}

	auto* ring = GetRing(v + 1);

	ring->MapComputeQueue(ring_addr, ring_size_dw, read_ptr_addr);

	EXIT_IF(!ring->IsActive());

	*read_ptr_addr = 0;

	return v + 1;
}

void Gpu::Unmap(uint32_t ring_id)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(ring_id < 1 || ring_id > 64);

	auto* ring = GetRing(ring_id);

	EXIT_NOT_IMPLEMENTED(!ring->IsActive());

	ring->SetActive(false);
}

void Gpu::DingDong(uint32_t ring_id, uint32_t offset_dw)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(ring_id < 1 || ring_id > 64);
	EXIT_NOT_IMPLEMENTED(offset_dw == 0);

	auto* ring = GetRing(ring_id);

	EXIT_NOT_IMPLEMENTED(!ring->IsActive());

	ring->DingDong(offset_dw);
}

void Gpu::Done()
{
	Core::LockGuard lock(m_mutex);

	m_gfx_ring->Done();
	for (auto& cr: m_compute_ring)
	{
		if (cr != nullptr)
		{
			cr->Done();
		}
	}

	m_done_num++;
}

bool Gpu::AreSubmitsAllowed()
{
	Core::LockGuard lock(m_mutex);

	if (m_gfx_ring->IsIdle())
	{
		for (auto& cr: m_compute_ring)
		{
			if (cr != nullptr)
			{
				if (!cr->IsIdle())
				{
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

int Gpu::GetFrameNum()
{
	// Core::LockGuard lock(m_mutex);

	return m_done_num;
}

void Gpu::Wait()
{
	Core::LockGuard lock(m_mutex);

	m_gfx_ring->WaitForIdle();
	m_gfx_cp->BufferWait();
	for (auto& cr: m_compute_ring)
	{
		if (cr != nullptr)
		{
			cr->WaitForIdle();
		}
	}
	for (auto& cp: m_compute_cp)
	{
		if (cp != nullptr)
		{
			cp->BufferWait();
		}
	}
}

void Gpu::Init()
{
	EXIT_IF(m_gfx_cp != nullptr);
	EXIT_IF(m_gfx_ring != nullptr);

	m_gfx_cp   = new CommandProcessor;
	m_gfx_ring = new GraphicsRing;
	m_gfx_cp->SetQueue(GraphicContext::QUEUE_GFX);
	m_gfx_ring->SetCp(m_gfx_cp);

	EXIT_IF(GraphicContext::QUEUE_COMPUTE_NUM < 8);
}

ComputeRing* Gpu::GetRing(uint32_t ring_id)
{
	int v        = static_cast<int>(ring_id - 1);
	int pipe_id  = v / 8;
	int queue_id = v % 8;

	if (m_compute_cp[pipe_id] == nullptr)
	{
		m_compute_cp[pipe_id] = new CommandProcessor;
		m_compute_cp[pipe_id]->SetQueue(GraphicContext::QUEUE_COMPUTE_START + pipe_id);
	}

	if (m_compute_ring[v] == nullptr)
	{
		m_compute_ring[v] = new ComputeRing;
		m_compute_ring[v]->SetQueueId(queue_id);
		m_compute_ring[v]->SetActive(false);
		m_compute_ring[v]->SetCp(m_compute_cp[pipe_id]);
	}

	return m_compute_ring[v];
}

void CommandProcessor::Reset()
{
	Core::LockGuard lock(m_mutex);

	BufferWait();

	GraphicsRenderDeleteIndexBuffers();

	m_sh_ctx.Reset();
	m_ucfg.Reset();
	m_ctx.Reset();
	m_index_type_and_size = 0;
	m_user_data_marker    = HW::UserSgprType::Unknown;

	std::memset(m_const_ram, 0, sizeof(m_const_ram));
}

void CommandProcessor::BufferInit()
{
	Core::LockGuard lock(m_mutex);

	if (m_current_buffer < 0)
	{
		for (auto& buf: m_buffer)
		{
			EXIT_IF(buf != nullptr);

			buf = new CommandBuffer(m_queue);
			buf->SetParent(this);
			// buf->SetQueue(m_queue);
		}

		m_current_buffer = 0;
		m_buffer[m_current_buffer]->Begin();
	}
}

void CommandProcessor::BufferFlush()
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);
	EXIT_IF(m_buffer[m_current_buffer] == nullptr);

	m_buffer[m_current_buffer]->End();
	m_buffer[m_current_buffer]->Execute();

	m_current_buffer = (m_current_buffer + 1) % VK_BUFFERS_NUM;

	EXIT_IF(m_buffer[m_current_buffer] == nullptr);

	m_buffer[m_current_buffer]->WaitForFenceAndReset();
	m_buffer[m_current_buffer]->Begin();
}

void CommandProcessor::BufferWait()
{
	BufferInit();

	Core::LockGuard lock(m_mutex);

	for (auto& buf: m_buffer)
	{
		EXIT_IF(buf == nullptr);

		buf->WaitForFenceAndReset();
	}
}

void CommandProcessor::ResetDeCe()
{
	m_de_counter.mutex.Lock();
	m_de_counter.value = 0;
	m_de_counter.cond_var.Signal();
	m_de_counter.mutex.Unlock();
	m_ce_counter.mutex.Lock();
	m_ce_counter.value = 0;
	m_ce_counter.cond_var.Signal();
	m_ce_counter.mutex.Unlock();
}

void CommandProcessor::WaitCe()
{
	m_de_counter.mutex.Lock();
	auto de_value = m_de_counter.value;
	m_de_counter.mutex.Unlock();

	m_ce_counter.mutex.Lock();
	while (!(m_ce_counter.value > de_value))
	{
		m_ce_counter.cond_var.Wait(&m_ce_counter.mutex);
	}
	m_ce_counter.mutex.Unlock();
}

void CommandProcessor::WaitDeDiff(uint32_t diff)
{
	m_ce_counter.mutex.Lock();
	auto ce_value = m_ce_counter.value;
	m_ce_counter.mutex.Unlock();

	m_de_counter.mutex.Lock();
	while (!(ce_value - m_de_counter.value < diff))
	{
		m_de_counter.cond_var.Wait(&m_de_counter.mutex);
	}
	m_de_counter.mutex.Unlock();
}

void CommandProcessor::IncremenetDe()
{
	BufferFlush();
	BufferWait();

	m_de_counter.mutex.Lock();
	m_de_counter.value++;
	m_de_counter.cond_var.Signal();
	m_de_counter.mutex.Unlock();
}

void CommandProcessor::IncremenetCe()
{
	m_ce_counter.mutex.Lock();
	m_ce_counter.value++;
	m_ce_counter.cond_var.Signal();
	m_ce_counter.mutex.Unlock();
}

void CommandProcessor::WriteConstRam(uint32_t offset, const uint32_t* src, uint32_t dw_num)
{
	Core::LockGuard lock(m_mutex);

	memcpy(m_const_ram + offset / 4, src, static_cast<size_t>(dw_num) * 4);
}

void CommandProcessor::DumpConstRam(uint32_t* dst, uint32_t offset, uint32_t dw_num)
{
	Core::LockGuard lock(m_mutex);

	GpuMemoryCheckAccessViolation(reinterpret_cast<uint64_t>(dst), static_cast<size_t>(dw_num) * 4);

	memcpy(dst, m_const_ram + offset / 4, static_cast<size_t>(dw_num) * 4);

	GraphicsRenderMemoryFlush(reinterpret_cast<uint64_t>(dst), static_cast<size_t>(dw_num) * 4);
}

void CommandProcessor::WaitRegMem32(uint32_t func, const uint32_t* addr, uint32_t ref, uint32_t mask, uint32_t poll)
{
	EXIT_NOT_IMPLEMENTED(func != 3);
	EXIT_NOT_IMPLEMENTED(poll != 10);

	BufferFlush();

	while (((*addr) & mask) != ref)
	{
		Core::Thread::SleepMicro(10);
	}
}

void CommandProcessor::WaitRegMem64(uint32_t func, const uint64_t* addr, uint64_t ref, uint64_t mask, uint32_t poll)
{
	EXIT_NOT_IMPLEMENTED(func != 3);
	EXIT_NOT_IMPLEMENTED(poll != 10);

	BufferFlush();

	while (((*addr) & mask) != ref)
	{
		Core::Thread::SleepMicro(10);
	}
}

void CommandProcessor::WriteData(uint32_t* dst, const uint32_t* src, uint32_t dw_num, uint32_t write_control, bool custom)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(!custom && write_control != 0x04100500);
	EXIT_NOT_IMPLEMENTED(custom && write_control != 0x01000004);

	GpuMemoryCheckAccessViolation(reinterpret_cast<uint64_t>(dst), static_cast<size_t>(dw_num) * 4);

	memcpy(dst, src, static_cast<size_t>(dw_num) * 4);

	GraphicsRenderMemoryFlush(reinterpret_cast<uint64_t>(dst), static_cast<size_t>(dw_num) * 4);
}

void GraphicsRing::Submit(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw, int handle,
                          int index, int flip_mode, int64_t flip_arg)
{
	EXIT_IF(m_cp == nullptr);

	Core::LockGuard lock(m_mutex);

	WindowWaitForGraphicInitialized();
	GraphicsRenderCreateContext();

	if (m_done)
	{
		while (!m_idle)
		{
			m_idle_cond_var.Wait(&m_mutex);
		}
		m_done = false;

		m_cp->Reset();
	}

	CmdBatch buf {};
	buf.draw_buffer.data    = cmd_draw_buffer;
	buf.draw_buffer.num_dw  = num_draw_dw;
	buf.const_buffer.data   = cmd_const_buffer;
	buf.const_buffer.num_dw = num_const_dw;
	buf.flip.handle         = handle;
	buf.flip.index          = index;
	buf.flip.flip_mode      = flip_mode;
	buf.flip.flip_arg       = flip_arg;

	m_cmd_batches.Add(buf);

	m_cond_var.Signal();
}

void GraphicsRing::Done()
{
	Core::LockGuard lock(m_mutex);
	if (m_done)
	{
		while (!m_idle)
		{
			m_idle_cond_var.Wait(&m_mutex);
		}
	}
	m_done = true;
}

void GraphicsRing::WaitForIdle()
{
	Core::LockGuard lock(m_mutex);
	while (!m_idle)
	{
		m_idle_cond_var.Wait(&m_mutex);
	}
}

bool GraphicsRing::IsIdle()
{
	Core::LockGuard lock(m_mutex);
	return m_idle;
}

GraphicsRing::CmdBatch GraphicsRing::GetCmdBatch()
{
	Core::LockGuard lock(m_mutex);

	while (m_cmd_batches.Size() == 0)
	{
		m_idle = true;
		m_idle_cond_var.Signal();

		m_cond_var.Wait(&m_mutex);
	}

	m_idle = false;

	auto first = m_cmd_batches.First();

	CmdBatch buf = m_cmd_batches.At(first);

	m_cmd_batches.Remove(first);

	return buf;
}

void GraphicsRing::ThreadBatchRun(void* data)
{
	EXIT_IF(data == nullptr);

	static std::atomic_uint64_t seq = 0;

	auto* ring = static_cast<GraphicsRing*>(data);
	auto* cp   = ring->m_cp;

	EXIT_IF(ring == nullptr);
	EXIT_IF(cp == nullptr);

	for (;;)
	{
		CmdBatch buf = ring->GetCmdBatch();

		cp->RunLock();
		{
			cp->BufferInit();
			cp->ResetDeCe();
			cp->SetFlip(buf.flip);
			cp->SetSumbitId(++seq);

			ring->m_job1.Execute([cp, buf](void* /*unused*/) { cp->Run(buf.draw_buffer.data, buf.draw_buffer.num_dw); });
			ring->m_job2.Execute([cp, buf](void* /*unused*/) { cp->Run(buf.const_buffer.data, buf.const_buffer.num_dw); });
			ring->m_job1.Wait();
			ring->m_job2.Wait();

			cp->BufferFlush();
		}
		cp->RunUnlock();
	}
}

void ComputeRing::ThreadRun(void* data)
{
	EXIT_IF(data == nullptr);

	auto* ring = static_cast<ComputeRing*>(data);
	auto* cp   = ring->m_cp;

	EXIT_IF(ring == nullptr);
	EXIT_IF(cp == nullptr);

	uint32_t* buffer = nullptr;

	KYTY_PROFILER_THREAD("Thread_Compute");

	ring->m_mutex.Lock();

	for (;;)
	{
		while (!(ring->m_active && ring->m_run_offset_dw > 0))
		{
			ring->m_idle = true;
			ring->m_idle_cond_var.Signal();
			ring->m_cond_var.Wait(&ring->m_mutex);
		}

		ring->m_idle = false;

		auto  pos       = *ring->m_read_ptr_addr;
		auto  num_dw    = ring->m_run_offset_dw;
		auto  next_pos  = pos + num_dw;
		auto  ring_size = ring->m_ring_size_dw;
		auto* ring_addr = ring->m_ring_addr;

		if (next_pos <= ring_size)
		{
			buffer = ring_addr + pos;
		} else
		{
			auto d1 = next_pos - ring_size;
			auto d2 = num_dw - d1;
			buffer  = ring->m_internal_buffer;
			memcpy(buffer, ring_addr + pos, d2);
			memcpy(buffer + d2, ring_addr, d1);
		}

		ring->m_mutex.Unlock();

		cp->RunLock();
		{
			cp->BufferInit();
			cp->ResetDeCe();

			GraphicsDbgDumpDcb("cc", num_dw, buffer);

			cp->Run(buffer, num_dw);

			cp->BufferFlush();
		}
		cp->RunUnlock();

		ring->m_mutex.Lock();

		ring->m_run_offset_dw -= num_dw;
		*ring->m_read_ptr_addr = next_pos % ring_size;
	}
}

void ComputeRing::MapComputeQueue(uint32_t* ring_addr, uint32_t ring_size_dw, uint32_t* read_ptr_addr)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_active);

	if (m_ring_size_dw != ring_size_dw)
	{
		delete[] m_internal_buffer;
		m_internal_buffer = new uint32_t[ring_size_dw];
	}

	m_ring_addr               = ring_addr;
	m_ring_size_dw            = ring_size_dw;
	m_read_ptr_addr           = read_ptr_addr;
	m_run_offset_dw           = 0;
	m_last_dingdong_offset_dw = 0;

	*m_read_ptr_addr = 0;
	m_active         = true;

	m_cond_var.Signal();
}

void ComputeRing::DingDong(uint32_t offset_dw)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(!m_active);

	WindowWaitForGraphicInitialized();
	GraphicsRenderCreateContext();

	if (m_done)
	{
		while (!m_idle)
		{
			m_idle_cond_var.Wait(&m_mutex);
		}
		m_done = false;
		/*m_cp->Reset();*/
	}

	uint32_t advance = (offset_dw + (offset_dw < m_last_dingdong_offset_dw ? m_ring_size_dw : 0)) - m_last_dingdong_offset_dw;

	m_run_offset_dw += advance;

	m_last_dingdong_offset_dw = offset_dw;

	m_cond_var.Signal();
}

void ComputeRing::Done()
{
	Core::LockGuard lock(m_mutex);
	if (m_done)
	{
		while (!m_idle)
		{
			m_idle_cond_var.Wait(&m_mutex);
		}
	}
	m_done = true;
}

void ComputeRing::WaitForIdle()
{
	Core::LockGuard lock(m_mutex);
	while (!m_idle)
	{
		m_idle_cond_var.Wait(&m_mutex);
	}
}

bool ComputeRing::IsIdle()
{
	Core::LockGuard lock(m_mutex);
	return m_idle;
}

void ComputeRing::SetActive(bool flag)
{
	Core::LockGuard lock(m_mutex);

	m_active = flag;

	m_cond_var.Signal();
}

bool ComputeRing::IsActive()
{
	Core::LockGuard lock(m_mutex);

	return m_active;
}

void CommandProcessor::Run(uint32_t* data, uint32_t num_dw)
{
	KYTY_PROFILER_BLOCK("CommandProcessor::Run");

	if (num_dw > 0)
	{
		GraphicsRenderMemoryFree(reinterpret_cast<uint64_t>(data), static_cast<size_t>(num_dw) * 4);
	}

	auto* cmd = data;
	auto  dw  = num_dw;
	for (;;)
	{
		if (dw == 0)
		{
			break;
		}

		EXIT_NOT_IMPLEMENTED(dw < 2);
		EXIT_NOT_IMPLEMENTED(dw > num_dw);

		auto cmd_id = *cmd++;

		auto op = (cmd_id >> 8u) & 0xffu;

		auto pfunc = g_cp_op_func[op];

		if (pfunc == nullptr)
		{
			EXIT("unknown op\n\t%05" PRIx32 ":\n\tcmd_id = %08" PRIx32 "\n", num_dw - dw, cmd_id);
		}

		auto s = pfunc(this, cmd_id, cmd, dw, num_dw);

		// printf("\t %05" PRIx32 ": %u\n", num_dw - dw, s);

		cmd += s;
		dw -= s + 1;
	}
}

void CommandProcessor::SetIndexType(uint32_t index_type_and_size)
{
	Core::LockGuard lock(m_mutex);

	m_index_type_and_size = index_type_and_size;
}

void CommandProcessor::SetNumInstances(uint32_t num_instances)
{
	Core::LockGuard lock(m_mutex);

	if (num_instances == 0)
	{
		num_instances = 1;
	}

	m_num_instances = num_instances;

	EXIT_NOT_IMPLEMENTED(m_num_instances != 1);
}

void CommandProcessor::DrawIndex(uint32_t index_count, const void* index_addr, uint32_t flags, uint32_t type)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	GraphicsRenderDrawIndex(m_sumbit_id, m_buffer[m_current_buffer], &m_ctx, &m_ucfg, &m_sh_ctx, m_index_type_and_size, index_count,
	                        index_addr, flags, type);
}

void CommandProcessor::DispatchDirect(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z, uint32_t mode)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	GraphicsRenderDispatchDirect(m_sumbit_id, m_buffer[m_current_buffer], &m_ctx, &m_sh_ctx, thread_group_x, thread_group_y, thread_group_z,
	                             mode);
}

void CommandProcessor::DrawIndexAuto(uint32_t index_count, uint32_t flags)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	GraphicsRenderDrawIndexAuto(m_sumbit_id, m_buffer[m_current_buffer], &m_ctx, &m_ucfg, &m_sh_ctx, index_count, flags);
}

void CommandProcessor::ClearGds(uint64_t dw_offset, uint32_t dw_num, uint32_t clear_value)
{
	Core::LockGuard lock(m_mutex);

	GraphicsRenderClearGds(dw_offset, dw_num, clear_value);
}

void CommandProcessor::ReadGds(uint32_t* dst, uint32_t dw_offset, uint32_t dw_size)
{
	Core::LockGuard lock(m_mutex);

	GraphicsRenderReadGds(dst, dw_offset, dw_size);
}

void CommandProcessor::WaitFlipDone(uint32_t video_out_handle, uint32_t display_buffer_index)
{
	BufferFlush();

	VideoOut::VideoOutWaitFlipDone(static_cast<int>(video_out_handle), static_cast<int>(display_buffer_index));
}

void CommandProcessor::WriteAtEndOfPipe32(uint32_t cache_policy, uint32_t event_write_dest, uint32_t eop_event_type, uint32_t cache_action,
                                          uint32_t event_index, uint32_t event_write_source, void* dst_gpu_addr, uint32_t value,
                                          uint32_t interrupt_selector)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	printf("CommandProcessor::WriteAtEndOfPipe32()\n");
	printf("\t cache_policy        = 0x%08" PRIx32 "\n", cache_policy);
	printf("\t event_write_dest    = 0x%08" PRIx32 "\n", event_write_dest);
	printf("\t eop_event_type      = 0x%08" PRIx32 "\n", eop_event_type);
	printf("\t cache_action        = 0x%08" PRIx32 "\n", cache_action);
	printf("\t event_index         = 0x%08" PRIx32 "\n", event_index);
	printf("\t event_write_source  = 0x%08" PRIx32 "\n", event_write_source);
	printf("\t interrupt_selector  = 0x%08" PRIx32 "\n", interrupt_selector);
	printf("\t dst_gpu_addr        = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(dst_gpu_addr));
	printf("\t value               = 0x%08" PRIx32 "\n", value);

	EXIT_NOT_IMPLEMENTED(cache_policy != 0x00000000);
	EXIT_NOT_IMPLEMENTED(event_write_dest != 0x00000000);
	EXIT_NOT_IMPLEMENTED(interrupt_selector != 0x0);

	if (event_write_source == 0x00000002 && eop_event_type == 0x0000002f && cache_action == 0x00000000 && event_index == 0x00000006)
	{
		GraphicsRenderWriteAtEndOfPipe32(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint32_t*>(dst_gpu_addr), value);
	} else if (event_write_source == 0x00000001 && eop_event_type == 0x0000002f && cache_action == 0x00000000 && event_index == 0x00000006)
	{
		GraphicsRenderWriteAtEndOfPipeGds32(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint32_t*>(dst_gpu_addr), value & 0xffffu,
		                                    value >> 16u);
	} else
	{
		EXIT("unknown event type\n");
	}
}

void CommandProcessor::WriteAtEndOfPipe64(uint32_t cache_policy, uint32_t event_write_dest, uint32_t eop_event_type, uint32_t cache_action,
                                          uint32_t event_index, uint32_t event_write_source, void* dst_gpu_addr, uint64_t value,
                                          uint32_t interrupt_selector)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	printf("CommandProcessor::WriteAtEndOfPipe64()\n");
	printf("\t cache_policy        = 0x%08" PRIx32 "\n", cache_policy);
	printf("\t event_write_dest    = 0x%08" PRIx32 "\n", event_write_dest);
	printf("\t eop_event_type      = 0x%08" PRIx32 "\n", eop_event_type);
	printf("\t cache_action        = 0x%08" PRIx32 "\n", cache_action);
	printf("\t event_index         = 0x%08" PRIx32 "\n", event_index);
	printf("\t event_write_source  = 0x%08" PRIx32 "\n", event_write_source);
	printf("\t interrupt_selector  = 0x%08" PRIx32 "\n", interrupt_selector);
	printf("\t dst_gpu_addr        = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(dst_gpu_addr));
	printf("\t value               = 0x%016" PRIx64 "\n", value);

	EXIT_NOT_IMPLEMENTED(cache_policy != 0x00000000);
	EXIT_NOT_IMPLEMENTED(event_write_dest != 0x00000000);

	bool with_interrupt = false;
	bool source64       = (event_write_source == 0x02);
	bool source32       = (event_write_source == 0x01);
	bool source_counter = (event_write_source == 0x04);

	switch (interrupt_selector)
	{
		case 0x00:
		case 0x03: with_interrupt = false; break;
		case 0x02: with_interrupt = true; break;
		default: EXIT("unknown interrupt_selector\n");
	}

	if (eop_event_type == 0x04 && cache_action == 0x00 && event_index == 0x05 && source64 && !with_interrupt)
	{
		GraphicsRenderWriteAtEndOfPipe64(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint64_t*>(dst_gpu_addr), value);
	} else if (eop_event_type == 0x04 && cache_action == 0x00 && event_index == 0x05 && source32 && !with_interrupt)
	{
		GraphicsRenderWriteAtEndOfPipe32(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint32_t*>(dst_gpu_addr), value);
	} else if (((eop_event_type == 0x04 && event_index == 0x05) || (eop_event_type == 0x28 && event_index == 0x05) ||
	            (eop_event_type == 0x2f && event_index == 0x06) || (eop_event_type == 0x14 && event_index == 0x00) ||
	            (eop_event_type == 0x28 && event_index == 0x00)) &&
	           cache_action == 0x38 && source64 && !with_interrupt)
	{
		GraphicsRenderWriteAtEndOfPipeWithWriteBack64(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint64_t*>(dst_gpu_addr), value);
	} else if (((eop_event_type == 0x04 && event_index == 0x05) || (eop_event_type == 0x28 && event_index == 0x00)) &&
	           cache_action == 0x00 && source_counter && !with_interrupt)
	{
		GraphicsRenderWriteAtEndOfPipeClockCounter(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint64_t*>(dst_gpu_addr));
	} else if ((eop_event_type == 0x04 && event_index == 0x05) && cache_action == 0x00 && source64 && with_interrupt)
	{
		GraphicsRenderWriteAtEndOfPipeWithInterrupt64(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint64_t*>(dst_gpu_addr), value);
	} else if ((eop_event_type == 0x04 && event_index == 0x05) && cache_action == 0x00 && source32 && with_interrupt)
	{
		GraphicsRenderWriteAtEndOfPipeWithInterrupt32(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint32_t*>(dst_gpu_addr), value);
	} else if ((eop_event_type == 0x04 && event_index == 0x05) && cache_action == 0x3b && source64 && with_interrupt)
	{
		GraphicsRenderWriteAtEndOfPipeWithInterruptWriteBack64(m_sumbit_id, m_buffer[m_current_buffer],
		                                                       static_cast<uint64_t*>(dst_gpu_addr), value);
	} else
	{
		EXIT("unknown event type\n");
	}
}

void CommandProcessor::MemoryBarrier()
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	GraphicsRenderMemoryBarrier(m_buffer[m_current_buffer]);
}

void CommandProcessor::RenderTextureBarrier(uint64_t vaddr, uint64_t size)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	GraphicsRenderRenderTextureBarrier(m_buffer[m_current_buffer], vaddr, size);
}

void CommandProcessor::DepthStencilBarrier(uint64_t vaddr, uint64_t size)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	GraphicsRenderDepthStencilBarrier(m_buffer[m_current_buffer], vaddr, size);
}

void CommandProcessor::TriggerEvent(uint32_t event_type, uint32_t event_index)
{
	Core::LockGuard lock(m_mutex);

	printf("CommandProcessor::TriggerEvent()\n");
	printf("\t event_type  = 0x%08" PRIx32 "\n", event_type);
	printf("\t event_index = 0x%08" PRIx32 "\n", event_index);

	if ((event_type == 0x00000016 || event_type == 0x00000031) && event_index == 0x00000007)
	{
		// CacheFlushAndInvEvent
		// FlushAndInvalidateCbPixelData
		MemoryBarrier();
	} else if ((event_type == 0x0000002c) && event_index == 0x00000007)
	{ // NOLINT
	  // FlushAndInvalidateDbMeta
	} else if ((event_type == 0x00000010) && event_index == 0x00000000)
	{
		// PsPartialFlush
	} else
	{
		EXIT("unknown event type: 0x%08" PRIx32 ", 0x%08" PRIx32 "\n", event_type, event_index);
	}
}

void CommandProcessor::Flip()
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	printf("CommandProcessor::Flip()\n");

	GraphicsRenderWriteAtEndOfPipeOnlyFlip(m_sumbit_id, m_buffer[m_current_buffer], m_flip.handle, m_flip.index, m_flip.flip_mode,
	                                       m_flip.flip_arg);
}

void CommandProcessor::Flip(void* dst_gpu_addr, uint32_t value)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	printf("CommandProcessor::Flip()\n");
	printf("\t dst_gpu_addr = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(dst_gpu_addr));
	printf("\t value        = 0x%08" PRIx32 "\n", value);

	GraphicsRenderWriteAtEndOfPipeWithFlip32(m_sumbit_id, m_buffer[m_current_buffer], static_cast<uint32_t*>(dst_gpu_addr), value,
	                                         m_flip.handle, m_flip.index, m_flip.flip_mode, m_flip.flip_arg);
}

void CommandProcessor::FlipWithInterrupt(uint32_t eop_event_type, uint32_t cache_action, void* dst_gpu_addr, uint32_t value)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_current_buffer < 0 || m_current_buffer >= VK_BUFFERS_NUM);

	printf("CommandProcessor::FlipWithInterrupt()\n");
	printf("\t eop_event_type      = 0x%08" PRIx32 "\n", eop_event_type);
	printf("\t cache_action        = 0x%08" PRIx32 "\n", cache_action);
	printf("\t dst_gpu_addr        = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(dst_gpu_addr));
	printf("\t value               = 0x%08" PRIx32 "\n", value);

	if (eop_event_type == 0x00000004 && cache_action == 0x00000038)
	{
		GraphicsRenderWriteAtEndOfPipeWithInterruptWriteBackFlip32(m_sumbit_id, m_buffer[m_current_buffer],
		                                                           static_cast<uint32_t*>(dst_gpu_addr), value, m_flip.handle, m_flip.index,
		                                                           m_flip.flip_mode, m_flip.flip_arg);
	} else
	{
		EXIT("unknown event type\n");
	}
}

void CommandProcessor::WriteBack()
{
	Core::LockGuard lock(m_mutex);

	GraphicsRenderWriteBack(this);
}

// void CommandBuffer::CommandProcessorWait()
//{
//	EXIT_IF(m_parent == nullptr);
//
//	m_parent->BufferWait();
//}

void GraphicsRunSubmit(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw)
{
	EXIT_IF(cmd_draw_buffer == nullptr);
	EXIT_IF(num_draw_dw == 0);
	EXIT_IF(g_gpu == nullptr);

	g_gpu->Submit(cmd_draw_buffer, num_draw_dw, cmd_const_buffer, num_const_dw);
}

void GraphicsRunSubmitAndFlip(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw,
                              int handle, int index, int flip_mode, int64_t flip_arg)
{
	EXIT_IF(cmd_draw_buffer == nullptr);
	EXIT_IF(num_draw_dw == 0);
	EXIT_IF(g_gpu == nullptr);

	g_gpu->SubmitAndFlip(cmd_draw_buffer, num_draw_dw, cmd_const_buffer, num_const_dw, handle, index, flip_mode, flip_arg);
}

void GraphicsRunDingDong(uint32_t ring_id, uint32_t offset_dw)
{
	EXIT_IF(g_gpu == nullptr);

	g_gpu->DingDong(ring_id, offset_dw);
}

uint32_t GraphicsRunMapComputeQueue(uint32_t pipe_id, uint32_t queue_id, uint32_t* ring_addr, uint32_t ring_size_dw,
                                    uint32_t* read_ptr_addr)
{
	EXIT_IF(g_gpu == nullptr);

	return g_gpu->MapComputeQueue(pipe_id, queue_id, ring_addr, ring_size_dw, read_ptr_addr);
}

void GraphicsRunUnmapComputeQueue(uint32_t id)
{
	EXIT_IF(g_gpu == nullptr);

	g_gpu->Unmap(id);
}

void GraphicsRunWait()
{
	EXIT_IF(g_gpu == nullptr);

	g_gpu->Wait();
}

void GraphicsRunDone()
{
	EXIT_IF(g_gpu == nullptr);

	g_gpu->Done();
}

bool GraphicsRunAreSubmitsAllowed()
{
	EXIT_IF(g_gpu == nullptr);

	return g_gpu->AreSubmitsAllowed();
}

int GraphicsRunGetFrameNum()
{
	EXIT_IF(g_gpu == nullptr);

	return g_gpu->GetFrameNum();
}

void GraphicsRunCommandProcessorFlush(CommandProcessor* cp)
{
	EXIT_IF(cp == nullptr);

	cp->BufferFlush();
}

void GraphicsRunCommandProcessorWait(CommandProcessor* cp)
{
	EXIT_IF(cp == nullptr);

	cp->BufferWait();
}

void GraphicsRunCommandProcessorLock(CommandProcessor* cp)
{
	EXIT_IF(cp == nullptr);

	cp->Lock();
}

void GraphicsRunCommandProcessorUnlock(CommandProcessor* cp)
{
	EXIT_IF(cp == nullptr);

	cp->Unlock();
}

KYTY_HW_CTX_PARSER(hw_ctx_set_aa_config)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_SC_AA_CONFIG);

	HW::AaConfig r;

	r.msaa_num_samples      = KYTY_PM4_GET(buffer[0], PA_SC_AA_CONFIG, MSAA_NUM_SAMPLES);
	r.aa_mask_centroid_dtmn = KYTY_PM4_GET(buffer[0], PA_SC_AA_CONFIG, AA_MASK_CENTROID_DTMN) != 0;
	r.max_sample_dist       = KYTY_PM4_GET(buffer[0], PA_SC_AA_CONFIG, MAX_SAMPLE_DIST);
	r.msaa_exposed_samples  = KYTY_PM4_GET(buffer[0], PA_SC_AA_CONFIG, MSAA_EXPOSED_SAMPLES);

	cp->GetCtx()->SetAaConfig(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_aa_sample_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0106900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0);

	uint32_t count = 1;

	if (dw >= 20 && buffer[16] == 0xc0026900 && buffer[17] == Pm4::PA_SC_CENTROID_PRIORITY_0)
	{
		count = 20;

		HW::AaSampleControl r;

		memcpy(r.locations, buffer, static_cast<size_t>(16) * 4);

		r.centroid_priority = static_cast<uint64_t>(buffer[18]) | (static_cast<uint64_t>(buffer[19]) << 32u);

		cp->GetCtx()->SetAaSampleControl(r);
	} else
	{
		KYTY_NOT_IMPLEMENTED;
	}

	return count;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_blend_color)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0046900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::CB_BLEND_RED);

	HW::BlendColor r;

	r.red   = *reinterpret_cast<const float*>(&buffer[0]);
	r.green = *reinterpret_cast<const float*>(&buffer[1]);
	r.blue  = *reinterpret_cast<const float*>(&buffer[2]);
	r.alpha = *reinterpret_cast<const float*>(&buffer[3]);

	cp->GetCtx()->SetBlendColor(r);

	return 4;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_blend_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);

	uint32_t param = (cmd_offset - Pm4::CB_BLEND0_CONTROL) / 1;

	HW::BlendControl r;

	r.color_srcblend  = (buffer[0] >> Pm4::CB_BLEND0_CONTROL_COLOR_SRCBLEND_SHIFT) & Pm4::CB_BLEND0_CONTROL_COLOR_SRCBLEND_MASK;
	r.color_comb_fcn  = (buffer[0] >> Pm4::CB_BLEND0_CONTROL_COLOR_COMB_FCN_SHIFT) & Pm4::CB_BLEND0_CONTROL_COLOR_COMB_FCN_MASK;
	r.color_destblend = (buffer[0] >> Pm4::CB_BLEND0_CONTROL_COLOR_DESTBLEND_SHIFT) & Pm4::CB_BLEND0_CONTROL_COLOR_DESTBLEND_MASK;
	r.alpha_srcblend  = (buffer[0] >> Pm4::CB_BLEND0_CONTROL_ALPHA_SRCBLEND_SHIFT) & Pm4::CB_BLEND0_CONTROL_ALPHA_SRCBLEND_MASK;
	r.alpha_comb_fcn  = (buffer[0] >> Pm4::CB_BLEND0_CONTROL_ALPHA_COMB_FCN_SHIFT) & Pm4::CB_BLEND0_CONTROL_ALPHA_COMB_FCN_MASK;
	r.alpha_destblend = (buffer[0] >> Pm4::CB_BLEND0_CONTROL_ALPHA_DESTBLEND_SHIFT) & Pm4::CB_BLEND0_CONTROL_ALPHA_DESTBLEND_MASK;
	r.separate_alpha_blend =
	    ((buffer[0] >> Pm4::CB_BLEND0_CONTROL_SEPARATE_ALPHA_BLEND_SHIFT) & Pm4::CB_BLEND0_CONTROL_SEPARATE_ALPHA_BLEND_MASK) != 0;
	r.enable = ((buffer[0] >> Pm4::CB_BLEND0_CONTROL_ENABLE_SHIFT) & Pm4::CB_BLEND0_CONTROL_ENABLE_MASK) != 0;

	cp->GetCtx()->SetBlendControl(param, r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_clip_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_CL_CLIP_CNTL);

	HW::ClipControl r;

	//	r.user_clip_planes                    = buffer[0] & 0x3fu;
	//	r.user_clip_plane_mode                = (buffer[0] >> 14u) & 0x3u;
	//	r.clip_space                          = (buffer[0] >> 19u) & 0x1u;
	//	r.vertex_kill_mode                    = (buffer[0] >> 21u) & 0x1u;
	//	r.min_z_clip_enable                   = (buffer[0] >> 26u) & 0x1u;
	//	r.max_z_clip_enable                   = (buffer[0] >> 27u) & 0x1u;
	//	r.user_clip_plane_negate_y            = (buffer[0] & 0x00002000u) != 0;
	//	r.clip_enable                         = (buffer[0] & 0x00010000u) != 0;
	//	r.user_clip_plane_cull_only           = (buffer[0] & 0x00020000u) != 0;
	//	r.cull_on_clipping_error_disable      = (buffer[0] & 0x00100000u) != 0;
	//	r.linear_attribute_clip_enable        = (buffer[0] & 0x01000000u) != 0;
	//	r.force_viewport_index_from_vs_enable = (buffer[0] & 0x02000000u) != 0;

	r.user_clip_planes                    = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, UCP_ENA);
	r.user_clip_plane_mode                = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, PS_UCP_MODE);
	r.dx_clip_space                       = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, DX_CLIP_SPACE_DEF) != 0;
	r.vertex_kill_any                     = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, VTX_KILL_OR) != 0;
	r.min_z_clip_disable                  = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, ZCLIP_NEAR_DISABLE) != 0;
	r.max_z_clip_disable                  = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, ZCLIP_FAR_DISABLE) != 0;
	r.user_clip_plane_negate_y            = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, PS_UCP_Y_SCALE_NEG) != 0;
	r.clip_disable                        = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, CLIP_DISABLE) != 0;
	r.user_clip_plane_cull_only           = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, UCP_CULL_ONLY_ENA) != 0;
	r.cull_on_clipping_error_disable      = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, DIS_CLIP_ERR_DETECT) != 0;
	r.linear_attribute_clip_enable        = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, DX_LINEAR_ATTR_CLIP_ENA) != 0;
	r.force_viewport_index_from_vs_enable = KYTY_PM4_GET(buffer[0], PA_CL_CLIP_CNTL, VTE_VPORT_PROVOKE_DISABLE) != 0;

	cp->GetCtx()->SetClipControl(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_color_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::CB_COLOR_CONTROL);

	HW::ColorControl r;

	r.mode = KYTY_PM4_GET(buffer[0], CB_COLOR_CONTROL, MODE);
	r.op   = KYTY_PM4_GET(buffer[0], CB_COLOR_CONTROL, ROP3);

	cp->GetCtx()->SetColorControl(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_color_info)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0016900);

	uint32_t param = (cmd_offset - Pm4::CB_COLOR0_INFO) / 15;

	HW::ColorInfo r;

	//	r.fmask_compression_enable = (buffer[4] & 0x4000u) != 0;
	//	// r.fmask_compression_mode   = (buffer[4] >> 26u) & 0x3u;
	//	r.fmask_data_compression_disable = ((buffer[4] >> 26u) & 0x1u) != 0;
	//	r.fmask_one_frag_mode            = ((buffer[4] >> 27u) & 0x1u) != 0;
	//	r.cmask_fast_clear_enable        = (buffer[4] & 0x2000u) != 0;
	//	r.dcc_compression_enable         = (buffer[4] & 0x10000000u) != 0;
	//	r.neo_mode                       = (buffer[4] & 0x80000000u) != 0;
	//	r.cmask_tile_mode                = (buffer[4] >> 19u) & 0x1u;
	//	r.cmask_tile_mode_neo            = (buffer[4] >> 29u) & 0x3u;
	//	r.format                         = (buffer[4] >> 2u) & 0x1fu;
	//	r.channel_type                   = (buffer[4] >> 8u) & 0x7u;
	//	r.channel_order                  = (buffer[4] >> 11u) & 0x3u;
	r.format                         = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, FORMAT);
	r.channel_type                   = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, NUMBER_TYPE);
	r.channel_order                  = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, COMP_SWAP);
	r.cmask_fast_clear_enable        = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, FAST_CLEAR) != 0;
	r.fmask_compression_enable       = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, COMPRESSION) != 0;
	r.blend_clamp                    = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, BLEND_CLAMP) != 0;
	r.blend_bypass                   = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, BLEND_BYPASS) != 0;
	r.round_mode                     = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, ROUND_MODE) != 0;
	r.cmask_tile_mode                = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, CMASK_IS_LINEAR);
	r.fmask_data_compression_disable = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, FMASK_COMPRESSION_DISABLE) != 0;
	r.fmask_one_frag_mode            = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, FMASK_COMPRESS_1FRAG_ONLY) != 0;
	r.dcc_compression_enable         = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, DCC_ENABLE) != 0;
	r.cmask_tile_mode_neo            = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, CMASK_ADDR_TYPE);
	r.neo_mode                       = KYTY_PM4_GET(buffer[0], CB_COLOR0_INFO, ALT_TILE_MODE) != 0;

	if (!r.neo_mode && Config::IsNextGen())
	{
		r.neo_mode = true;
	}

	cp->GetCtx()->SetColorInfo(param, r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_depth_clear)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_DEPTH_CLEAR);

	cp->GetCtx()->SetDepthClearValue(*reinterpret_cast<const float*>(buffer));

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_depth_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_DEPTH_CONTROL);

	HW::DepthControl r;

	r.stencil_enable                     = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, STENCIL_ENABLE) != 0;
	r.z_enable                           = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, Z_ENABLE) != 0;
	r.z_write_enable                     = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, Z_WRITE_ENABLE) != 0;
	r.depth_bounds_enable                = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, DEPTH_BOUNDS_ENABLE) != 0;
	r.zfunc                              = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, ZFUNC);
	r.backface_enable                    = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, BACKFACE_ENABLE) != 0;
	r.stencilfunc                        = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, STENCILFUNC);
	r.stencilfunc_bf                     = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, STENCILFUNC_BF);
	r.color_writes_on_depth_fail_enable  = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, ENABLE_COLOR_WRITES_ON_DEPTH_FAIL) != 0;
	r.color_writes_on_depth_pass_disable = KYTY_PM4_GET(buffer[0], DB_DEPTH_CONTROL, DISABLE_COLOR_WRITES_ON_DEPTH_PASS) != 0;

	cp->GetCtx()->SetDepthControl(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_depth_render_target)
{
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_Z_INFO);

	uint32_t count = 1;

	if (cmd_id == 0xC0016900)
	{
		HW::DepthZInfo r;

		//	r.expclear_enabled = (buffer[0] & 0x08000000u) != 0;
		//	r.format              = (buffer[0] >> Pm4::DB_Z_INFO_FORMAT_SHIFT) & Pm4::DB_Z_INFO_FORMAT_MASK;
		//	r.num_samples         = (buffer[0] >> Pm4::DB_Z_INFO_NUM_SAMPLES_SHIFT) & Pm4::DB_Z_INFO_NUM_SAMPLES_MASK;
		//	r.tile_mode_index     = (buffer[0] >> Pm4::DB_Z_INFO_TILE_MODE_INDEX_SHIFT) & Pm4::DB_Z_INFO_TILE_MODE_INDEX_MASK;
		//	r.tile_surface_enable = ((buffer[0] >> Pm4::DB_Z_INFO_TILE_SURFACE_ENABLE_SHIFT) & Pm4::DB_Z_INFO_TILE_SURFACE_ENABLE_MASK) !=0
		// 	r.zrange_precision    = (buffer[0] >> Pm4::DB_Z_INFO_ZRANGE_PRECISION_SHIFT) & Pm4::DB_Z_INFO_ZRANGE_PRECISION_MASK;

		r.format                    = KYTY_PM4_GET(buffer[0], DB_Z_INFO, FORMAT);
		r.num_samples               = KYTY_PM4_GET(buffer[0], DB_Z_INFO, NUM_SAMPLES);
		r.embedded_sample_locations = KYTY_PM4_GET(buffer[0], DB_Z_INFO, ITERATE_FLUSH) != 0;
		r.partially_resident        = KYTY_PM4_GET(buffer[0], DB_Z_INFO, PARTIALLY_RESIDENT) != 0;
		r.num_mip_levels            = KYTY_PM4_GET(buffer[0], DB_Z_INFO, MAXMIP);
		r.tile_mode_index           = KYTY_PM4_GET(buffer[0], DB_Z_INFO, TILE_MODE_INDEX);
		r.plane_compression         = KYTY_PM4_GET(buffer[0], DB_Z_INFO, DECOMPRESS_ON_N_ZPLANES);
		r.expclear_enabled          = KYTY_PM4_GET(buffer[0], DB_Z_INFO, ALLOW_EXPCLEAR) != 0;
		r.tile_surface_enable       = KYTY_PM4_GET(buffer[0], DB_Z_INFO, TILE_SURFACE_ENABLE) != 0;
		r.zrange_precision          = KYTY_PM4_GET(buffer[0], DB_Z_INFO, ZRANGE_PRECISION);

		cp->GetCtx()->SetDepthZInfo(r);
	} else if (cmd_id == 0xC0086900)
	{
		if (dw >= 22 && buffer[8] == 0xC0016900 && buffer[9] == Pm4::DB_DEPTH_INFO && buffer[11] == 0xC0016900 &&
		    buffer[12] == Pm4::DB_DEPTH_VIEW && buffer[14] == 0xC0016900 && buffer[15] == Pm4::DB_HTILE_DATA_BASE &&
		    buffer[17] == 0xC0016900 && buffer[18] == Pm4::DB_HTILE_SURFACE && buffer[20] == 0xC0001000)
		{
			count = 22;

			HW::DepthRenderTarget z;

			//			z.z_info.expclear_enabled    = (buffer[0] & 0x08000000u) != 0;
			//			z.z_info.format              = (buffer[0] >> Pm4::DB_Z_INFO_FORMAT_SHIFT) & Pm4::DB_Z_INFO_FORMAT_MASK;
			//			z.z_info.num_samples         = (buffer[0] >> Pm4::DB_Z_INFO_NUM_SAMPLES_SHIFT) & Pm4::DB_Z_INFO_NUM_SAMPLES_MASK;
			//			z.z_info.tile_mode_index     = (buffer[0] >> Pm4::DB_Z_INFO_TILE_MODE_INDEX_SHIFT) &
			// Pm4::DB_Z_INFO_TILE_MODE_INDEX_MASK; 			z.z_info.tile_surface_enable = KYTY_PM4_GET(buffer[0], DB_Z_INFO,
			// TILE_SURFACE_ENABLE) != 0; 			z.z_info.zrange_precision    = (buffer[0] >> Pm4::DB_Z_INFO_ZRANGE_PRECISION_SHIFT) &
			// Pm4::DB_Z_INFO_ZRANGE_PRECISION_MASK;

			z.z_info.format                    = KYTY_PM4_GET(buffer[0], DB_Z_INFO, FORMAT);
			z.z_info.num_samples               = KYTY_PM4_GET(buffer[0], DB_Z_INFO, NUM_SAMPLES);
			z.z_info.embedded_sample_locations = KYTY_PM4_GET(buffer[0], DB_Z_INFO, ITERATE_FLUSH) != 0;
			z.z_info.partially_resident        = KYTY_PM4_GET(buffer[0], DB_Z_INFO, PARTIALLY_RESIDENT) != 0;
			z.z_info.num_mip_levels            = KYTY_PM4_GET(buffer[0], DB_Z_INFO, MAXMIP);
			z.z_info.tile_mode_index           = KYTY_PM4_GET(buffer[0], DB_Z_INFO, TILE_MODE_INDEX);
			z.z_info.plane_compression         = KYTY_PM4_GET(buffer[0], DB_Z_INFO, DECOMPRESS_ON_N_ZPLANES);
			z.z_info.expclear_enabled          = KYTY_PM4_GET(buffer[0], DB_Z_INFO, ALLOW_EXPCLEAR) != 0;
			z.z_info.tile_surface_enable       = KYTY_PM4_GET(buffer[0], DB_Z_INFO, TILE_SURFACE_ENABLE) != 0;
			z.z_info.zrange_precision          = KYTY_PM4_GET(buffer[0], DB_Z_INFO, ZRANGE_PRECISION);

			//			z.stencil_info.expclear_enabled     = (buffer[1] & 0x08000000u) != 0;
			//			z.stencil_info.tile_split           = (buffer[1] >> 13u) & 0x7u;
			//			z.stencil_info.format               = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, FORMAT);
			//			z.stencil_info.tile_mode_index      = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, TILE_MODE_INDEX);
			//			z.stencil_info.tile_stencil_disable = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, TILE_STENCIL_DISABLE);
			z.stencil_info.format                     = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, FORMAT);
			z.stencil_info.texture_compatible_stencil = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, ITERATE_FLUSH) != 0;
			z.stencil_info.partially_resident         = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, PARTIALLY_RESIDENT) != 0;
			z.stencil_info.tile_split                 = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, RESERVED_FIELD_1);
			z.stencil_info.tile_mode_index            = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, TILE_MODE_INDEX);
			z.stencil_info.expclear_enabled           = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, ALLOW_EXPCLEAR) != 0;
			z.stencil_info.tile_stencil_disable       = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, TILE_STENCIL_DISABLE) != 0;

			z.z_read_base_addr        = static_cast<uint64_t>(buffer[2]) << 8u;
			z.stencil_read_base_addr  = static_cast<uint64_t>(buffer[3]) << 8u;
			z.z_write_base_addr       = static_cast<uint64_t>(buffer[4]) << 8u;
			z.stencil_write_base_addr = static_cast<uint64_t>(buffer[5]) << 8u;

			// DB_DEPTH_SIZE
			z.pitch_div8_minus1  = (buffer[6] >> Pm4::DB_DEPTH_SIZE_PITCH_TILE_MAX_SHIFT) & Pm4::DB_DEPTH_SIZE_PITCH_TILE_MAX_MASK;
			z.height_div8_minus1 = (buffer[6] >> Pm4::DB_DEPTH_SIZE_HEIGHT_TILE_MAX_SHIFT) & Pm4::DB_DEPTH_SIZE_HEIGHT_TILE_MAX_MASK;

			// DB_DEPTH_SLICE
			z.slice_div64_minus1 = (buffer[7] >> Pm4::DB_DEPTH_SLICE_SLICE_TILE_MAX_SHIFT) & Pm4::DB_DEPTH_SLICE_SLICE_TILE_MAX_MASK;

			z.depth_info.addr5_swizzle_mask = KYTY_PM4_GET(buffer[10], DB_DEPTH_INFO, ADDR5_SWIZZLE_MASK);
			z.depth_info.array_mode         = (buffer[10] >> Pm4::DB_DEPTH_INFO_ARRAY_MODE_SHIFT) & Pm4::DB_DEPTH_INFO_ARRAY_MODE_MASK;
			z.depth_info.pipe_config        = (buffer[10] >> Pm4::DB_DEPTH_INFO_PIPE_CONFIG_SHIFT) & Pm4::DB_DEPTH_INFO_PIPE_CONFIG_MASK;
			z.depth_info.bank_width         = (buffer[10] >> Pm4::DB_DEPTH_INFO_BANK_WIDTH_SHIFT) & Pm4::DB_DEPTH_INFO_BANK_WIDTH_MASK;
			z.depth_info.bank_height        = (buffer[10] >> Pm4::DB_DEPTH_INFO_BANK_HEIGHT_SHIFT) & Pm4::DB_DEPTH_INFO_BANK_HEIGHT_MASK;
			z.depth_info.macro_tile_aspect  = KYTY_PM4_GET(buffer[10], DB_DEPTH_INFO, MACRO_TILE_ASPECT);
			z.depth_info.num_banks          = (buffer[10] >> Pm4::DB_DEPTH_INFO_NUM_BANKS_SHIFT) & Pm4::DB_DEPTH_INFO_NUM_BANKS_MASK;

			// z.depth_view.slice_start = (buffer[13] >> Pm4::DB_DEPTH_VIEW_SLICE_START_SHIFT) & Pm4::DB_DEPTH_VIEW_SLICE_START_MASK;
			// z.depth_view.slice_max   = (buffer[13] >> Pm4::DB_DEPTH_VIEW_SLICE_MAX_SHIFT) & Pm4::DB_DEPTH_VIEW_SLICE_MAX_MASK;

			z.depth_view.slice_start =
			    KYTY_PM4_GET(buffer[13], DB_DEPTH_VIEW, SLICE_START) + (KYTY_PM4_GET(buffer[13], DB_DEPTH_VIEW, SLICE_START_HI) << 11u);
			z.depth_view.slice_max =
			    KYTY_PM4_GET(buffer[13], DB_DEPTH_VIEW, SLICE_MAX) + (KYTY_PM4_GET(buffer[13], DB_DEPTH_VIEW, SLICE_MAX_HI) << 11u);
			z.depth_view.depth_write_disable   = KYTY_PM4_GET(buffer[13], DB_DEPTH_VIEW, Z_READ_ONLY) != 0;
			z.depth_view.stencil_write_disable = KYTY_PM4_GET(buffer[13], DB_DEPTH_VIEW, STENCIL_READ_ONLY) != 0;
			z.depth_view.current_mip_level     = KYTY_PM4_GET(buffer[13], DB_DEPTH_VIEW, MIPID);

			z.htile_data_base_addr = static_cast<uint64_t>(buffer[16]) << 8u;

			z.htile_surface.linear     = (buffer[19] >> Pm4::DB_HTILE_SURFACE_LINEAR_SHIFT) & Pm4::DB_HTILE_SURFACE_LINEAR_MASK;
			z.htile_surface.full_cache = (buffer[19] >> Pm4::DB_HTILE_SURFACE_FULL_CACHE_SHIFT) & Pm4::DB_HTILE_SURFACE_FULL_CACHE_MASK;
			z.htile_surface.htile_uses_preload_win = KYTY_PM4_GET(buffer[19], DB_HTILE_SURFACE, HTILE_USES_PRELOAD_WIN);
			z.htile_surface.preload         = (buffer[19] >> Pm4::DB_HTILE_SURFACE_PRELOAD_SHIFT) & Pm4::DB_HTILE_SURFACE_PRELOAD_MASK;
			z.htile_surface.prefetch_width  = KYTY_PM4_GET(buffer[19], DB_HTILE_SURFACE, PREFETCH_WIDTH);
			z.htile_surface.prefetch_height = KYTY_PM4_GET(buffer[19], DB_HTILE_SURFACE, PREFETCH_HEIGHT);
			z.htile_surface.dst_outside_zero_to_one = KYTY_PM4_GET(buffer[19], DB_HTILE_SURFACE, DST_OUTSIDE_ZERO_TO_ONE);

			z.width  = (buffer[21] >> 0u) & 0xffffu;
			z.height = (buffer[21] >> 16u) & 0xffffu;

			cp->GetCtx()->SetDepthRenderTarget(z);
		} else
		{
			KYTY_NOT_IMPLEMENTED;
		}
	} else
	{
		KYTY_NOT_IMPLEMENTED;
	}

	return count;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_eqaa_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_EQAA);

	HW::EqaaControl r;

	r.max_anchor_samples         = KYTY_PM4_GET(buffer[0], DB_EQAA, MAX_ANCHOR_SAMPLES);
	r.ps_iter_samples            = KYTY_PM4_GET(buffer[0], DB_EQAA, PS_ITER_SAMPLES);
	r.mask_export_num_samples    = KYTY_PM4_GET(buffer[0], DB_EQAA, MASK_EXPORT_NUM_SAMPLES);
	r.alpha_to_mask_num_samples  = KYTY_PM4_GET(buffer[0], DB_EQAA, ALPHA_TO_MASK_NUM_SAMPLES);
	r.high_quality_intersections = KYTY_PM4_GET(buffer[0], DB_EQAA, HIGH_QUALITY_INTERSECTIONS) != 0;
	r.incoherent_eqaa_reads      = KYTY_PM4_GET(buffer[0], DB_EQAA, INCOHERENT_EQAA_READS) != 0;
	r.interpolate_comp_z         = KYTY_PM4_GET(buffer[0], DB_EQAA, INTERPOLATE_COMP_Z) != 0;
	r.static_anchor_associations = KYTY_PM4_GET(buffer[0], DB_EQAA, STATIC_ANCHOR_ASSOCIATIONS) != 0;

	cp->GetCtx()->SetEqaaControl(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_generic_scissor)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0026900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_SC_GENERIC_SCISSOR_TL);

	int left   = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(buffer[0], PA_SC_GENERIC_SCISSOR_TL, TL_X)));
	int top    = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(buffer[0], PA_SC_GENERIC_SCISSOR_TL, TL_Y)));
	int right  = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(buffer[1], PA_SC_GENERIC_SCISSOR_BR, BR_X)));
	int bottom = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(buffer[1], PA_SC_GENERIC_SCISSOR_BR, BR_Y)));

	bool window_offset_disable = KYTY_PM4_GET(buffer[0], PA_SC_GENERIC_SCISSOR_TL, WINDOW_OFFSET_DISABLE) != 0;

	cp->GetCtx()->SetGenericScissor(left, top, right, bottom, !window_offset_disable);

	return 2;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_guard_bands)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0046900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_CL_GB_VERT_CLIP_ADJ);

	auto vert_clip    = *reinterpret_cast<const float*>(&buffer[0]); // PA_CL_GB_VERT_CLIP_ADJ
	auto vert_discard = *reinterpret_cast<const float*>(&buffer[1]); // PA_CL_GB_VERT_DISC_ADJ
	auto horz_clip    = *reinterpret_cast<const float*>(&buffer[2]); // PA_CL_GB_HORZ_CLIP_ADJ
	auto horz_discard = *reinterpret_cast<const float*>(&buffer[3]); // PA_CL_GB_HORZ_DISC_ADJ

	cp->GetCtx()->SetGuardBands(horz_clip, vert_clip, horz_discard, vert_discard);

	return 4;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_hardware_screen_offset)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_SU_HARDWARE_SCREEN_OFFSET);

	// uint32_t x = static_cast<uint16_t>(buffer[0] & 0xffffu);
	// uint32_t y = static_cast<uint16_t>((buffer[0] >> 16u) & 0xffffu);

	uint32_t x = KYTY_PM4_GET(buffer[0], PA_SU_HARDWARE_SCREEN_OFFSET, HW_SCREEN_OFFSET_X);
	uint32_t y = KYTY_PM4_GET(buffer[0], PA_SU_HARDWARE_SCREEN_OFFSET, HW_SCREEN_OFFSET_Y);

	cp->GetCtx()->SetHardwareScreenOffset(x, y);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_line_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_SU_LINE_CNTL);

	auto line_width = KYTY_PM4_GET(buffer[0], PA_SU_LINE_CNTL, WIDTH);

	if (line_width == 8)
	{
		cp->GetCtx()->SetLineWidth(1.0f);
	} else
	{
		cp->GetCtx()->SetLineWidth(static_cast<float>(line_width) / 8.0f);
	}

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_mode_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_SU_SC_MODE_CNTL);

	HW::ModeControl r;

	r.cull_front               = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, CULL_FRONT) != 0;
	r.cull_back                = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, CULL_BACK) != 0;
	r.face                     = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, FACE) != 0;
	r.poly_mode                = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, POLY_MODE);
	r.polymode_front_ptype     = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, POLYMODE_FRONT_PTYPE);
	r.polymode_back_ptype      = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, POLYMODE_BACK_PTYPE);
	r.poly_offset_front_enable = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, POLY_OFFSET_FRONT_ENABLE) != 0;
	r.poly_offset_back_enable  = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, POLY_OFFSET_BACK_ENABLE) != 0;
	r.vtx_window_offset_enable = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, VTX_WINDOW_OFFSET_ENABLE) != 0;
	r.provoking_vtx_last       = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, PROVOKING_VTX_LAST) != 0;
	r.persp_corr_dis           = KYTY_PM4_GET(buffer[0], PA_SU_SC_MODE_CNTL, PERSP_CORR_DIS) != 0;

	cp->GetCtx()->SetModeControl(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_ps_input)
{
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::SPI_PS_INPUT_CNTL_0);

	uint32_t count = (cmd_id >> 16u) & 0x3fffu;

	EXIT_NOT_IMPLEMENTED(count == 0);
	EXIT_NOT_IMPLEMENTED(count > 32);

	for (uint32_t i = 0; i < count; i++)
	{
		cp->GetCtx()->SetPsInputSettings(i, buffer[i]);
	}

	return count;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_render_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_RENDER_CONTROL);

	HW::RenderControl r;

	r.depth_clear_enable       = KYTY_PM4_GET(buffer[0], DB_RENDER_CONTROL, DEPTH_CLEAR_ENABLE) != 0;
	r.stencil_clear_enable     = KYTY_PM4_GET(buffer[0], DB_RENDER_CONTROL, STENCIL_CLEAR_ENABLE) != 0;
	r.resummarize_enable       = KYTY_PM4_GET(buffer[0], DB_RENDER_CONTROL, RESUMMARIZE_ENABLE) != 0;
	r.stencil_compress_disable = KYTY_PM4_GET(buffer[0], DB_RENDER_CONTROL, STENCIL_COMPRESS_DISABLE) != 0;
	r.depth_compress_disable   = KYTY_PM4_GET(buffer[0], DB_RENDER_CONTROL, DEPTH_COMPRESS_DISABLE) != 0;
	r.copy_centroid            = KYTY_PM4_GET(buffer[0], DB_RENDER_CONTROL, COPY_CENTROID) != 0;
	r.copy_sample              = KYTY_PM4_GET(buffer[0], DB_RENDER_CONTROL, COPY_SAMPLE);

	cp->GetCtx()->SetRenderControl(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_render_target)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC00E6900 && cmd_id != 0xC00B6900);

	uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_BASE) / 15;

	uint32_t count = 11;

	auto* ctx = cp->GetCtx();

	HW::ColorBase       base;
	HW::ColorPitch      pitch;
	HW::ColorSlice      slice;
	HW::ColorView       view;
	HW::ColorInfo       info;
	HW::ColorAttrib     attrib;
	HW::ColorDccControl dcc;
	HW::ColorCmask      cmask;
	HW::ColorCmaskSlice cmask_slice;
	HW::ColorFmask      fmask;
	HW::ColorFmaskSlice fmask_slice;

	base.addr                     = static_cast<uint64_t>(buffer[0]) << 8u;
	pitch.pitch_div8_minus1       = buffer[1] & 0x7ffu;
	pitch.fmask_pitch_div8_minus1 = (buffer[1] >> 20u) & 0x7ffu;
	slice.slice_div64_minus1      = buffer[2] & 0x3fffffu;

	// view.base_array_slice_index     = buffer[3] & 0x7ffu;
	// view.last_array_slice_index     = (buffer[3] >> 13u) & 0x7ffu;
	view.base_array_slice_index = KYTY_PM4_GET(buffer[3], CB_COLOR0_VIEW, SLICE_START);
	view.last_array_slice_index = KYTY_PM4_GET(buffer[3], CB_COLOR0_VIEW, SLICE_MAX);
	view.current_mip_level      = KYTY_PM4_GET(buffer[3], CB_COLOR0_VIEW, MIP_LEVEL);

	//	info.fmask_compression_enable = (buffer[4] & 0x4000u) != 0;
	//
	//	// info.fmask_compression_mode   = (buffer[4] >> 26u) & 0x3u;
	//	info.fmask_data_compression_disable = ((buffer[4] >> 26u) & 0x1u) != 0;
	//	info.fmask_one_frag_mode            = ((buffer[4] >> 27u) & 0x1u) != 0;
	//
	//	info.cmask_fast_clear_enable = (buffer[4] & 0x2000u) != 0;
	//	info.dcc_compression_enable  = (buffer[4] & 0x10000000u) != 0;
	//	info.neo_mode                = (buffer[4] & 0x80000000u) != 0;
	//	info.cmask_tile_mode         = (buffer[4] >> 19u) & 0x1u;
	//	info.cmask_tile_mode_neo     = (buffer[4] >> 29u) & 0x3u;
	//	info.format                  = (buffer[4] >> 2u) & 0x1fu;
	//	info.channel_type            = (buffer[4] >> 8u) & 0x7u;
	//	info.channel_order           = (buffer[4] >> 11u) & 0x3u;
	info.format                         = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, FORMAT);
	info.channel_type                   = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, NUMBER_TYPE);
	info.channel_order                  = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, COMP_SWAP);
	info.cmask_fast_clear_enable        = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, FAST_CLEAR) != 0;
	info.fmask_compression_enable       = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, COMPRESSION) != 0;
	info.blend_clamp                    = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, BLEND_CLAMP) != 0;
	info.blend_bypass                   = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, BLEND_BYPASS) != 0;
	info.round_mode                     = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, ROUND_MODE) != 0;
	info.cmask_tile_mode                = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, CMASK_IS_LINEAR);
	info.fmask_data_compression_disable = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, FMASK_COMPRESSION_DISABLE) != 0;
	info.fmask_one_frag_mode            = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, FMASK_COMPRESS_1FRAG_ONLY) != 0;
	info.dcc_compression_enable         = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, DCC_ENABLE) != 0;
	info.cmask_tile_mode_neo            = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, CMASK_ADDR_TYPE);
	info.neo_mode                       = KYTY_PM4_GET(buffer[4], CB_COLOR0_INFO, ALT_TILE_MODE) != 0;

	if (!info.neo_mode && Config::IsNextGen())
	{
		info.neo_mode = true;
	}

	//	attrib.force_dest_alpha_to_one  = (buffer[5] & 0x20000u) != 0;
	//	attrib.tile_mode                = buffer[5] & 0x1fu;
	//	attrib.fmask_tile_mode          = (buffer[5] >> 5u) & 0x1fu;
	//	attrib.num_samples              = (buffer[5] >> 12u) & 0x7u;
	//	attrib.num_fragments            = (buffer[5] >> 15u) & 0x3u;
	attrib.force_dest_alpha_to_one = KYTY_PM4_GET(buffer[5], CB_COLOR0_ATTRIB, FORCE_DST_ALPHA_1) != 0;
	attrib.tile_mode               = KYTY_PM4_GET(buffer[5], CB_COLOR0_ATTRIB, TILE_MODE_INDEX);
	attrib.fmask_tile_mode         = KYTY_PM4_GET(buffer[5], CB_COLOR0_ATTRIB, FMASK_TILE_MODE_INDEX);
	attrib.num_samples             = KYTY_PM4_GET(buffer[5], CB_COLOR0_ATTRIB, NUM_SAMPLES);
	attrib.num_fragments           = KYTY_PM4_GET(buffer[5], CB_COLOR0_ATTRIB, NUM_FRAGMENTS);

	//	dcc.max_uncompressed_block_size = (buffer[6] >> 2u) & 0x3u;
	//	dcc.max_compressed_block_size   = (buffer[6] >> 5u) & 0x3u;
	//	dcc.min_compressed_block_size   = (buffer[6] >> 4u) & 0x1u;
	//	dcc.color_transform             = (buffer[6] >> 7u) & 0x3u;
	//	dcc.overwrite_combiner_disable   = (buffer[6] & 0x1u) != 0;
	//	dcc.independent_64b_blocks    = (buffer[6] & 0x200u) != 0;
	dcc.overwrite_combiner_disable     = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, OVERWRITE_COMBINER_DISABLE) != 0;
	dcc.dcc_clear_key_enable           = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, KEY_CLEAR_ENABLE) != 0;
	dcc.max_uncompressed_block_size    = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, MAX_UNCOMPRESSED_BLOCK_SIZE);
	dcc.min_compressed_block_size      = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, MIN_COMPRESSED_BLOCK_SIZE);
	dcc.max_compressed_block_size      = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, MAX_COMPRESSED_BLOCK_SIZE);
	dcc.color_transform                = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, COLOR_TRANSFORM);
	dcc.independent_64b_blocks         = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, INDEPENDENT_64B_BLOCKS) != 0;
	dcc.data_write_on_dcc_clear_to_reg = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, ENABLE_CONSTANT_ENCODE_REG_WRITE) != 0;
	dcc.independent_128b_blocks        = KYTY_PM4_GET(buffer[6], CB_COLOR0_DCC_CONTROL, INDEPENDENT_128B_BLOCKS) != 0;

	cmask.addr               = static_cast<uint64_t>(buffer[7]) << 8u;
	cmask_slice.slice_minus1 = buffer[8] & 0x3fffu;

	fmask.addr               = static_cast<uint64_t>(buffer[9]) << 8u;
	fmask_slice.slice_minus1 = buffer[10] & 0x3fffffu;

	ctx->SetColorBase(slot, base);
	ctx->SetColorPitch(slot, pitch);
	ctx->SetColorSlice(slot, slice);
	ctx->SetColorView(slot, view);
	ctx->SetColorInfo(slot, info);
	ctx->SetColorAttrib(slot, attrib);
	ctx->SetColorDccControl(slot, dcc);
	ctx->SetColorCmask(slot, cmask);
	ctx->SetColorCmaskSlice(slot, cmask_slice);
	ctx->SetColorFmask(slot, fmask);
	ctx->SetColorFmaskSlice(slot, fmask_slice);

	if (cmd_id == 0xC00E6900)
	{
		count = 14;

		HW::ColorClearWord0 clear_word0;
		HW::ColorClearWord1 clear_word1;
		HW::ColorDccAddr    dcc_addr;

		clear_word0.word0 = buffer[11];
		clear_word1.word1 = buffer[12];
		dcc_addr.addr     = static_cast<uint64_t>(buffer[13]) << 8u;

		ctx->SetColorClearWord0(slot, clear_word0);
		ctx->SetColorClearWord1(slot, clear_word1);
		ctx->SetColorDccAddr(slot, dcc_addr);
	}

	if (dw >= count + 2 && buffer[count] == 0xC0001000)
	{
		HW::ColorSize size;

		size.width  = (buffer[count + 1] >> 0u) & 0xffffu;
		size.height = (buffer[count + 1] >> 16u) & 0xffffu;

		ctx->SetColorSize(slot, size);

		count += 2;
	}

	return count;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_render_target_mask)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::CB_TARGET_MASK);

	cp->GetCtx()->SetRenderTargetMask(*buffer);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_scan_mode_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_SC_MODE_CNTL_0);

	HW::ScanModeControl r;

	r.msaa_enable          = KYTY_PM4_GET(buffer[0], PA_SC_MODE_CNTL_0, MSAA_ENABLE) != 0;
	r.vport_scissor_enable = KYTY_PM4_GET(buffer[0], PA_SC_MODE_CNTL_0, VPORT_SCISSOR_ENABLE) != 0;
	r.line_stipple_enable  = KYTY_PM4_GET(buffer[0], PA_SC_MODE_CNTL_0, LINE_STIPPLE_ENABLE) != 0;

	cp->GetCtx()->SetScanModeControl(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_screen_scissor)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0026900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_SC_SCREEN_SCISSOR_TL);

	int left   = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(buffer[0], PA_SC_SCREEN_SCISSOR_TL, TL_X)));
	int top    = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(buffer[0], PA_SC_SCREEN_SCISSOR_TL, TL_Y)));
	int right  = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(buffer[1], PA_SC_SCREEN_SCISSOR_BR, BR_X)));
	int bottom = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(buffer[1], PA_SC_SCREEN_SCISSOR_BR, BR_Y)));

	cp->GetCtx()->SetScreenScissor(left, top, right, bottom);

	return 2;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_shader_stages)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::VGT_SHADER_STAGES_EN);

	cp->GetCtx()->SetShaderStages(buffer[0]);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_stencil_clear)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_STENCIL_CLEAR);

	cp->GetCtx()->SetStencilClearValue(KYTY_PM4_GET(buffer[0], DB_STENCIL_CLEAR, CLEAR));

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_stencil_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_STENCIL_CONTROL);

	HW::StencilControl r;

	r.stencil_fail     = KYTY_PM4_GET(buffer[0], DB_STENCIL_CONTROL, STENCILFAIL);
	r.stencil_zpass    = KYTY_PM4_GET(buffer[0], DB_STENCIL_CONTROL, STENCILZPASS);
	r.stencil_zfail    = KYTY_PM4_GET(buffer[0], DB_STENCIL_CONTROL, STENCILZFAIL);
	r.stencil_fail_bf  = KYTY_PM4_GET(buffer[0], DB_STENCIL_CONTROL, STENCILFAIL_BF);
	r.stencil_zpass_bf = KYTY_PM4_GET(buffer[0], DB_STENCIL_CONTROL, STENCILZPASS_BF);
	r.stencil_zfail_bf = KYTY_PM4_GET(buffer[0], DB_STENCIL_CONTROL, STENCILZFAIL_BF);

	cp->GetCtx()->SetStencilControl(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_stencil_info)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_STENCIL_INFO);

	HW::DepthStencilInfo r;

	//	r.expclear_enabled = (buffer[0] & 0x08000000u) != 0;
	//	r.tile_split       = (buffer[0] >> 13u) & 0x7u;
	//	r.format          = (buffer[0] >> Pm4::DB_STENCIL_INFO_FORMAT_SHIFT) & Pm4::DB_STENCIL_INFO_FORMAT_MASK;
	//	r.tile_mode_index = (buffer[0] >> Pm4::DB_STENCIL_INFO_TILE_MODE_INDEX_SHIFT) & Pm4::DB_STENCIL_INFO_TILE_MODE_INDEX_MASK;
	//	r.tile_stencil_disable =
	//	    ((buffer[0] >> Pm4::DB_STENCIL_INFO_TILE_STENCIL_DISABLE_SHIFT) & Pm4::DB_STENCIL_INFO_TILE_STENCIL_DISABLE_MASK) != 0;

	r.format                     = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, FORMAT);
	r.texture_compatible_stencil = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, ITERATE_FLUSH) != 0;
	r.partially_resident         = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, PARTIALLY_RESIDENT) != 0;
	r.tile_split                 = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, RESERVED_FIELD_1);
	r.tile_mode_index            = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, TILE_MODE_INDEX);
	r.expclear_enabled           = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, ALLOW_EXPCLEAR) != 0;
	r.tile_stencil_disable       = KYTY_PM4_GET(buffer[1], DB_STENCIL_INFO, TILE_STENCIL_DISABLE) != 0;

	cp->GetCtx()->SetDepthStencilInfo(r);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_stencil_mask)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0026900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::DB_STENCILREFMASK);

	HW::StencilMask r;

	r.stencil_testval      = KYTY_PM4_GET(buffer[0], DB_STENCILREFMASK, STENCILTESTVAL);
	r.stencil_mask         = KYTY_PM4_GET(buffer[0], DB_STENCILREFMASK, STENCILMASK);
	r.stencil_writemask    = KYTY_PM4_GET(buffer[0], DB_STENCILREFMASK, STENCILWRITEMASK);
	r.stencil_opval        = KYTY_PM4_GET(buffer[0], DB_STENCILREFMASK, STENCILOPVAL);
	r.stencil_testval_bf   = KYTY_PM4_GET(buffer[1], DB_STENCILREFMASK_BF, STENCILTESTVAL_BF);
	r.stencil_mask_bf      = KYTY_PM4_GET(buffer[1], DB_STENCILREFMASK_BF, STENCILMASK_BF);
	r.stencil_writemask_bf = KYTY_PM4_GET(buffer[1], DB_STENCILREFMASK_BF, STENCILWRITEMASK_BF);
	r.stencil_opval_bf     = KYTY_PM4_GET(buffer[1], DB_STENCILREFMASK_BF, STENCILOPVAL_BF);

	cp->GetCtx()->SetStencilMask(r);

	return 2;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_viewport_scale_offset)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0066900);

	uint32_t param = (cmd_offset - Pm4::PA_CL_VPORT_XSCALE) / 6;

	EXIT_NOT_IMPLEMENTED(param != 0);

	cp->GetCtx()->SetViewportScaleOffset(param, *reinterpret_cast<const float*>(buffer), *reinterpret_cast<const float*>(buffer + 1),
	                                     *reinterpret_cast<const float*>(buffer + 2), *reinterpret_cast<const float*>(buffer + 3),
	                                     *reinterpret_cast<const float*>(buffer + 4), *reinterpret_cast<const float*>(buffer + 5));

	return 6;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_viewport_transform_control)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0016900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::PA_CL_VTE_CNTL);

	cp->GetCtx()->SetViewportTransformControl(*buffer);

	return 1;
}

KYTY_HW_CTX_PARSER(hw_ctx_set_viewport_z)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0026900);

	uint32_t param = (cmd_offset - Pm4::PA_SC_VPORT_ZMIN_0) / 2;

	EXIT_NOT_IMPLEMENTED(param != 0);

	cp->GetCtx()->SetViewportZ(param, *reinterpret_cast<const float*>(buffer), *reinterpret_cast<const float*>(buffer + 1));

	return 2;
}

KYTY_HW_SH_PARSER(hw_sh_set_cs_shader)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC017101C);

	auto shader_modifier = buffer[0];

	HW::CsStageRegisters r {};

	r.data_addr      = (static_cast<uint64_t>(buffer[1]) << 8u) | (static_cast<uint64_t>(buffer[2]) << 40u);
	r.vgprs          = (buffer[3] >> Pm4::COMPUTE_PGM_RSRC1_VGPRS_SHIFT) & Pm4::COMPUTE_PGM_RSRC1_VGPRS_MASK;
	r.sgprs          = (buffer[3] >> Pm4::COMPUTE_PGM_RSRC1_SGPRS_SHIFT) & Pm4::COMPUTE_PGM_RSRC1_SGPRS_MASK;
	r.bulky          = (buffer[3] >> Pm4::COMPUTE_PGM_RSRC1_BULKY_SHIFT) & Pm4::COMPUTE_PGM_RSRC1_BULKY_MASK;
	r.scratch_en     = (buffer[4] >> Pm4::COMPUTE_PGM_RSRC2_SCRATCH_EN_SHIFT) & Pm4::COMPUTE_PGM_RSRC2_SCRATCH_EN_MASK;
	r.user_sgpr      = (buffer[4] >> Pm4::COMPUTE_PGM_RSRC2_USER_SGPR_SHIFT) & Pm4::COMPUTE_PGM_RSRC2_USER_SGPR_MASK;
	r.tgid_x_en      = (buffer[4] >> Pm4::COMPUTE_PGM_RSRC2_TGID_X_EN_SHIFT) & Pm4::COMPUTE_PGM_RSRC2_TGID_X_EN_MASK;
	r.tgid_y_en      = (buffer[4] >> Pm4::COMPUTE_PGM_RSRC2_TGID_Y_EN_SHIFT) & Pm4::COMPUTE_PGM_RSRC2_TGID_Y_EN_MASK;
	r.tgid_z_en      = (buffer[4] >> Pm4::COMPUTE_PGM_RSRC2_TGID_Z_EN_SHIFT) & Pm4::COMPUTE_PGM_RSRC2_TGID_Z_EN_MASK;
	r.tg_size_en     = (buffer[4] >> Pm4::COMPUTE_PGM_RSRC2_TG_SIZE_EN_SHIFT) & Pm4::COMPUTE_PGM_RSRC2_TG_SIZE_EN_MASK;
	r.tidig_comp_cnt = (buffer[4] >> Pm4::COMPUTE_PGM_RSRC2_TIDIG_COMP_CNT_SHIFT) & Pm4::COMPUTE_PGM_RSRC2_TIDIG_COMP_CNT_MASK;
	r.lds_size       = (buffer[4] >> Pm4::COMPUTE_PGM_RSRC2_LDS_SIZE_SHIFT) & Pm4::COMPUTE_PGM_RSRC2_LDS_SIZE_MASK;
	r.num_thread_x   = buffer[5];
	r.num_thread_y   = buffer[6];
	r.num_thread_z   = buffer[7];

	cp->GetShCtx()->SetCsShader(r, shader_modifier);

	return 24;
}

KYTY_HW_SH_PARSER(hw_sh_set_cs_user_sgpr)
{
	EXIT_NOT_IMPLEMENTED(!(cmd_offset >= Pm4::COMPUTE_USER_DATA_0 && cmd_offset <= Pm4::COMPUTE_USER_DATA_15));

	uint32_t slot = (cmd_offset - Pm4::COMPUTE_USER_DATA_0) / 1;

	auto reg_num = (cmd_id >> 16u) & 0x3fffu;

	EXIT_NOT_IMPLEMENTED(reg_num >= HW::UserSgprInfo::SGPRS_MAX);

	for (uint32_t i = 0; i < reg_num; i++)
	{
		cp->GetShCtx()->SetCsUserSgpr(slot + i, buffer[i], cp->GetUserDataMarker());
	}
	cp->SetUserDataMarker(HW::UserSgprType::Unknown);

	return reg_num;
}

KYTY_HW_SH_PARSER(hw_sh_set_ps_embedded)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0261038);

	auto id = buffer[0];

	cp->GetShCtx()->SetPsEmbedded(id);

	return 39;
}

KYTY_HW_SH_PARSER(hw_sh_set_ps_shader)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0261008);

	// HW::PsStageRegisters r {};
	HW::PsShaderResource1 r1;
	HW::PsShaderResource2 r2;

	//	r.data_addr   = (static_cast<uint64_t>(buffer[0]) << 8u) | (static_cast<uint64_t>(buffer[1]) << 40u);
	//	r.vgprs       = (buffer[2] >> Pm4::SPI_SHADER_PGM_RSRC1_PS_VGPRS_SHIFT) & Pm4::SPI_SHADER_PGM_RSRC1_PS_VGPRS_MASK;
	//	r.sgprs       = (buffer[2] >> Pm4::SPI_SHADER_PGM_RSRC1_PS_SGPRS_SHIFT) & Pm4::SPI_SHADER_PGM_RSRC1_PS_SGPRS_MASK;
	//	r.scratch_en  = (buffer[3] >> Pm4::SPI_SHADER_PGM_RSRC2_PS_SCRATCH_EN_SHIFT) & Pm4::SPI_SHADER_PGM_RSRC2_PS_SCRATCH_EN_MASK;
	//	r.user_sgpr   = (buffer[3] >> Pm4::SPI_SHADER_PGM_RSRC2_PS_USER_SGPR_SHIFT) & Pm4::SPI_SHADER_PGM_RSRC2_PS_USER_SGPR_MASK;
	//	r.wave_cnt_en = (buffer[3] >> Pm4::SPI_SHADER_PGM_RSRC2_PS_WAVE_CNT_EN_SHIFT) & Pm4::SPI_SHADER_PGM_RSRC2_PS_WAVE_CNT_EN_MASK;

	uint64_t addr = (static_cast<uint64_t>(buffer[0]) << 8u) | (static_cast<uint64_t>(buffer[1] & 0xffu) << 40u);

	r1.vgprs                    = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, VGPRS);
	r1.sgprs                    = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, SGPRS);
	r1.priority                 = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, PRIORITY);
	r1.float_mode               = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, FLOAT_MODE);
	r1.dx10_clamp               = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, DX10_CLAMP) != 0;
	r1.debug_mode               = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, DEBUG_MODE) != 0;
	r1.ieee_mode                = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, IEEE_MODE) != 0;
	r1.cu_group_disable         = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, CU_GROUP_DISABLE) != 0;
	r1.require_forward_progress = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, FWD_PROGRESS) != 0;
	r1.fp16_overflow            = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, FP16_OVFL) != 0;

	r2.scratch_en = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, SCRATCH_EN);
	r2.user_sgpr  = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, USER_SGPR) +
	               (KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, USER_SGPR_MSB) << 5u);
	r2.wave_cnt_en            = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, WAVE_CNT_EN);
	r2.extra_lds_size         = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, EXTRA_LDS_SIZE);
	r2.raster_ordered_shading = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, LOAD_INTRAWAVE_COLLISION);
	r2.shared_vgprs           = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, SHARED_VGPR_CNT);

	cp->GetCtx()->SetShaderZFormat(buffer[4]);

	for (uint32_t i = 0; i < 8; i++)
	{
		cp->GetCtx()->SetTargetOutputMode(i, (buffer[5] >> (i * 4)) & 0xFu);
	}

	cp->GetCtx()->SetPsInputEna(buffer[6]);
	cp->GetCtx()->SetPsInputAddr(buffer[7]);
	cp->GetCtx()->SetPsInControl(buffer[8]);
	cp->GetCtx()->SetBarycCntl(buffer[9]);

	HW::DepthShaderControl db_shader_control {};

	//	db_shader_control.conservative_z_export_value = (buffer[10] >> 13u) & 0x3u;
	//	db_shader_control.shader_z_behavior           = (buffer[10] >> 4u) & 0x3u;
	//	db_shader_control.shader_kill_enable          = (buffer[10] & 0x40u) != 0;
	//	db_shader_control.shader_z_export_enable      = (buffer[10] & 0x1u) != 0;
	//	db_shader_control.shader_execute_on_noop      = (buffer[10] & 0x400u) != 0;
	db_shader_control.other_bits                  = buffer[10] & 0xFFFF9B8Eu;
	db_shader_control.conservative_z_export_value = KYTY_PM4_GET(buffer[10], DB_SHADER_CONTROL, CONSERVATIVE_Z_EXPORT);
	db_shader_control.shader_z_behavior           = KYTY_PM4_GET(buffer[10], DB_SHADER_CONTROL, Z_ORDER);
	db_shader_control.shader_kill_enable          = KYTY_PM4_GET(buffer[10], DB_SHADER_CONTROL, KILL_ENABLE) != 0;
	db_shader_control.shader_z_export_enable      = KYTY_PM4_GET(buffer[10], DB_SHADER_CONTROL, Z_EXPORT_ENABLE) != 0;
	db_shader_control.shader_execute_on_noop      = KYTY_PM4_GET(buffer[10], DB_SHADER_CONTROL, EXEC_ON_NOOP) != 0;

	cp->GetCtx()->SetDepthShaderControl(db_shader_control);
	cp->GetCtx()->SetShaderMask(buffer[11]);

	// cp->GetShCtx()->SetPsShader(r);
	cp->GetShCtx()->SetPsShaderBase(addr);
	cp->GetShCtx()->SetPsShaderResource1(r1);
	cp->GetShCtx()->SetPsShaderResource2(r2);

	return 39;
}

KYTY_HW_SH_PARSER(hw_sh_set_ps_user_sgpr)
{
	EXIT_NOT_IMPLEMENTED(!(cmd_offset >= Pm4::SPI_SHADER_USER_DATA_PS_0 && cmd_offset <= Pm4::SPI_SHADER_USER_DATA_PS_15));

	uint32_t slot = (cmd_offset - Pm4::SPI_SHADER_USER_DATA_PS_0) / 1;

	auto reg_num = (cmd_id >> 16u) & 0x3fffu;

	EXIT_NOT_IMPLEMENTED(reg_num >= HW::UserSgprInfo::SGPRS_MAX);

	for (uint32_t i = 0; i < reg_num; i++)
	{
		cp->GetShCtx()->SetPsUserSgpr(slot + i, buffer[i], cp->GetUserDataMarker());
	}
	cp->SetUserDataMarker(HW::UserSgprType::Unknown);

	return reg_num;
}

KYTY_HW_SH_PARSER(hw_sh_set_vs_embedded)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc01b1034);

	auto shader_modifier = buffer[0];
	auto id              = buffer[1];

	cp->GetShCtx()->SetVsEmbedded(id, shader_modifier);

	return 28;
}

KYTY_HW_SH_PARSER(hw_sh_set_vs_shader)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC01B1004);

	auto shader_modifier = buffer[0];

	// HW::VsStageRegisters r {};

	uint64_t addr = (static_cast<uint64_t>(buffer[1]) << 8u) | (static_cast<uint64_t>(buffer[2] & 0xffu) << 40u);

	HW::VsShaderResource1 r1;
	HW::VsShaderResource2 r2;

	// r.m_spiShaderPgmLoVs    = buffer[1];
	// r.m_spiShaderPgmHiVs    = buffer[2];
	// r.m_spiShaderPgmRsrc1Vs = buffer[3];
	// r.m_spiShaderPgmRsrc2Vs = buffer[4];

	r1.vgprs                    = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, VGPRS);
	r1.sgprs                    = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, SGPRS);
	r1.priority                 = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, PRIORITY);
	r1.float_mode               = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, FLOAT_MODE);
	r1.dx10_clamp               = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, DX10_CLAMP) != 0;
	r1.ieee_mode                = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, IEEE_MODE) != 0;
	r1.vgpr_component_count     = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, VGPR_COMP_CNT);
	r1.cu_group_enable          = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, CU_GROUP_ENABLE) != 0;
	r1.require_forward_progress = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, FWD_PROGRESS) != 0;
	r1.fp16_overflow            = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, FP16_OVFL) != 0;

	r2.scratch_en = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, SCRATCH_EN) != 0;
	r2.user_sgpr  = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, USER_SGPR) +
	               (KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, USER_SGPR_MSB) << 5u);
	r2.offchip_lds       = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, OC_LDS_EN) != 0;
	r2.streamout_enabled = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, SO_EN) != 0;
	r2.shared_vgprs      = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, SHARED_VGPR_CNT);

	uint32_t m_spi_vs_out_config     = buffer[5];
	uint32_t m_spi_shader_pos_format = buffer[6];
	uint32_t m_pa_cl_vs_out_cntl     = buffer[7];

	// cp->GetShCtx()->SetVsShader(r, shader_modifier);
	cp->GetShCtx()->SetVsShaderBase(addr);
	cp->GetShCtx()->SetVsShaderModifier(shader_modifier);
	cp->GetShCtx()->SetVsShaderResource1(r1);
	cp->GetShCtx()->SetVsShaderResource2(r2);

	cp->GetCtx()->SetVsOutConfig(m_spi_vs_out_config);
	cp->GetCtx()->SetShaderPosFormat(m_spi_shader_pos_format);
	cp->GetCtx()->SetClVsOutCntl(m_pa_cl_vs_out_cntl);

	return 28;
}

KYTY_HW_SH_PARSER(hw_sh_set_vs_user_sgpr)
{
	EXIT_NOT_IMPLEMENTED(!(cmd_offset >= Pm4::SPI_SHADER_USER_DATA_VS_0 && cmd_offset <= Pm4::SPI_SHADER_USER_DATA_VS_15));

	uint32_t slot = (cmd_offset - Pm4::SPI_SHADER_USER_DATA_VS_0) / 1;

	auto reg_num = (cmd_id >> 16u) & 0x3fffu;

	EXIT_NOT_IMPLEMENTED(reg_num >= HW::UserSgprInfo::SGPRS_MAX);

	for (uint32_t i = 0; i < reg_num; i++)
	{
		cp->GetShCtx()->SetVsUserSgpr(slot + i, buffer[i], cp->GetUserDataMarker());
	}
	cp->SetUserDataMarker(HW::UserSgprType::Unknown);

	return reg_num;
}

KYTY_HW_SH_PARSER(hw_sh_set_gs_user_sgpr)
{
	EXIT_NOT_IMPLEMENTED(!(cmd_offset >= Pm4::SPI_SHADER_USER_DATA_GS_0 && cmd_offset <= Pm4::SPI_SHADER_USER_DATA_GS_15));

	uint32_t slot = (cmd_offset - Pm4::SPI_SHADER_USER_DATA_GS_0) / 1;

	auto reg_num = (cmd_id >> 16u) & 0x3fffu;

	EXIT_NOT_IMPLEMENTED(reg_num >= HW::UserSgprInfo::SGPRS_MAX);

	for (uint32_t i = 0; i < reg_num; i++)
	{
		cp->GetShCtx()->SetGsUserSgpr(slot + i, buffer[i], cp->GetUserDataMarker());
	}
	cp->SetUserDataMarker(HW::UserSgprType::Unknown);

	return reg_num;
}

KYTY_HW_SH_PARSER(hw_sh_update_ps_shader)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0261040);

	HW::PsShaderResource1 r1;
	HW::PsShaderResource2 r2;

	uint64_t addr = (static_cast<uint64_t>(buffer[0]) << 8u) | (static_cast<uint64_t>(buffer[1] & 0xffu) << 40u);

	r1.vgprs                    = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, VGPRS);
	r1.sgprs                    = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, SGPRS);
	r1.priority                 = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, PRIORITY);
	r1.float_mode               = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, FLOAT_MODE);
	r1.dx10_clamp               = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, DX10_CLAMP) != 0;
	r1.debug_mode               = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, DEBUG_MODE) != 0;
	r1.ieee_mode                = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, IEEE_MODE) != 0;
	r1.cu_group_disable         = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, CU_GROUP_DISABLE) != 0;
	r1.require_forward_progress = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, FWD_PROGRESS) != 0;
	r1.fp16_overflow            = KYTY_PM4_GET(buffer[2], SPI_SHADER_PGM_RSRC1_PS, FP16_OVFL) != 0;

	r2.scratch_en = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, SCRATCH_EN);
	r2.user_sgpr  = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, USER_SGPR) +
	               (KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, USER_SGPR_MSB) << 5u);
	r2.wave_cnt_en            = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, WAVE_CNT_EN);
	r2.extra_lds_size         = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, EXTRA_LDS_SIZE);
	r2.raster_ordered_shading = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, LOAD_INTRAWAVE_COLLISION);
	r2.shared_vgprs           = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC2_PS, SHARED_VGPR_CNT);

	cp->GetCtx()->SetShaderZFormat(buffer[4]);

	// cp->GetShCtx()->UpdatePsShader(r);
	cp->GetShCtx()->SetPsShaderBase(addr);
	cp->GetShCtx()->SetPsShaderResource1(r1);
	cp->GetShCtx()->SetPsShaderResource2(r2);

	return 39;
}

KYTY_HW_SH_PARSER(hw_sh_update_vs_shader)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc01b103c);

	auto shader_modifier = buffer[0];

	uint64_t addr = (static_cast<uint64_t>(buffer[1]) << 8u) | (static_cast<uint64_t>(buffer[2] & 0xffu) << 40u);

	HW::VsShaderResource1 r1;
	HW::VsShaderResource2 r2;

	r1.vgprs                    = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, VGPRS);
	r1.sgprs                    = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, SGPRS);
	r1.priority                 = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, PRIORITY);
	r1.float_mode               = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, FLOAT_MODE);
	r1.dx10_clamp               = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, DX10_CLAMP) != 0;
	r1.ieee_mode                = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, IEEE_MODE) != 0;
	r1.vgpr_component_count     = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, VGPR_COMP_CNT);
	r1.cu_group_enable          = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, CU_GROUP_ENABLE) != 0;
	r1.require_forward_progress = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, FWD_PROGRESS) != 0;
	r1.fp16_overflow            = KYTY_PM4_GET(buffer[3], SPI_SHADER_PGM_RSRC1_VS, FP16_OVFL) != 0;

	r2.scratch_en = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, SCRATCH_EN) != 0;
	r2.user_sgpr  = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, USER_SGPR) +
	               (KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, USER_SGPR_MSB) << 5u);
	r2.offchip_lds       = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, OC_LDS_EN) != 0;
	r2.streamout_enabled = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, SO_EN) != 0;
	r2.shared_vgprs      = KYTY_PM4_GET(buffer[4], SPI_SHADER_PGM_RSRC2_VS, SHARED_VGPR_CNT);

	cp->GetShCtx()->SetVsShaderBase(addr);
	cp->GetShCtx()->SetVsShaderModifier(shader_modifier);
	cp->GetShCtx()->SetVsShaderResource1(r1);
	cp->GetShCtx()->SetVsShaderResource2(r2);

	return 28;
}

KYTY_HW_UC_PARSER(hw_uc_set_primitive_type)
{
	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0017900);
	EXIT_NOT_IMPLEMENTED(cmd_offset != Pm4::VGT_PRIMITIVE_TYPE);

	uint32_t prim_type = KYTY_PM4_GET(buffer[0], VGT_PRIMITIVE_TYPE, PRIM_TYPE);

	cp->GetUcfg()->SetPrimitiveType(prim_type);

	return 1;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_CP_OP_PARSER(cp_op_acquire_mem)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0055800 && cmd_id != 0xc0061050);

	bool custom = (cmd_id == 0xc0061050);

	uint32_t                  stall_mode   = buffer[0] >> 31u;
	uint32_t                  cache_action = buffer[0] & 0x7fffffffu;
	uint64_t                  size_lo      = buffer[1];
	uint32_t                  size_hi      = buffer[2];
	uint64_t                  base_lo      = buffer[3];
	uint32_t                  base_hi      = buffer[4];
	uint32_t                  poll         = buffer[5];
	[[maybe_unused]] uint32_t gcr_cntl     = (custom ? buffer[6] : 0);

	uint32_t target_mask     = cache_action & 0x00007FC0u;
	uint32_t extended_action = cache_action & 0x2E000000u;
	uint32_t action          = ((cache_action & 0x00C00000u) >> 0x12u) | ((cache_action & 0x00058000u) >> 0xfu);

	// EXIT_NOT_IMPLEMENTED(stall_mode != 1);
	EXIT_NOT_IMPLEMENTED(size_hi != 0);
	EXIT_NOT_IMPLEMENTED(base_hi != 0);
	EXIT_NOT_IMPLEMENTED(poll != 10);

	switch (cache_action)
	{
		case 0x02c40040:
		case 0x02c43fc0:
		case 0x02c47fc0:
		{
			// target_mask:     0x00000040 (rt0), 0x00003fc0 (all rt), 0x00007fc0 (all rt and depth)
			// extended_action: 0x02000000 (FlushAndInvalidateCbCache)
			// action:          0x38 (WriteBackAndInvalidateL1andL2)
			EXIT_IF(target_mask != 0x00000040 && target_mask != 0x00003FC0 && target_mask != 0x00007FC0);
			EXIT_IF(extended_action != 0x02000000);
			EXIT_IF(action != 0x38);
			EXIT_NOT_IMPLEMENTED(size_lo == 0);
			EXIT_NOT_IMPLEMENTED(base_lo == 0);

			cp->RenderTextureBarrier(base_lo << 8u, size_lo << 8u);
			cp->WriteBack();
		}
		break;
		case 0x02003fc0:
		{
			// target_mask:     0x00003FC0 (all rt)
			// extended_action: 0x02000000 (FlushAndInvalidateCbCache)
			// action:          0x00 (none)
			EXIT_IF(target_mask != 0x00003FC0);
			EXIT_IF(extended_action != 0x02000000);
			EXIT_IF(action != 0x00);
			EXIT_NOT_IMPLEMENTED(size_lo == 0);
			EXIT_NOT_IMPLEMENTED(base_lo == 0);

			cp->RenderTextureBarrier(base_lo << 8u, size_lo << 8u);
		}
		break;
		case 0x00C40000:
		{
			// target_mask:     0x00000000 (none)
			// extended_action: 0x00000000 (none)
			// action:          0x38 (WriteBackAndInvalidateL1andL2)
			EXIT_IF(target_mask != 0x00000000);
			EXIT_IF(extended_action != 0x00000000);
			EXIT_IF(action != 0x38);
			EXIT_NOT_IMPLEMENTED(size_lo != 1);
			EXIT_NOT_IMPLEMENTED(base_lo != 0);

			cp->MemoryBarrier();
			cp->WriteBack();
		}
		break;
		case 0x00400000:
		{
			// target_mask:     0x00000000 (none)
			// extended_action: 0x00000000 (none)
			// action:          0x10 (InvalidateL1)
			EXIT_IF(target_mask != 0x00000000);
			EXIT_IF(extended_action != 0x00000000);
			EXIT_IF(action != 0x10);
			EXIT_NOT_IMPLEMENTED(size_lo != 1);
			EXIT_NOT_IMPLEMENTED(base_lo != 0);

			cp->MemoryBarrier();
		}
		break;
		case 0x04c44000:
		{
			// target_mask:     0x00004000 (Depth Target)
			// extended_action: 0x04000000 (FlushAndInvalidateDbCache)
			// action:          0x38 (WriteBackAndInvalidateL1andL2)
			EXIT_IF(target_mask != 0x00004000);
			EXIT_IF(extended_action != 0x04000000);
			EXIT_IF(action != 0x38);

			cp->DepthStencilBarrier(base_lo << 8u, size_lo << 8u);
			cp->WriteBack();
		}
		break;

		case 0x06007fc0:
		{
			// target_mask:     0x00007fc0 (all rt and depth)
			// extended_action: 0x06000000 (Flush Cb & Db)
			// action:          0x00 (none)
			EXIT_NOT_IMPLEMENTED(gcr_cntl != 0x280); // kGl0ScalarInvalidate & kGl1Invalidate
			EXIT_NOT_IMPLEMENTED(size_lo != 0);
			EXIT_NOT_IMPLEMENTED(base_lo != 0);

			EXIT_IF(target_mask != 0x00007fc0);
			EXIT_IF(extended_action != 0x06000000);
			EXIT_IF(action != 0x00);

			cp->MemoryBarrier();
		}
		break;

		default:
			EXIT("unknown barrier: 0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 "\n", cache_action, target_mask,
			     extended_action, action);
	}

	if (stall_mode == 0)
	{
		cp->BufferFlush();
		cp->BufferWait();
	}

	return (custom ? 7 : 6);
}

KYTY_CP_OP_PARSER(cp_op_dispatch_direct)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0071020);

	uint32_t thread_group_x = buffer[0];
	uint32_t thread_group_y = buffer[1];
	uint32_t thread_group_z = buffer[2];
	uint32_t mode           = buffer[3];

	cp->DispatchDirect(thread_group_x, thread_group_y, thread_group_z, mode);

	return 8;
}

KYTY_CP_OP_PARSER(cp_op_dispatch_reset)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0001024);

	cp->Reset();

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_dma_data)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0055000);

	uint32_t type  = buffer[0];
	uint32_t type2 = (buffer[5] >> 21u);
	uint64_t src   = (buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));
	uint64_t dst   = (buffer[3] | (static_cast<uint64_t>(buffer[4]) << 32u));
	uint32_t size  = buffer[5] & 0x1fffffu;

	if (type == 0x60000000 && dst == 0x0003022C && type2 == 0x141u)
	{
		auto* addr = reinterpret_cast<void*>(src);

		cp->PrefetchL2(addr, size);
	} else if (type == 0xc4104000 && type2 == 0x0u)
	{
		EXIT_NOT_IMPLEMENTED((dst & 0x3u) != 0);
		EXIT_NOT_IMPLEMENTED((size & 0x3u) != 0);

		cp->ClearGds(dst / 4, size / 4, static_cast<uint32_t>(src));
	} else if (type == 0xa4004000 && type2 == 0x0u)
	{
		EXIT_NOT_IMPLEMENTED((src & 0x3u) != 0);
		EXIT_NOT_IMPLEMENTED((size & 0x3u) != 0);

		auto* addr = reinterpret_cast<uint32_t*>(dst);

		cp->ReadGds(addr, src / 4, size / 4);
	} else
	{
		EXIT("unknown dma\n");
	}

	return 6;
}

KYTY_CP_OP_PARSER(cp_op_draw_index)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC008100C && cmd_id != 0xc0042700);

	if (cmd_id == 0xC008100C)
	{
		uint32_t index_count = buffer[0];
		auto*    index_addr  = reinterpret_cast<void*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));
		uint32_t flags       = buffer[3];
		uint32_t type        = buffer[4];

		cp->DrawIndex(index_count, index_addr, flags, type);

		return 9;
	}

	if (cmd_id == 0xc0042700)
	{
		uint32_t index_count = buffer[0];
		auto*    index_addr  = reinterpret_cast<void*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));

		EXIT_NOT_IMPLEMENTED(buffer[3] != index_count);
		EXIT_NOT_IMPLEMENTED(buffer[4] != 0);

		cp->DrawIndex(index_count, index_addr, 0, 1);

		EXIT_NOT_IMPLEMENTED(!(dw >= 7));

		if (buffer[5] == 0xc0001000)
		{
			EXIT_NOT_IMPLEMENTED(buffer[6] != 0);

			return 7;
		}

		if (buffer[5] == 0xc0021000)
		{
			EXIT_NOT_IMPLEMENTED(buffer[6] != 0);

			return 9;
		}

		EXIT("invalid draw_index\n");
	}

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_draw_index_auto)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0051010 && cmd_id != 0xc0012d00);

	if (cmd_id == 0xC0051010)
	{
		uint32_t index_count = buffer[0];
		uint32_t flags       = buffer[1];

		cp->DrawIndexAuto(index_count, flags);

		return 6;
	}

	if (cmd_id == 0xc0012d00)
	{
		uint32_t index_count = buffer[0];
		uint32_t flags       = 0;

		EXIT_NOT_IMPLEMENTED(buffer[1] != 2);

		cp->DrawIndexAuto(index_count, flags);

		EXIT_NOT_IMPLEMENTED(!(dw >= 4));

		if (buffer[2] == 0xc0001000)
		{
			EXIT_NOT_IMPLEMENTED(buffer[3] != 0);

			return 4;
		}

		if (buffer[2] == 0xc0021000)
		{
			EXIT_NOT_IMPLEMENTED(buffer[3] != 0);

			return 6;
		}

		EXIT("invalid draw_index_auto\n");
	}

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_draw_reset)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0001014);

	cp->Reset();

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_dump_const_ram)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0038300);

	auto  offset = buffer[0];
	auto  dw_num = buffer[1];
	auto* dst    = reinterpret_cast<uint32_t*>(buffer[2] | (static_cast<uint64_t>(buffer[3]) << 32u));

	EXIT_NOT_IMPLEMENTED(dw_num >= 0x3000);
	EXIT_NOT_IMPLEMENTED(offset > 0xbffc);
	EXIT_NOT_IMPLEMENTED((offset & 0x3u) != 0);

	cp->DumpConstRam(dst, offset, dw_num);

	return 4;
}

KYTY_CP_OP_PARSER(cp_op_event_write)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0004600);

	uint32_t event_index = (buffer[0] >> 8u) & 0x7u;
	uint32_t event_type  = (buffer[0]) & 0x3fu;

	cp->TriggerEvent(event_type, event_index);

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_event_write_eop)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0044700);

	uint32_t cache_policy       = (buffer[0] >> 25u) & 0x3u;
	uint32_t event_write_dest   = ((buffer[0] >> 23u) & 0x10u) | ((buffer[2] >> 16u) & 0x01u);
	uint32_t eop_event_type     = (buffer[0]) & 0x3fu;
	uint32_t cache_action       = (buffer[0] >> 12u) & 0x3fu;
	uint32_t event_index        = (buffer[0] >> 8u) & 0x7u;
	uint32_t event_write_source = ((buffer[2] >> 29u) & 0x7u);
	uint32_t interrupt_selector = (buffer[2] >> 24u) & 0x7u;
	auto*    dst_gpu_addr       = reinterpret_cast<void*>(buffer[1] | (static_cast<uint64_t>(buffer[2] & 0xffffu) << 32u));
	uint64_t value              = (buffer[3] | (static_cast<uint64_t>(buffer[4]) << 32u));

	cp->WriteAtEndOfPipe64(cache_policy, event_write_dest, eop_event_type, cache_action, event_index, event_write_source, dst_gpu_addr,
	                       value, interrupt_selector);

	return 5;
}

KYTY_CP_OP_PARSER(cp_op_event_write_eos)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0034802);

	uint32_t cache_policy       = (buffer[0] >> 25u) & 0x3u;
	uint32_t event_write_dest   = 0;
	uint32_t eop_event_type     = (buffer[0]) & 0x3fu;
	uint32_t cache_action       = (buffer[0] >> 12u) & 0x3fu;
	uint32_t event_index        = (buffer[0] >> 8u) & 0x7u;
	uint32_t event_write_source = ((buffer[2] >> 29u) & 0x7u);
	uint32_t interrupt_selector = (buffer[2] >> 24u) & 0x7u;

	auto*    dst_gpu_addr = reinterpret_cast<void*>(buffer[1] | (static_cast<uint64_t>(buffer[2] & 0xffffu) << 32u));
	uint32_t value        = buffer[3];

	cp->WriteAtEndOfPipe32(cache_policy, event_write_dest, eop_event_type, cache_action, event_index, event_write_source, dst_gpu_addr,
	                       value, interrupt_selector);

	return 4;
}

KYTY_CP_OP_PARSER(cp_op_flip)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc004105c);

	CommandProcessor::FlipInfo f;

	f.handle    = static_cast<int>(buffer[0]);
	f.index     = static_cast<int>(buffer[1]);
	f.flip_mode = static_cast<int>(buffer[2]);
	f.flip_arg  = static_cast<int64_t>(buffer[3] | (static_cast<uint64_t>(buffer[4]) << 32u));

	cp->SetFlip(f);
	cp->Flip();

	return 5;
}

KYTY_CP_OP_PARSER(cp_op_increment_ce_counter)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0008400);
	EXIT_NOT_IMPLEMENTED(buffer[0] != 1);

	cp->IncremenetCe();

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_increment_de_counter)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0008500);
	EXIT_NOT_IMPLEMENTED(buffer[0] != 0);

	cp->IncremenetDe();

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_index_type)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0002A00);

	cp->SetIndexType(buffer[0]);

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_indirect_buffer)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0023f02);

	EXIT_NOT_IMPLEMENTED((buffer[2] & 0xff00000u) != 0x1800000u);

	auto*    indirect_buffer = reinterpret_cast<uint32_t*>(buffer[0] | (static_cast<uint64_t>(buffer[1] & 0xffffu) << 32u));
	uint32_t indirect_num_dw = buffer[2] & 0xfffffu;

	EXIT_NOT_IMPLEMENTED(indirect_buffer == nullptr);
	EXIT_NOT_IMPLEMENTED(indirect_num_dw == 0);

	GraphicsDbgDumpDcb("ci", indirect_num_dw, indirect_buffer);

	cp->Run(indirect_buffer, indirect_num_dw);

	return 3;
}

KYTY_CP_OP_PARSER(cp_op_indirect_cx_regs)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0021048);

	auto*    indirect_buffer = reinterpret_cast<uint32_t*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));
	uint32_t indirect_num_dw = buffer[0];

	EXIT_NOT_IMPLEMENTED(indirect_buffer == nullptr);
	EXIT_NOT_IMPLEMENTED(indirect_num_dw == 0);

	for (uint32_t i = 0; i < indirect_num_dw; i++, indirect_buffer += 2)
	{
		auto cmd_offset = indirect_buffer[0];
		auto value      = indirect_buffer[1];

		EXIT_NOT_IMPLEMENTED(cmd_offset >= Pm4::CX_NUM);

		auto pfunc = g_hw_ctx_indirect_func[cmd_offset & (Pm4::CX_NUM - 1)];

		if (pfunc == nullptr)
		{
			EXIT("unknown cx reg at %05" PRIx32 ": 0x%" PRIx32 "\n", num_dw - dw, cmd_offset);
		}

		pfunc(cp, cmd_offset, value);
	}

	return 3;
}

KYTY_CP_OP_PARSER(cp_op_indirect_sh_regs)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0021044);

	auto*    indirect_buffer = reinterpret_cast<uint32_t*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));
	uint32_t indirect_num_dw = buffer[0];

	EXIT_NOT_IMPLEMENTED(indirect_buffer == nullptr);
	EXIT_NOT_IMPLEMENTED(indirect_num_dw == 0);

	for (uint32_t i = 0; i < indirect_num_dw; i++, indirect_buffer += 2)
	{
		auto cmd_offset = indirect_buffer[0];
		auto value      = indirect_buffer[1];

		EXIT_NOT_IMPLEMENTED(cmd_offset >= Pm4::SH_NUM);

		auto pfunc = g_hw_sh_indirect_func[cmd_offset & (Pm4::SH_NUM - 1)];

		if (pfunc == nullptr)
		{
			EXIT("unknown sh reg at %05" PRIx32 ": 0x%" PRIx32 "\n", num_dw - dw, cmd_offset);
		}

		pfunc(cp, cmd_offset, value);
	}

	return 3;
}

KYTY_CP_OP_PARSER(cp_op_indirect_uc_regs)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc002104c);

	auto*    indirect_buffer = reinterpret_cast<uint32_t*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));
	uint32_t indirect_num_dw = buffer[0];

	EXIT_NOT_IMPLEMENTED(indirect_buffer == nullptr);
	EXIT_NOT_IMPLEMENTED(indirect_num_dw == 0);

	for (uint32_t i = 0; i < indirect_num_dw; i++, indirect_buffer += 2)
	{
		auto cmd_offset = indirect_buffer[0];
		auto value      = indirect_buffer[1];

		EXIT_NOT_IMPLEMENTED(cmd_offset >= Pm4::UC_NUM);

		auto pfunc = g_hw_uc_indirect_func[cmd_offset & (Pm4::UC_NUM - 1)];

		if (pfunc == nullptr)
		{
			EXIT("unknown uc reg at %05" PRIx32 ": 0x%" PRIx32 "\n", num_dw - dw, cmd_offset);
		}

		pfunc(cp, cmd_offset, value);
	}

	return 3;
}

KYTY_CP_OP_PARSER(cp_op_marker)
{
	KYTY_PROFILER_FUNCTION();

	// EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0001000);

	uint32_t id     = buffer[0] & 0xfffu;
	uint32_t align  = (buffer[0] >> 12u) & 0xfu;
	uint32_t len_dw = ((cmd_id >> 16u) & 0x3fffu);

	switch (id)
	{
		case 0x0: cp->SetEmbeddedDataMarker(buffer + 1, len_dw, align); break;
		case 0x4: cp->SetUserDataMarker(HW::UserSgprType::Vsharp); break;
		case 0xd: cp->SetUserDataMarker(HW::UserSgprType::Region); break;
		case 0x777:
		{
			cp->Flip();
			break;
		}
		case 0x778:
		{
			auto*    addr  = reinterpret_cast<void*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));
			uint32_t value = buffer[3];
			cp->Flip(addr, value);
			break;
		}
		case 0x781:
		{
			auto*    addr           = reinterpret_cast<void*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));
			uint32_t value          = buffer[3];
			uint32_t eop_event_type = buffer[4];
			uint32_t cache_action   = buffer[5];
			cp->FlipWithInterrupt(eop_event_type, cache_action, addr, value);
			break;
		}
		default: EXIT("unknown marker at %05" PRIx32 ": 0x%" PRIx32 "\n", num_dw - dw, id); break;
	}

	return len_dw + 1;
}

KYTY_CP_OP_PARSER(cp_op_nop)
{
	KYTY_PROFILER_FUNCTION();

	auto r = KYTY_PM4_R(cmd_id);

	if (r == Pm4::R_ZERO)
	{
		if ((buffer[0] & 0xffff0000u) == 0x68750000u)
		{
			return cp_op_marker(cp, cmd_id, buffer, dw, num_dw);
		}
	}

	auto hw_ctx = g_hw_sh_custom_func[r];
	auto cp_op  = g_cp_op_custom_func[r];

	if (hw_ctx != nullptr && cp_op == nullptr)
	{
		return hw_ctx(cp, cmd_id, 0, buffer, dw);
	}

	if (hw_ctx == nullptr && cp_op != nullptr)
	{
		return cp_op(cp, cmd_id, buffer, dw, num_dw);
	}

	EXIT("unknown custom code at 0x%05" PRIx32 ": 0x%02" PRIx32 "\n", num_dw - dw, r);

	return 0;
}

KYTY_CP_OP_PARSER(cp_op_num_instances)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0002f00);

	cp->SetNumInstances(buffer[0]);

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_pop_marker)
{
	KYTY_PROFILER_FUNCTION();

	auto dw_num = (cmd_id >> 16u) & 0x3fffu;

	printf("Pop marker\n");

	cp->PopMarker();

	return dw_num + 1;
}

KYTY_CP_OP_PARSER(cp_op_push_marker)
{
	KYTY_PROFILER_FUNCTION();

	auto dw_num = (cmd_id >> 16u) & 0x3fffu;

	const char* str = reinterpret_cast<const char*>(buffer);

	printf("Push marker: %s\n", str);

	cp->PushMarker(str);

	return dw_num + 1;
}

KYTY_CP_OP_PARSER(cp_op_release_mem)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0054902 && cmd_id != 0xc0051060);

	bool custom = (cmd_id == 0xc0051060);

	uint32_t cache_policy       = (buffer[0] >> 25u) & 0x3u;
	uint32_t cache_action       = (buffer[0] >> 12u) & 0x3fu;
	uint32_t eop_event_type     = (buffer[0]) & 0x3fu;
	uint32_t event_index        = (buffer[0] >> 8u) & 0x7u;
	uint32_t event_write_dest   = ((buffer[0] >> 23u) & 0x10u) | ((buffer[1] >> 16u) & 0x01u);
	uint32_t event_write_source = (buffer[1] >> 29u) & 0x7u;
	uint32_t interrupt_selector = (buffer[1] >> 24u) & 0x7u;
	auto*    dst_gpu_addr       = reinterpret_cast<void*>(buffer[2] | (static_cast<uint64_t>(buffer[3]) << 32u));
	uint64_t value              = (buffer[4] | (static_cast<uint64_t>(buffer[5]) << 32u));

	if (custom)
	{
		uint32_t gcr_cntl = buffer[1] & 0xffffu;
		uint32_t data_sel = (buffer[1] >> 16u) & 0xffu;

		EXIT_NOT_IMPLEMENTED(data_sel != 2 && data_sel != 3);

		if (data_sel == 2)
		{
			EXIT_NOT_IMPLEMENTED(gcr_cntl != 0x0200);
			EXIT_NOT_IMPLEMENTED((buffer[0] >> 8u) != 0x3);

			if (gcr_cntl == 0x200)
			{
				cache_action = 0x38;
			}

			cache_policy       = 0;
			event_index        = 0;
			event_write_dest   = 0;
			event_write_source = 2;
			interrupt_selector = 0;
		} else if (data_sel == 3)
		{
			EXIT_NOT_IMPLEMENTED(gcr_cntl != 0x0000);
			EXIT_NOT_IMPLEMENTED((buffer[0] >> 8u) != 0x0);

			cache_action = 0x00;

			cache_policy       = 0;
			event_index        = 0;
			event_write_dest   = 0;
			event_write_source = 4;
			interrupt_selector = 0;
		}
	}

	cp->WriteAtEndOfPipe64(cache_policy, event_write_dest, eop_event_type, cache_action, event_index, event_write_source, dst_gpu_addr,
	                       value, interrupt_selector);

	return 6;
}

KYTY_CP_OP_PARSER(cp_op_set_context_reg)
{
	KYTY_PROFILER_FUNCTION();

	auto cmd_offset = buffer[0];

	EXIT_NOT_IMPLEMENTED(cmd_offset >= Pm4::CX_NUM);

	auto pfunc = g_hw_ctx_func[cmd_offset & (Pm4::CX_NUM - 1)];

	if (pfunc == nullptr)
	{
		EXIT("unknown context register\n\t%05" PRIx32 ":\n\tcmd_id = %08" PRIx32 "\n\tcmd_offset = %08" PRIx32 "\n", num_dw - dw, cmd_id,
		     cmd_offset);
	}

	auto s = pfunc(cp, cmd_id, cmd_offset, buffer + 1, dw);

	EXIT_IF(s == 0);

	return s + 1;
}

KYTY_CP_OP_PARSER(cp_op_set_shader_reg)
{
	KYTY_PROFILER_FUNCTION();

	auto cmd_offset = buffer[0];

	EXIT_NOT_IMPLEMENTED(cmd_offset >= Pm4::SH_NUM);

	auto pfunc = g_hw_sh_func[cmd_offset & (Pm4::SH_NUM - 1)];

	if (pfunc == nullptr)
	{
		EXIT("unknown shader register\n\t%05" PRIx32 ":\n\tcmd_id = %08" PRIx32 "\n\tcmd_offset = %08" PRIx32 "\n", num_dw - dw, cmd_id,
		     cmd_offset);
	}

	auto s = pfunc(cp, cmd_id, cmd_offset, buffer + 1, dw);
	EXIT_IF(s == 0);
	return s + 1;
}

KYTY_CP_OP_PARSER(cp_op_set_uconfig_reg)
{
	KYTY_PROFILER_FUNCTION();

	auto cmd_offset = buffer[0] & 0xEFFFFFFFu; // ignore neo bit

	EXIT_NOT_IMPLEMENTED(cmd_offset >= Pm4::UC_NUM);

	auto pfunc = g_hw_uc_func[cmd_offset & (Pm4::UC_NUM - 1)];

	if (pfunc == nullptr)
	{
		EXIT("unknown user config register\n\t%05" PRIx32 ":\n\tcmd_id = %08" PRIx32 "\n\tcmd_offset = %08" PRIx32 "\n", num_dw - dw,
		     cmd_id, cmd_offset);
	}

	auto s = pfunc(cp, cmd_id, cmd_offset, buffer + 1, dw);
	EXIT_IF(s == 0);
	return s + 1;
}

KYTY_CP_OP_PARSER(cp_op_wait_flip_done)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0051018);

	auto video_out_handle     = buffer[0];
	auto display_buffer_index = buffer[1];

	cp->WaitFlipDone(video_out_handle, display_buffer_index);

	return 6;
}

KYTY_CP_OP_PARSER(cp_op_wait_reg_mem_32)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC00C1028);

	auto* addr = reinterpret_cast<uint32_t*>(buffer[0] | (static_cast<uint64_t>(buffer[1]) << 32u));
	auto  mask = buffer[2];
	auto  func = buffer[3];
	auto  ref  = buffer[4];
	auto  poll = 10;

	cp->WaitRegMem32(func, addr, ref, mask, poll);

	return 13;
}

KYTY_CP_OP_PARSER(cp_op_wait_reg_mem_64)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xc0071058);

	auto* addr = reinterpret_cast<uint64_t*>(buffer[0] | (static_cast<uint64_t>(buffer[1]) << 32u));
	auto  mask = buffer[2] | (static_cast<uint64_t>(buffer[3]) << 32u);
	auto  ref  = buffer[4] | (static_cast<uint64_t>(buffer[5]) << 32u);
	auto  func = buffer[6];
	auto  poll = buffer[7];

	cp->WaitRegMem64(func, addr, ref, mask, poll);

	return 8;
}

KYTY_CP_OP_PARSER(cp_op_wait_on_ce_counter)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0008600);
	EXIT_NOT_IMPLEMENTED(buffer[0] != 1);

	cp->WaitCe();

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_wait_on_de_counter_diff)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0008800);

	cp->WaitDeDiff(buffer[0]);

	return 1;
}

KYTY_CP_OP_PARSER(cp_op_wait_reg_mem)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(cmd_id != 0xC0053C00);

	auto  func = buffer[0] & 0x7u;
	bool  me   = (buffer[0] & 0x100u) == 0;
	bool  mem  = (buffer[0] & 0x10u) != 0;
	auto* addr = reinterpret_cast<uint32_t*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));
	auto  ref  = buffer[3];
	auto  mask = buffer[4];
	auto  poll = buffer[5];

	EXIT_NOT_IMPLEMENTED(!me);
	EXIT_NOT_IMPLEMENTED(!mem);

	cp->WaitRegMem32(func, addr, ref, mask, poll);

	return 6;
}

KYTY_CP_OP_PARSER(cp_op_write_const_ram)
{
	KYTY_PROFILER_FUNCTION();

	auto dw_num = (cmd_id >> 16u) & 0x3fffu;
	auto offset = buffer[0];

	EXIT_NOT_IMPLEMENTED(dw_num >= 0x3000);
	EXIT_NOT_IMPLEMENTED(offset > 0xbffc);
	EXIT_NOT_IMPLEMENTED((offset & 0x3u) != 0);

	cp->WriteConstRam(offset, buffer + 1, dw_num);

	return 1 + dw_num;
}

KYTY_CP_OP_PARSER(cp_op_write_data)
{
	KYTY_PROFILER_FUNCTION();

	auto op = (cmd_id >> 8u) & 0xffu;

	EXIT_NOT_IMPLEMENTED(op != Pm4::IT_WRITE_DATA && op != Pm4::IT_NOP);

	bool custom = (op == Pm4::IT_NOP);

	auto dw_num = (cmd_id >> 16u) & 0x3fffu;

	auto  write_control = buffer[0];
	auto* dst           = reinterpret_cast<uint32_t*>(buffer[1] | (static_cast<uint64_t>(buffer[2]) << 32u));

	cp->WriteData(dst, buffer + 3, dw_num - 2, write_control, custom);

	return 1 + dw_num;
}

static void graphics_init_jmp_tables_cx_indirect()
{
	for (auto& func: g_hw_ctx_indirect_func)
	{
		func = nullptr;
	}

	for (auto cmd_offset = Pm4::SPI_PS_INPUT_CNTL_0; cmd_offset <= Pm4::SPI_PS_INPUT_CNTL_31; cmd_offset++)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = cmd_offset - Pm4::SPI_PS_INPUT_CNTL_0;
			cp->GetCtx()->SetPsInputSettings(slot, value);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_BASE; cmd_offset <= Pm4::CB_COLOR7_BASE; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_BASE) / 15;
			auto     base = cp->GetCtx()->GetRenderTarget(slot).base;
			base.addr &= 0xFFFFFF00000000FFull;
			base.addr |= static_cast<uint64_t>(value) << 8u;
			cp->GetCtx()->SetColorBase(slot, base);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_BASE_EXT; cmd_offset <= Pm4::CB_COLOR7_BASE_EXT; cmd_offset++)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_BASE_EXT);
			auto     base = cp->GetCtx()->GetRenderTarget(slot).base;
			base.addr &= 0xFFFF00FFFFFFFFFFull;
			base.addr |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
			cp->GetCtx()->SetColorBase(slot, base);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_VIEW; cmd_offset <= Pm4::CB_COLOR7_VIEW; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t      slot = (cmd_offset - Pm4::CB_COLOR0_VIEW) / 15;
			HW::ColorView view;
			view.base_array_slice_index = KYTY_PM4_GET(value, CB_COLOR0_VIEW, SLICE_START);
			view.last_array_slice_index = KYTY_PM4_GET(value, CB_COLOR0_VIEW, SLICE_MAX);
			view.current_mip_level      = KYTY_PM4_GET(value, CB_COLOR0_VIEW, MIP_LEVEL);
			cp->GetCtx()->SetColorView(slot, view);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_INFO; cmd_offset <= Pm4::CB_COLOR7_INFO; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t      slot = (cmd_offset - Pm4::CB_COLOR0_INFO) / 15;
			HW::ColorInfo info;
			info.format                         = KYTY_PM4_GET(value, CB_COLOR0_INFO, FORMAT);
			info.channel_type                   = KYTY_PM4_GET(value, CB_COLOR0_INFO, NUMBER_TYPE);
			info.channel_order                  = KYTY_PM4_GET(value, CB_COLOR0_INFO, COMP_SWAP);
			info.cmask_fast_clear_enable        = KYTY_PM4_GET(value, CB_COLOR0_INFO, FAST_CLEAR) != 0;
			info.fmask_compression_enable       = KYTY_PM4_GET(value, CB_COLOR0_INFO, COMPRESSION) != 0;
			info.blend_clamp                    = KYTY_PM4_GET(value, CB_COLOR0_INFO, BLEND_CLAMP) != 0;
			info.blend_bypass                   = KYTY_PM4_GET(value, CB_COLOR0_INFO, BLEND_BYPASS) != 0;
			info.round_mode                     = KYTY_PM4_GET(value, CB_COLOR0_INFO, ROUND_MODE) != 0;
			info.cmask_tile_mode                = KYTY_PM4_GET(value, CB_COLOR0_INFO, CMASK_IS_LINEAR);
			info.fmask_data_compression_disable = KYTY_PM4_GET(value, CB_COLOR0_INFO, FMASK_COMPRESSION_DISABLE) != 0;
			info.fmask_one_frag_mode            = KYTY_PM4_GET(value, CB_COLOR0_INFO, FMASK_COMPRESS_1FRAG_ONLY) != 0;
			info.dcc_compression_enable         = KYTY_PM4_GET(value, CB_COLOR0_INFO, DCC_ENABLE) != 0;
			info.cmask_tile_mode_neo            = KYTY_PM4_GET(value, CB_COLOR0_INFO, CMASK_ADDR_TYPE);
			info.neo_mode                       = KYTY_PM4_GET(value, CB_COLOR0_INFO, ALT_TILE_MODE) != 0;
			if (!info.neo_mode && Config::IsNextGen())
			{
				info.neo_mode = true;
			}
			cp->GetCtx()->SetColorInfo(slot, info);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_ATTRIB; cmd_offset <= Pm4::CB_COLOR7_ATTRIB; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t        slot = (cmd_offset - Pm4::CB_COLOR0_ATTRIB) / 15;
			HW::ColorAttrib attrib;
			attrib.force_dest_alpha_to_one = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB, FORCE_DST_ALPHA_1) != 0;
			attrib.tile_mode               = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB, TILE_MODE_INDEX);
			attrib.fmask_tile_mode         = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB, FMASK_TILE_MODE_INDEX);
			attrib.num_samples             = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB, NUM_SAMPLES);
			attrib.num_fragments           = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB, NUM_FRAGMENTS);
			cp->GetCtx()->SetColorAttrib(slot, attrib);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_DCC_CONTROL; cmd_offset <= Pm4::CB_COLOR7_DCC_CONTROL; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t            slot = (cmd_offset - Pm4::CB_COLOR0_DCC_CONTROL) / 15;
			HW::ColorDccControl dcc;
			dcc.overwrite_combiner_disable     = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, OVERWRITE_COMBINER_DISABLE) != 0;
			dcc.dcc_clear_key_enable           = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, KEY_CLEAR_ENABLE) != 0;
			dcc.max_uncompressed_block_size    = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, MAX_UNCOMPRESSED_BLOCK_SIZE);
			dcc.min_compressed_block_size      = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, MIN_COMPRESSED_BLOCK_SIZE);
			dcc.max_compressed_block_size      = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, MAX_COMPRESSED_BLOCK_SIZE);
			dcc.color_transform                = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, COLOR_TRANSFORM);
			dcc.independent_64b_blocks         = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, INDEPENDENT_64B_BLOCKS) != 0;
			dcc.data_write_on_dcc_clear_to_reg = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, ENABLE_CONSTANT_ENCODE_REG_WRITE) != 0;
			dcc.independent_128b_blocks        = KYTY_PM4_GET(value, CB_COLOR0_DCC_CONTROL, INDEPENDENT_128B_BLOCKS) != 0;
			cp->GetCtx()->SetColorDccControl(slot, dcc);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_CMASK; cmd_offset <= Pm4::CB_COLOR7_CMASK; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_CMASK) / 15;
			auto     base = cp->GetCtx()->GetRenderTarget(slot).cmask;
			base.addr &= 0xFFFFFF00000000FFull;
			base.addr |= static_cast<uint64_t>(value) << 8u;
			cp->GetCtx()->SetColorCmask(slot, base);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_CMASK_BASE_EXT; cmd_offset <= Pm4::CB_COLOR7_CMASK_BASE_EXT; cmd_offset++)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_CMASK_BASE_EXT);
			auto     base = cp->GetCtx()->GetRenderTarget(slot).cmask;
			base.addr &= 0xFFFF00FFFFFFFFFFull;
			base.addr |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
			cp->GetCtx()->SetColorCmask(slot, base);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_FMASK; cmd_offset <= Pm4::CB_COLOR7_FMASK; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_FMASK) / 15;
			auto     base = cp->GetCtx()->GetRenderTarget(slot).fmask;
			base.addr &= 0xFFFFFF00000000FFull;
			base.addr |= static_cast<uint64_t>(value) << 8u;
			cp->GetCtx()->SetColorFmask(slot, base);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_FMASK_BASE_EXT; cmd_offset <= Pm4::CB_COLOR7_FMASK_BASE_EXT; cmd_offset++)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_FMASK_BASE_EXT);
			auto     base = cp->GetCtx()->GetRenderTarget(slot).fmask;
			base.addr &= 0xFFFF00FFFFFFFFFFull;
			base.addr |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
			cp->GetCtx()->SetColorFmask(slot, base);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_CLEAR_WORD0; cmd_offset <= Pm4::CB_COLOR7_CLEAR_WORD0; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			HW::ColorClearWord0 clear_word0;
			clear_word0.word0 = value;
			uint32_t slot     = (cmd_offset - Pm4::CB_COLOR0_CLEAR_WORD0) / 15;
			cp->GetCtx()->SetColorClearWord0(slot, clear_word0);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_CLEAR_WORD1; cmd_offset <= Pm4::CB_COLOR7_CLEAR_WORD1; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			HW::ColorClearWord1 clear_word1;
			clear_word1.word1 = value;
			uint32_t slot     = (cmd_offset - Pm4::CB_COLOR0_CLEAR_WORD1) / 15;
			cp->GetCtx()->SetColorClearWord1(slot, clear_word1);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_DCC_BASE; cmd_offset <= Pm4::CB_COLOR7_DCC_BASE; cmd_offset += 15)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_DCC_BASE) / 15;
			auto     base = cp->GetCtx()->GetRenderTarget(slot).dcc_addr;
			base.addr &= 0xFFFFFF00000000FFull;
			base.addr |= static_cast<uint64_t>(value) << 8u;
			cp->GetCtx()->SetColorDccAddr(slot, base);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_DCC_BASE_EXT; cmd_offset <= Pm4::CB_COLOR7_DCC_BASE_EXT; cmd_offset++)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t slot = (cmd_offset - Pm4::CB_COLOR0_DCC_BASE_EXT);
			auto     base = cp->GetCtx()->GetRenderTarget(slot).dcc_addr;
			base.addr &= 0xFFFF00FFFFFFFFFFull;
			base.addr |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
			cp->GetCtx()->SetColorDccAddr(slot, base);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_ATTRIB2; cmd_offset <= Pm4::CB_COLOR7_ATTRIB2; cmd_offset++)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t         slot = (cmd_offset - Pm4::CB_COLOR0_ATTRIB2);
			HW::ColorAttrib2 attrib2;
			attrib2.height         = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB2, MIP0_HEIGHT);
			attrib2.width          = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB2, MIP0_WIDTH);
			attrib2.num_mip_levels = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB2, MAX_MIP);
			cp->GetCtx()->SetColorAttrib2(slot, attrib2);
		};
	}

	for (auto cmd_offset = Pm4::CB_COLOR0_ATTRIB3; cmd_offset <= Pm4::CB_COLOR7_ATTRIB3; cmd_offset++)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			uint32_t         slot = (cmd_offset - Pm4::CB_COLOR0_ATTRIB3);
			HW::ColorAttrib3 attrib3;
			attrib3.depth              = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB3, MIP0_DEPTH);
			attrib3.tile_mode          = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB3, COLOR_SW_MODE);
			attrib3.dimension          = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB3, RESOURCE_TYPE);
			attrib3.cmask_pipe_aligned = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB3, CMASK_PIPE_ALIGNED);
			attrib3.dcc_pipe_aligned   = KYTY_PM4_GET(value, CB_COLOR0_ATTRIB3, DCC_PIPE_ALIGNED);
			cp->GetCtx()->SetColorAttrib3(slot, attrib3);
		};
	}

	for (auto cmd_offset = Pm4::PA_CL_VPORT_XSCALE; cmd_offset <= Pm4::PA_CL_VPORT_XSCALE_15; cmd_offset += 6)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{ cp->GetCtx()->SetViewportXScale((cmd_offset - Pm4::PA_CL_VPORT_XSCALE) / 6, *reinterpret_cast<const float*>(&value)); };
	}

	for (auto cmd_offset = Pm4::PA_CL_VPORT_XOFFSET; cmd_offset <= Pm4::PA_CL_VPORT_XOFFSET_15; cmd_offset += 6)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{ cp->GetCtx()->SetViewportXOffset((cmd_offset - Pm4::PA_CL_VPORT_XOFFSET) / 6, *reinterpret_cast<const float*>(&value)); };
	}

	for (auto cmd_offset = Pm4::PA_CL_VPORT_YSCALE; cmd_offset <= Pm4::PA_CL_VPORT_YSCALE_15; cmd_offset += 6)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{ cp->GetCtx()->SetViewportYScale((cmd_offset - Pm4::PA_CL_VPORT_YSCALE) / 6, *reinterpret_cast<const float*>(&value)); };
	}

	for (auto cmd_offset = Pm4::PA_CL_VPORT_YOFFSET; cmd_offset <= Pm4::PA_CL_VPORT_YOFFSET_15; cmd_offset += 6)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{ cp->GetCtx()->SetViewportYOffset((cmd_offset - Pm4::PA_CL_VPORT_YOFFSET) / 6, *reinterpret_cast<const float*>(&value)); };
	}

	for (auto cmd_offset = Pm4::PA_CL_VPORT_ZSCALE; cmd_offset <= Pm4::PA_CL_VPORT_ZSCALE_15; cmd_offset += 6)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{ cp->GetCtx()->SetViewportZScale((cmd_offset - Pm4::PA_CL_VPORT_ZSCALE) / 6, *reinterpret_cast<const float*>(&value)); };
	}

	for (auto cmd_offset = Pm4::PA_CL_VPORT_ZOFFSET; cmd_offset <= Pm4::PA_CL_VPORT_ZOFFSET_15; cmd_offset += 6)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{ cp->GetCtx()->SetViewportZOffset((cmd_offset - Pm4::PA_CL_VPORT_ZOFFSET) / 6, *reinterpret_cast<const float*>(&value)); };
	}

	for (auto cmd_offset = Pm4::PA_SC_VPORT_SCISSOR_0_TL; cmd_offset <= Pm4::PA_SC_VPORT_SCISSOR_15_TL; cmd_offset += 2)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			int  left                  = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(value, PA_SC_VPORT_SCISSOR_0_TL, TL_X)));
			int  top                   = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(value, PA_SC_VPORT_SCISSOR_0_TL, TL_Y)));
			bool window_offset_disable = KYTY_PM4_GET(value, PA_SC_VPORT_SCISSOR_0_TL, WINDOW_OFFSET_DISABLE) != 0;
			cp->GetCtx()->SetViewportScissorTL((cmd_offset - Pm4::PA_SC_VPORT_SCISSOR_0_TL) / 2, left, top, !window_offset_disable);
		};
	}

	for (auto cmd_offset = Pm4::PA_SC_VPORT_SCISSOR_0_BR; cmd_offset <= Pm4::PA_SC_VPORT_SCISSOR_15_BR; cmd_offset += 2)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{
			int right  = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(value, PA_SC_VPORT_SCISSOR_0_BR, BR_X)));
			int bottom = static_cast<int16_t>(static_cast<uint16_t>(KYTY_PM4_GET(value, PA_SC_VPORT_SCISSOR_0_BR, BR_Y)));
			cp->GetCtx()->SetViewportScissorBR((cmd_offset - Pm4::PA_SC_VPORT_SCISSOR_0_BR) / 2, right, bottom);
		};
	}

	for (auto cmd_offset = Pm4::PA_SC_VPORT_ZMIN_0; cmd_offset <= Pm4::PA_SC_VPORT_ZMIN_15; cmd_offset += 2)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{ cp->GetCtx()->SetViewportZMin((cmd_offset - Pm4::PA_SC_VPORT_ZMIN_0) / 2, *reinterpret_cast<const float*>(&value)); };
	}

	for (auto cmd_offset = Pm4::PA_SC_VPORT_ZMAX_0; cmd_offset <= Pm4::PA_SC_VPORT_ZMAX_15; cmd_offset += 2)
	{
		g_hw_ctx_indirect_func[cmd_offset] = [](KYTY_HW_CTX_INDIRECT_ARGS)
		{ cp->GetCtx()->SetViewportZMax((cmd_offset - Pm4::PA_SC_VPORT_ZMAX_0) / 2, *reinterpret_cast<const float*>(&value)); };
	}

	g_hw_ctx_indirect_func[Pm4::SPI_VS_OUT_CONFIG] = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetVsOutConfig(value); };

	g_hw_ctx_indirect_func[Pm4::SPI_SHADER_POS_FORMAT] = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetShaderPosFormat(value); };
	g_hw_ctx_indirect_func[Pm4::SPI_SHADER_IDX_FORMAT] = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetShaderIdxFormat(value); };
	g_hw_ctx_indirect_func[Pm4::PA_CL_VS_OUT_CNTL]     = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetClVsOutCntl(value); };
	g_hw_ctx_indirect_func[Pm4::GE_NGG_SUBGRP_CNTL]    = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetNggSubgrpCntl(value); };
	g_hw_ctx_indirect_func[Pm4::VGT_GS_INSTANCE_CNT]   = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetGsInstanceCnt(value); };
	g_hw_ctx_indirect_func[Pm4::VGT_GS_ONCHIP_CNTL]    = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetGsOnchipCntl(value); };

	g_hw_ctx_indirect_func[Pm4::GE_MAX_OUTPUT_PER_SUBGROUP] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{ cp->GetCtx()->SetMaxOutputPerSubgroup(value); };

	g_hw_ctx_indirect_func[Pm4::VGT_ESGS_RING_ITEMSIZE] = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetEsgsRingItemsize(value); };
	g_hw_ctx_indirect_func[Pm4::VGT_GS_MAX_VERT_OUT]    = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetGsMaxVertOut(value); };
	g_hw_ctx_indirect_func[Pm4::VGT_SHADER_STAGES_EN]   = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetShaderStages(value); };
	g_hw_ctx_indirect_func[Pm4::VGT_GS_OUT_PRIM_TYPE]   = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetGsOutPrimType(value); };
	g_hw_ctx_indirect_func[Pm4::SPI_SHADER_Z_FORMAT]    = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetShaderZFormat(value); };

	g_hw_ctx_indirect_func[Pm4::SPI_SHADER_COL_FORMAT] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		for (uint32_t i = 0; i < 8; i++)
		{
			cp->GetCtx()->SetTargetOutputMode(i, (value >> (i * 4)) & 0xFu);
		}
	};

	g_hw_ctx_indirect_func[Pm4::SPI_PS_INPUT_ENA]  = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetPsInputEna(value); };
	g_hw_ctx_indirect_func[Pm4::SPI_PS_INPUT_ADDR] = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetPsInputAddr(value); };
	g_hw_ctx_indirect_func[Pm4::SPI_PS_IN_CONTROL] = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetPsInControl(value); };
	g_hw_ctx_indirect_func[Pm4::SPI_BARYC_CNTL]    = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetBarycCntl(value); };

	g_hw_ctx_indirect_func[Pm4::DB_SHADER_CONTROL] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		HW::DepthShaderControl db_shader_control {};
		db_shader_control.other_bits                  = value & 0xFFFF9B8Eu;
		db_shader_control.conservative_z_export_value = KYTY_PM4_GET(value, DB_SHADER_CONTROL, CONSERVATIVE_Z_EXPORT);
		db_shader_control.shader_z_behavior           = KYTY_PM4_GET(value, DB_SHADER_CONTROL, Z_ORDER);
		db_shader_control.shader_kill_enable          = KYTY_PM4_GET(value, DB_SHADER_CONTROL, KILL_ENABLE) != 0;
		db_shader_control.shader_z_export_enable      = KYTY_PM4_GET(value, DB_SHADER_CONTROL, Z_EXPORT_ENABLE) != 0;
		db_shader_control.shader_execute_on_noop      = KYTY_PM4_GET(value, DB_SHADER_CONTROL, EXEC_ON_NOOP) != 0;
		cp->GetCtx()->SetDepthShaderControl(db_shader_control);
	};

	g_hw_ctx_indirect_func[Pm4::CB_SHADER_MASK]       = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetShaderMask(value); };
	g_hw_ctx_indirect_func[Pm4::PA_SC_SHADER_CONTROL] = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetScShaderControl(value); };
	g_hw_ctx_indirect_func[Pm4::CB_TARGET_MASK]       = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetRenderTargetMask(value); };

	g_hw_ctx_indirect_func[Pm4::DB_Z_INFO] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		HW::DepthZInfo r;
		r.format                    = KYTY_PM4_GET(value, DB_Z_INFO, FORMAT);
		r.num_samples               = KYTY_PM4_GET(value, DB_Z_INFO, NUM_SAMPLES);
		r.embedded_sample_locations = KYTY_PM4_GET(value, DB_Z_INFO, ITERATE_FLUSH) != 0;
		r.partially_resident        = KYTY_PM4_GET(value, DB_Z_INFO, PARTIALLY_RESIDENT) != 0;
		r.num_mip_levels            = KYTY_PM4_GET(value, DB_Z_INFO, MAXMIP);
		r.tile_mode_index           = KYTY_PM4_GET(value, DB_Z_INFO, TILE_MODE_INDEX);
		r.plane_compression         = KYTY_PM4_GET(value, DB_Z_INFO, DECOMPRESS_ON_N_ZPLANES);
		r.expclear_enabled          = KYTY_PM4_GET(value, DB_Z_INFO, ALLOW_EXPCLEAR) != 0;
		r.tile_surface_enable       = KYTY_PM4_GET(value, DB_Z_INFO, TILE_SURFACE_ENABLE) != 0;
		r.zrange_precision          = KYTY_PM4_GET(value, DB_Z_INFO, ZRANGE_PRECISION);
		cp->GetCtx()->SetDepthZInfo(r);
	};

	g_hw_ctx_indirect_func[Pm4::DB_STENCIL_INFO] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		HW::DepthStencilInfo r;
		r.format                     = KYTY_PM4_GET(value, DB_STENCIL_INFO, FORMAT);
		r.texture_compatible_stencil = KYTY_PM4_GET(value, DB_STENCIL_INFO, ITERATE_FLUSH) != 0;
		r.partially_resident         = KYTY_PM4_GET(value, DB_STENCIL_INFO, PARTIALLY_RESIDENT) != 0;
		r.tile_split                 = KYTY_PM4_GET(value, DB_STENCIL_INFO, RESERVED_FIELD_1);
		r.tile_mode_index            = KYTY_PM4_GET(value, DB_STENCIL_INFO, TILE_MODE_INDEX);
		r.expclear_enabled           = KYTY_PM4_GET(value, DB_STENCIL_INFO, ALLOW_EXPCLEAR) != 0;
		r.tile_stencil_disable       = KYTY_PM4_GET(value, DB_STENCIL_INFO, TILE_STENCIL_DISABLE) != 0;
		cp->GetCtx()->SetDepthStencilInfo(r);
	};

	g_hw_ctx_indirect_func[Pm4::DB_Z_READ_BASE] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().z_read_base_addr;
		base &= 0xFFFFFF00000000FFull;
		base |= static_cast<uint64_t>(value) << 8u;
		cp->GetCtx()->SetDepthZReadBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_Z_READ_BASE_HI] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().z_read_base_addr;
		base &= 0xFFFF00FFFFFFFFFFull;
		base |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
		cp->GetCtx()->SetDepthZReadBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_STENCIL_READ_BASE] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().stencil_read_base_addr;
		base &= 0xFFFFFF00000000FFull;
		base |= static_cast<uint64_t>(value) << 8u;
		cp->GetCtx()->SetDepthStencilReadBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_STENCIL_READ_BASE_HI] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().stencil_read_base_addr;
		base &= 0xFFFF00FFFFFFFFFFull;
		base |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
		cp->GetCtx()->SetDepthStencilReadBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_Z_WRITE_BASE] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().z_write_base_addr;
		base &= 0xFFFFFF00000000FFull;
		base |= static_cast<uint64_t>(value) << 8u;
		cp->GetCtx()->SetDepthZWriteBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_Z_WRITE_BASE_HI] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().z_write_base_addr;
		base &= 0xFFFF00FFFFFFFFFFull;
		base |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
		cp->GetCtx()->SetDepthZWriteBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_STENCIL_WRITE_BASE] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().stencil_write_base_addr;
		base &= 0xFFFFFF00000000FFull;
		base |= static_cast<uint64_t>(value) << 8u;
		cp->GetCtx()->SetDepthStencilWriteBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_STENCIL_WRITE_BASE_HI] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().stencil_write_base_addr;
		base &= 0xFFFF00FFFFFFFFFFull;
		base |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
		cp->GetCtx()->SetDepthStencilWriteBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_HTILE_DATA_BASE] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().htile_data_base_addr;
		base &= 0xFFFFFF00000000FFull;
		base |= static_cast<uint64_t>(value) << 8u;
		cp->GetCtx()->SetDepthHTileDataBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_HTILE_DATA_BASE_HI] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		auto base = cp->GetCtx()->GetDepthRenderTarget().htile_data_base_addr;
		base &= 0xFFFF00FFFFFFFFFFull;
		base |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
		cp->GetCtx()->SetDepthHTileDataBase(base);
	};

	g_hw_ctx_indirect_func[Pm4::DB_DEPTH_VIEW] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		HW::DepthDepthView r;
		r.slice_start = KYTY_PM4_GET(value, DB_DEPTH_VIEW, SLICE_START) + (KYTY_PM4_GET(value, DB_DEPTH_VIEW, SLICE_START_HI) << 11u);
		r.slice_max   = KYTY_PM4_GET(value, DB_DEPTH_VIEW, SLICE_MAX) + (KYTY_PM4_GET(value, DB_DEPTH_VIEW, SLICE_MAX_HI) << 11u);
		r.depth_write_disable   = KYTY_PM4_GET(value, DB_DEPTH_VIEW, Z_READ_ONLY) != 0;
		r.stencil_write_disable = KYTY_PM4_GET(value, DB_DEPTH_VIEW, STENCIL_READ_ONLY) != 0;
		r.current_mip_level     = KYTY_PM4_GET(value, DB_DEPTH_VIEW, MIPID);
		cp->GetCtx()->SetDepthDepthView(r);
	};

	g_hw_ctx_indirect_func[Pm4::DB_DEPTH_SIZE_XY] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		HW::DepthDepthSizeXY r;
		r.x_max = KYTY_PM4_GET(value, DB_DEPTH_SIZE_XY, X_MAX);
		r.y_max = KYTY_PM4_GET(value, DB_DEPTH_SIZE_XY, Y_MAX);
		cp->GetCtx()->SetDepthDepthSizeXY(r);
	};

	g_hw_ctx_indirect_func[Pm4::DB_DEPTH_CLEAR] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{ cp->GetCtx()->SetDepthClearValue(*reinterpret_cast<const float*>(&value)); };
	g_hw_ctx_indirect_func[Pm4::DB_STENCIL_CLEAR] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{ cp->GetCtx()->SetStencilClearValue(KYTY_PM4_GET(value, DB_STENCIL_CLEAR, CLEAR)); };
	g_hw_ctx_indirect_func[Pm4::PA_CL_VTE_CNTL] = [](KYTY_HW_CTX_INDIRECT_ARGS) { cp->GetCtx()->SetViewportTransformControl(value); };

	g_hw_ctx_indirect_func[Pm4::PA_SC_MODE_CNTL_0] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		HW::ScanModeControl r;
		r.msaa_enable          = KYTY_PM4_GET(value, PA_SC_MODE_CNTL_0, MSAA_ENABLE) != 0;
		r.vport_scissor_enable = KYTY_PM4_GET(value, PA_SC_MODE_CNTL_0, VPORT_SCISSOR_ENABLE) != 0;
		r.line_stipple_enable  = KYTY_PM4_GET(value, PA_SC_MODE_CNTL_0, LINE_STIPPLE_ENABLE) != 0;
		cp->GetCtx()->SetScanModeControl(r);
	};

	g_hw_ctx_indirect_func[Pm4::DB_DEPTH_CONTROL] = [](KYTY_HW_CTX_INDIRECT_ARGS)
	{
		HW::DepthControl r;
		r.stencil_enable                     = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, STENCIL_ENABLE) != 0;
		r.z_enable                           = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, Z_ENABLE) != 0;
		r.z_write_enable                     = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, Z_WRITE_ENABLE) != 0;
		r.depth_bounds_enable                = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, DEPTH_BOUNDS_ENABLE) != 0;
		r.zfunc                              = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, ZFUNC);
		r.backface_enable                    = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, BACKFACE_ENABLE) != 0;
		r.stencilfunc                        = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, STENCILFUNC);
		r.stencilfunc_bf                     = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, STENCILFUNC_BF);
		r.color_writes_on_depth_fail_enable  = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, ENABLE_COLOR_WRITES_ON_DEPTH_FAIL) != 0;
		r.color_writes_on_depth_pass_disable = KYTY_PM4_GET(value, DB_DEPTH_CONTROL, DISABLE_COLOR_WRITES_ON_DEPTH_PASS) != 0;
		cp->GetCtx()->SetDepthControl(r);
	};
}

static void graphics_init_jmp_tables_sh_indirect()
{
	for (auto& func: g_hw_sh_indirect_func)
	{
		func = nullptr;
	}

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_LO_ES] = [](KYTY_HW_SH_INDIRECT_ARGS)
	{
		auto base = cp->GetShCtx()->GetVs().es_regs.data_addr;
		base &= 0xFFFFFF00000000FFull;
		base |= static_cast<uint64_t>(value) << 8u;
		cp->GetShCtx()->SetEsShaderBase(base);
	};

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_HI_ES] = [](KYTY_HW_SH_INDIRECT_ARGS)
	{
		auto base = cp->GetShCtx()->GetVs().es_regs.data_addr;
		base &= 0xFFFF00FFFFFFFFFFull;
		base |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
		cp->GetShCtx()->SetEsShaderBase(base);
	};

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_CHKSUM_GS] = [](KYTY_HW_SH_INDIRECT_ARGS) { cp->GetShCtx()->SetGsShaderChksum(value); };

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_RSRC1_GS] = [](KYTY_HW_SH_INDIRECT_ARGS)
	{
		HW::GsShaderResource1 r1;
		r1.vgprs                    = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, VGPRS);
		r1.sgprs                    = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, SGPRS);
		r1.priority                 = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, PRIORITY);
		r1.float_mode               = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, FLOAT_MODE);
		r1.dx10_clamp               = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, DX10_CLAMP) != 0;
		r1.debug_mode               = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, DEBUG_MODE) != 0;
		r1.ieee_mode                = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, IEEE_MODE) != 0;
		r1.cu_group_enable          = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, CU_GROUP_ENABLE) != 0;
		r1.require_forward_progress = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, FWD_PROGRESS) != 0;
		r1.lds_configuration        = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, WGP_MODE) != 0;
		r1.gs_vgpr_component_count  = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, GS_VGPR_COMP_CNT);
		r1.fp16_overflow            = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_GS, FP16_OVFL) != 0;
		cp->GetShCtx()->SetGsShaderResource1(r1);
	};

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_RSRC2_GS] = [](KYTY_HW_SH_INDIRECT_ARGS)
	{
		HW::GsShaderResource2 r2;
		r2.scratch_en = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_GS, SCRATCH_EN) != 0;
		r2.user_sgpr =
		    KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_GS, USER_SGPR) + (KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_GS, USER_SGPR_MSB) << 5u);
		r2.es_vgpr_component_count = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_GS, ES_VGPR_COMP_CNT);
		r2.offchip_lds             = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_GS, OC_LDS_EN) != 0;
		r2.lds_size                = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_GS, LDS_SIZE);
		r2.shared_vgprs            = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_GS, SHARED_VGPR_CNT);
		cp->GetShCtx()->SetGsShaderResource2(r2);
	};

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_LO_PS] = [](KYTY_HW_SH_INDIRECT_ARGS)
	{
		auto base = cp->GetShCtx()->GetPs().ps_regs.data_addr;
		base &= 0xFFFFFF00000000FFull;
		base |= static_cast<uint64_t>(value) << 8u;
		cp->GetShCtx()->SetPsShaderBase(base);
	};

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_HI_PS] = [](KYTY_HW_SH_INDIRECT_ARGS)
	{
		auto base = cp->GetShCtx()->GetPs().ps_regs.data_addr;
		base &= 0xFFFF00FFFFFFFFFFull;
		base |= (static_cast<uint64_t>(value) & 0xffu) << 40u;
		cp->GetShCtx()->SetPsShaderBase(base);
	};

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_CHKSUM_PS] = [](KYTY_HW_SH_INDIRECT_ARGS) { cp->GetShCtx()->SetPsShaderChksum(value); };

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_RSRC1_PS] = [](KYTY_HW_SH_INDIRECT_ARGS)
	{
		HW::PsShaderResource1 r1;
		r1.vgprs                    = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, VGPRS);
		r1.sgprs                    = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, SGPRS);
		r1.priority                 = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, PRIORITY);
		r1.float_mode               = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, FLOAT_MODE);
		r1.dx10_clamp               = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, DX10_CLAMP) != 0;
		r1.debug_mode               = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, DEBUG_MODE) != 0;
		r1.ieee_mode                = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, IEEE_MODE) != 0;
		r1.cu_group_disable         = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, CU_GROUP_DISABLE) != 0;
		r1.require_forward_progress = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, FWD_PROGRESS) != 0;
		r1.fp16_overflow            = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC1_PS, FP16_OVFL) != 0;
		cp->GetShCtx()->SetPsShaderResource1(r1);
	};

	g_hw_sh_indirect_func[Pm4::SPI_SHADER_PGM_RSRC2_PS] = [](KYTY_HW_SH_INDIRECT_ARGS)
	{
		HW::PsShaderResource2 r2;
		r2.scratch_en = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_PS, SCRATCH_EN);
		r2.user_sgpr =
		    KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_PS, USER_SGPR) + (KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_PS, USER_SGPR_MSB) << 5u);
		r2.wave_cnt_en            = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_PS, WAVE_CNT_EN);
		r2.extra_lds_size         = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_PS, EXTRA_LDS_SIZE);
		r2.raster_ordered_shading = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_PS, LOAD_INTRAWAVE_COLLISION);
		r2.shared_vgprs           = KYTY_PM4_GET(value, SPI_SHADER_PGM_RSRC2_PS, SHARED_VGPR_CNT);
		cp->GetShCtx()->SetPsShaderResource2(r2);
	};
}

static void graphics_init_jmp_tables_uc_indirect()
{
	for (auto& func: g_hw_uc_indirect_func)
	{
		func = nullptr;
	}

	g_hw_uc_indirect_func[Pm4::GE_CNTL] = [](KYTY_HW_UC_INDIRECT_ARGS)
	{
		HW::GeControl r;
		r.primitive_group_size = KYTY_PM4_GET(value, GE_CNTL, PRIM_GRP_SIZE);
		r.vertex_group_size    = KYTY_PM4_GET(value, GE_CNTL, VERT_GRP_SIZE);
		cp->GetUcfg()->SetGeControl(r);
	};

	g_hw_uc_indirect_func[Pm4::GE_USER_VGPR_EN] = [](KYTY_HW_UC_INDIRECT_ARGS)
	{
		HW::GeUserVgprEn r;
		r.vgpr1 = KYTY_PM4_GET(value, GE_USER_VGPR_EN, EN_USER_VGPR1) != 0;
		r.vgpr2 = KYTY_PM4_GET(value, GE_USER_VGPR_EN, EN_USER_VGPR2) != 0;
		r.vgpr3 = KYTY_PM4_GET(value, GE_USER_VGPR_EN, EN_USER_VGPR3) != 0;
		cp->GetUcfg()->SetGeUserVgprEn(r);
	};

	g_hw_uc_indirect_func[Pm4::VGT_PRIMITIVE_TYPE] = [](KYTY_HW_UC_INDIRECT_ARGS)
	{
		uint32_t prim_type = KYTY_PM4_GET(value, VGT_PRIMITIVE_TYPE, PRIM_TYPE);
		cp->GetUcfg()->SetPrimitiveType(prim_type);
	};
}

static void graphics_init_jmp_tables()
{
	for (auto& func: g_hw_ctx_func)
	{
		func = nullptr;
	}

	g_hw_ctx_func[Pm4::DB_RENDER_CONTROL]                 = hw_ctx_set_render_control;
	g_hw_ctx_func[Pm4::DB_STENCIL_CLEAR]                  = hw_ctx_set_stencil_clear;
	g_hw_ctx_func[Pm4::DB_DEPTH_CLEAR]                    = hw_ctx_set_depth_clear;
	g_hw_ctx_func[Pm4::PA_SC_SCREEN_SCISSOR_TL]           = hw_ctx_set_screen_scissor;
	g_hw_ctx_func[Pm4::DB_Z_INFO]                         = hw_ctx_set_depth_render_target;
	g_hw_ctx_func[Pm4::DB_STENCIL_INFO]                   = hw_ctx_set_stencil_info;
	g_hw_ctx_func[Pm4::PA_SU_HARDWARE_SCREEN_OFFSET]      = hw_ctx_set_hardware_screen_offset;
	g_hw_ctx_func[Pm4::CB_TARGET_MASK]                    = hw_ctx_set_render_target_mask;
	g_hw_ctx_func[Pm4::PA_SC_GENERIC_SCISSOR_TL]          = hw_ctx_set_generic_scissor;
	g_hw_ctx_func[Pm4::CB_BLEND_RED]                      = hw_ctx_set_blend_color;
	g_hw_ctx_func[Pm4::DB_STENCIL_CONTROL]                = hw_ctx_set_stencil_control;
	g_hw_ctx_func[Pm4::DB_STENCILREFMASK]                 = hw_ctx_set_stencil_mask;
	g_hw_ctx_func[Pm4::SPI_PS_INPUT_CNTL_0]               = hw_ctx_set_ps_input;
	g_hw_ctx_func[Pm4::DB_DEPTH_CONTROL]                  = hw_ctx_set_depth_control;
	g_hw_ctx_func[Pm4::DB_EQAA]                           = hw_ctx_set_eqaa_control;
	g_hw_ctx_func[Pm4::CB_COLOR_CONTROL]                  = hw_ctx_set_color_control;
	g_hw_ctx_func[Pm4::PA_CL_CLIP_CNTL]                   = hw_ctx_set_clip_control;
	g_hw_ctx_func[Pm4::PA_SU_SC_MODE_CNTL]                = hw_ctx_set_mode_control;
	g_hw_ctx_func[Pm4::PA_CL_VTE_CNTL]                    = hw_ctx_set_viewport_transform_control;
	g_hw_ctx_func[Pm4::PA_SU_LINE_CNTL]                   = hw_ctx_set_line_control;
	g_hw_ctx_func[Pm4::PA_SC_MODE_CNTL_0]                 = hw_ctx_set_scan_mode_control;
	g_hw_ctx_func[Pm4::PA_SC_AA_CONFIG]                   = hw_ctx_set_aa_config;
	g_hw_ctx_func[Pm4::PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0] = hw_ctx_set_aa_sample_control;
	g_hw_ctx_func[Pm4::VGT_SHADER_STAGES_EN]              = hw_ctx_set_shader_stages;
	g_hw_ctx_func[Pm4::PA_CL_GB_VERT_CLIP_ADJ]            = hw_ctx_set_guard_bands;

	for (uint32_t slot = 0; slot < 8; slot++)
	{
		g_hw_ctx_func[Pm4::CB_COLOR0_BASE + slot * 15] = hw_ctx_set_render_target;
		g_hw_ctx_func[Pm4::CB_COLOR0_INFO + slot * 15] = hw_ctx_set_color_info;

		g_hw_ctx_func[Pm4::CB_BLEND0_CONTROL + slot * 1] = hw_ctx_set_blend_control;
	}

	for (uint32_t viewport = 0; viewport < 16; viewport++)
	{
		g_hw_ctx_func[Pm4::PA_SC_VPORT_ZMIN_0 + viewport * 2] = hw_ctx_set_viewport_z;
		g_hw_ctx_func[Pm4::PA_CL_VPORT_XSCALE + viewport * 6] = hw_ctx_set_viewport_scale_offset;
	}

	for (auto& func: g_hw_sh_func)
	{
		func = nullptr;
	}

	for (uint32_t slot = 0; slot < 16; slot++)
	{
		g_hw_sh_func[Pm4::SPI_SHADER_USER_DATA_VS_0 + slot * 1] = hw_sh_set_vs_user_sgpr;
		g_hw_sh_func[Pm4::SPI_SHADER_USER_DATA_PS_0 + slot * 1] = hw_sh_set_ps_user_sgpr;
		g_hw_sh_func[Pm4::COMPUTE_USER_DATA_0 + slot * 1]       = hw_sh_set_cs_user_sgpr;
		g_hw_sh_func[Pm4::SPI_SHADER_USER_DATA_GS_0 + slot * 1] = hw_sh_set_gs_user_sgpr;
	}

	for (auto& func: g_hw_uc_func)
	{
		func = nullptr;
	}

	g_hw_uc_func[Pm4::VGT_PRIMITIVE_TYPE] = hw_uc_set_primitive_type;

	for (auto& func: g_hw_sh_custom_func)
	{
		func = nullptr;
	}

	g_hw_sh_custom_func[Pm4::R_VS]          = hw_sh_set_vs_shader;
	g_hw_sh_custom_func[Pm4::R_PS]          = hw_sh_set_ps_shader;
	g_hw_sh_custom_func[Pm4::R_CS]          = hw_sh_set_cs_shader;
	g_hw_sh_custom_func[Pm4::R_VS_EMBEDDED] = hw_sh_set_vs_embedded;
	g_hw_sh_custom_func[Pm4::R_PS_EMBEDDED] = hw_sh_set_ps_embedded;
	g_hw_sh_custom_func[Pm4::R_VS_UPDATE]   = hw_sh_update_vs_shader;
	g_hw_sh_custom_func[Pm4::R_PS_UPDATE]   = hw_sh_update_ps_shader;

	for (auto& func: g_cp_op_func)
	{
		func = nullptr;
	}

	g_cp_op_func[Pm4::IT_NOP]                     = cp_op_nop;
	g_cp_op_func[Pm4::IT_DRAW_INDEX_2]            = cp_op_draw_index;
	g_cp_op_func[Pm4::IT_INDEX_TYPE]              = cp_op_index_type;
	g_cp_op_func[Pm4::IT_NUM_INSTANCES]           = cp_op_num_instances;
	g_cp_op_func[Pm4::IT_DRAW_INDEX_AUTO]         = cp_op_draw_index_auto;
	g_cp_op_func[Pm4::IT_WAIT_REG_MEM]            = cp_op_wait_reg_mem;
	g_cp_op_func[Pm4::IT_WRITE_DATA]              = cp_op_write_data;
	g_cp_op_func[Pm4::IT_INDIRECT_BUFFER]         = cp_op_indirect_buffer;
	g_cp_op_func[Pm4::IT_EVENT_WRITE]             = cp_op_event_write;
	g_cp_op_func[Pm4::IT_EVENT_WRITE_EOP]         = cp_op_event_write_eop;
	g_cp_op_func[Pm4::IT_EVENT_WRITE_EOS]         = cp_op_event_write_eos;
	g_cp_op_func[Pm4::IT_RELEASE_MEM]             = cp_op_release_mem;
	g_cp_op_func[Pm4::IT_DMA_DATA]                = cp_op_dma_data;
	g_cp_op_func[Pm4::IT_ACQUIRE_MEM]             = cp_op_acquire_mem;
	g_cp_op_func[Pm4::IT_SET_CONTEXT_REG]         = cp_op_set_context_reg;
	g_cp_op_func[Pm4::IT_SET_SH_REG]              = cp_op_set_shader_reg;
	g_cp_op_func[Pm4::IT_SET_UCONFIG_REG]         = cp_op_set_uconfig_reg;
	g_cp_op_func[Pm4::IT_WRITE_CONST_RAM]         = cp_op_write_const_ram;
	g_cp_op_func[Pm4::IT_DUMP_CONST_RAM]          = cp_op_dump_const_ram;
	g_cp_op_func[Pm4::IT_INCREMENT_CE_COUNTER]    = cp_op_increment_ce_counter;
	g_cp_op_func[Pm4::IT_INCREMENT_DE_COUNTER]    = cp_op_increment_de_counter;
	g_cp_op_func[Pm4::IT_WAIT_ON_CE_COUNTER]      = cp_op_wait_on_ce_counter;
	g_cp_op_func[Pm4::IT_WAIT_ON_DE_COUNTER_DIFF] = cp_op_wait_on_de_counter_diff;

	for (auto& func: g_cp_op_custom_func)
	{
		func = nullptr;
	}

	g_cp_op_custom_func[Pm4::R_DRAW_INDEX]       = cp_op_draw_index;
	g_cp_op_custom_func[Pm4::R_DRAW_INDEX_AUTO]  = cp_op_draw_index_auto;
	g_cp_op_custom_func[Pm4::R_DISPATCH_DIRECT]  = cp_op_dispatch_direct;
	g_cp_op_custom_func[Pm4::R_DISPATCH_RESET]   = cp_op_dispatch_reset;
	g_cp_op_custom_func[Pm4::R_WAIT_MEM_32]      = cp_op_wait_reg_mem_32;
	g_cp_op_custom_func[Pm4::R_DRAW_RESET]       = cp_op_draw_reset;
	g_cp_op_custom_func[Pm4::R_WAIT_FLIP_DONE]   = cp_op_wait_flip_done;
	g_cp_op_custom_func[Pm4::R_PUSH_MARKER]      = cp_op_push_marker;
	g_cp_op_custom_func[Pm4::R_POP_MARKER]       = cp_op_pop_marker;
	g_cp_op_custom_func[Pm4::R_CX_REGS_INDIRECT] = cp_op_indirect_cx_regs;
	g_cp_op_custom_func[Pm4::R_SH_REGS_INDIRECT] = cp_op_indirect_sh_regs;
	g_cp_op_custom_func[Pm4::R_UC_REGS_INDIRECT] = cp_op_indirect_uc_regs;
	g_cp_op_custom_func[Pm4::R_ACQUIRE_MEM]      = cp_op_acquire_mem;
	g_cp_op_custom_func[Pm4::R_WRITE_DATA]       = cp_op_write_data;
	g_cp_op_custom_func[Pm4::R_WAIT_MEM_64]      = cp_op_wait_reg_mem_64;
	g_cp_op_custom_func[Pm4::R_FLIP]             = cp_op_flip;
	g_cp_op_custom_func[Pm4::R_RELEASE_MEM]      = cp_op_release_mem;

	graphics_init_jmp_tables_cx_indirect();
	graphics_init_jmp_tables_sh_indirect();
	graphics_init_jmp_tables_uc_indirect();
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
