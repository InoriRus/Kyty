#ifndef EMULATOR_INCLUDE_EMULATOR_LOADER_JIT_H_
#define EMULATOR_INCLUDE_EMULATOR_LOADER_JIT_H_

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Loader::Jit {

#pragma pack(1)

struct JmpWithIndex
{
	void SetIndex(uint32_t index) { *reinterpret_cast<uint32_t*>(&code[1]) = index; }

	void SetFunc(void* handler)
	{
		auto func_addr = reinterpret_cast<int64_t>(handler);
		auto rip_addr  = reinterpret_cast<int64_t>(&code[10]);
		auto offset64  = func_addr - rip_addr;
		auto offset32  = static_cast<uint32_t>(static_cast<uint64_t>(offset64) & 0xffffffffu);

		*reinterpret_cast<uint32_t*>(&code[6]) = offset32;
	}

	static uint64_t GetSize() { return 16; }

	// 68 00 00 00 00          push     <index>
	// E9 E0 FF FF FF          jmp      <handler>
	uint8_t code[16] = {0x68, 0x00, 0x00, 0x00, 0x00, 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
};

struct CallPlt
{
	explicit CallPlt(uint32_t table_size)
	{
		for (uint32_t index = 0; index < table_size; index++)
		{
			auto* c = new (&code[32] + JmpWithIndex::GetSize() * index) JmpWithIndex;
			c->SetIndex(index);
			c->SetFunc(this);
		}
	}

	void SetPltGot(uint64_t vaddr) { *reinterpret_cast<uint64_t*>(&code[2]) = vaddr; }

	uint64_t GetAddr(uint32_t index) { return reinterpret_cast<uint64_t>(&code[32] + JmpWithIndex::GetSize() * index); }

	static uint64_t GetSize(uint32_t table_size) { return 32 + JmpWithIndex::GetSize() * table_size; }

	// 0:  49 bb 88 77 66 55 44    movabs r11,0x1122334455667788
	// 7:  33 22 11
	// a:  41 ff 73 08             push   QWORD PTR [r11+0x8]
	// e:  41 ff 63 10             jmp    QWORD PTR [r11+0x10]
	uint8_t code[32] = {0x49, 0xBB, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x41, 0xFF,
	                    0x73, 0x08, 0x41, 0xFF, 0x63, 0x10, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
};

struct JmpRax
{
	template <class Handler>
	void SetFunc(Handler func)
	{
		*reinterpret_cast<Handler*>(&code[2]) = func;
	}

	// mov rax, 0x1122334455667788
	// jmp rax
	uint8_t code[16] = {0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0xFF, 0xE0};
};

struct Call9
{
	template <class Handler>
	void SetFunc(Handler func)
	{
		auto func_addr = reinterpret_cast<int64_t>(reinterpret_cast<void*>(func));
		auto rip_addr  = reinterpret_cast<int64_t>(&code[5]);
		auto offset64  = func_addr - rip_addr;
		auto offset32  = static_cast<uint32_t>(static_cast<uint64_t>(offset64) & 0xffffffffu);

		*reinterpret_cast<uint32_t*>(&code[1]) = offset32;
	}

	static uint64_t GetSize() { return 9; }

	// call func
	// mov rax,rax
	// nop
	uint8_t code[9] = {0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0xC0, 0x90};
};

struct SafeCall
{
	using func_t = KYTY_MS_ABI uint8_t* (*)();

	void SetFunc(func_t func) { *reinterpret_cast<func_t*>(&code[0x22]) = func; }
	void SetRegSaveArea(uint8_t* area) { *reinterpret_cast<uint8_t**>(&code[0x18]) = area; }
	void SetLockVar(uint8_t* lock_var) { *reinterpret_cast<uint8_t**>(&code[0x0e]) = lock_var; }

	static uint64_t GetSize() { return 0x1000; }

	uint8_t code[0x6c] = {
	    /*00*/ 0x51,                                                       // push   rcx  /* Save general purpose registers */
	    /*01*/ 0x52,                                                       // push   rdx
	    /*02*/ 0x41, 0x50,                                                 // push   r8
	    /*04*/ 0x41, 0x51,                                                 // push   r9
	    /*06*/ 0x41, 0x52,                                                 // push   r10
	    /*08*/ 0x41, 0x53,                                                 // push   r11
	    /*0a*/ 0x57,                                                       // push   rdi
	    /*0b*/ 0x56,                                                       // push   rsi
	    /*0c*/ 0x48, 0xbf, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // movabs rdi,0x1122334455667788
	    /*16*/ 0x48, 0xbe, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // movabs rsi,0x1122334455667788
	    /*20*/ 0x48, 0xb9, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // movabs rcx,0x1122334455667788
	    /*2a*/ 0xb0, 0x01,                                                 // mov    al,0x1                  /* Lock */
	    /*2c*/ 0x86, 0x07,                                                 // xchg   BYTE PTR [rdi],al
	    /*2e*/ 0x84, 0xc0,                                                 // test   al,al
	    /*30*/ 0x75, 0xf8,                                                 // jne    2a <LOCK>
	    /*32*/ 0xb8, 0xff, 0xff, 0xff, 0xff,                               // mov    eax,0xffffffff
	    /*37*/ 0xba, 0xff, 0xff, 0xff, 0xff,                               // mov    edx,0xffffffff
	    /*3c*/ 0x0f, 0xae, 0x26,                                           // xsave  [rsi]      /* Save float registers */
	    /*3f*/ 0x48, 0x83, 0xec, 0x08,                                     // sub    rsp,0x8    /* Align stack */
	    /*43*/ 0xff, 0xd1,                                                 // call   rcx
	    /*45*/ 0x48, 0x83, 0xc4, 0x08,                                     // add    rsp,0x8
	    /*49*/ 0x48, 0x89, 0xc1,                                           // mov    rcx,rax
	    /*4c*/ 0xb8, 0xff, 0xff, 0xff, 0xff,                               // mov    eax,0xffffffff
	    /*51*/ 0xba, 0xff, 0xff, 0xff, 0xff,                               // mov    edx,0xffffffff
	    /*56*/ 0x0f, 0xae, 0x2e,                                           // xrstor [rsi]      /* Restore float registers */
	    /*59*/ 0x48, 0x89, 0xc8,                                           // mov    rax,rcx
	    /*5c*/ 0xc6, 0x07, 0x00,                                           // mov    BYTE PTR [rdi],0x0       /* Unlock */
	    /*5f*/ 0x5e,                                                       // pop    rsi      /* Restore general purpose registers */
	    /*60*/ 0x5f,                                                       // pop    rdi
	    /*61*/ 0x41, 0x5b,                                                 // pop    r11
	    /*63*/ 0x41, 0x5a,                                                 // pop    r10
	    /*65*/ 0x41, 0x59,                                                 // pop    r9
	    /*67*/ 0x41, 0x58,                                                 // pop    r8
	    /*69*/ 0x5a,                                                       // pop    rdx
	    /*6a*/ 0x59,                                                       // pop    rcx
	    /*6b*/ 0xc3,                                                       // ret

	};
};

#pragma pack()

} // namespace Kyty::Loader::Jit

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LOADER_JIT_H_ */
