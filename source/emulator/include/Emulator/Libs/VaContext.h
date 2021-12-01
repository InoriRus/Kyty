#ifndef EMULATOR_INCLUDE_EMULATOR_LIBS_VACONTEXT_H_
#define EMULATOR_INCLUDE_EMULATOR_LIBS_VACONTEXT_H_

#include <cstddef>
#include <xmmintrin.h>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VA_ARGS                                                                                                                            \
	uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9, uint64_t overflow_arg_area, __m128 xmm0,             \
	    __m128 xmm1, __m128 xmm2, __m128 xmm3, __m128 xmm4, __m128 xmm5, __m128 xmm6, __m128 xmm7, ...

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VA_CONTEXT(ctx)                                                                                                                    \
	alignas(16) VaContext ctx;                                                                                                             \
	(ctx).reg_save_area.gp[0]       = rdi;                                                                                                 \
	(ctx).reg_save_area.gp[1]       = rsi;                                                                                                 \
	(ctx).reg_save_area.gp[2]       = rdx;                                                                                                 \
	(ctx).reg_save_area.gp[3]       = rcx;                                                                                                 \
	(ctx).reg_save_area.gp[4]       = r8;                                                                                                  \
	(ctx).reg_save_area.gp[5]       = r9;                                                                                                  \
	(ctx).reg_save_area.fp[0]       = xmm0;                                                                                                \
	(ctx).reg_save_area.fp[1]       = xmm1;                                                                                                \
	(ctx).reg_save_area.fp[2]       = xmm2;                                                                                                \
	(ctx).reg_save_area.fp[3]       = xmm3;                                                                                                \
	(ctx).reg_save_area.fp[4]       = xmm4;                                                                                                \
	(ctx).reg_save_area.fp[5]       = xmm5;                                                                                                \
	(ctx).reg_save_area.fp[6]       = xmm6;                                                                                                \
	(ctx).reg_save_area.fp[7]       = xmm7;                                                                                                \
	(ctx).va_list.reg_save_area     = &(ctx).reg_save_area;                                                                                \
	(ctx).va_list.gp_offset         = offsetof(VaRegSave, gp);                                                                             \
	(ctx).va_list.fp_offset         = offsetof(VaRegSave, fp);                                                                             \
	(ctx).va_list.overflow_arg_area = &overflow_arg_area;

namespace Kyty::Libs {

#pragma pack(1)

struct VaList
{
	uint32_t gp_offset;
	uint32_t fp_offset;
	void*    overflow_arg_area;
	void*    reg_save_area;
};

// typedef float __m128 __attribute__((__vector_size__(16), __aligned__(16)));

struct VaRegSave
{
	uint64_t gp[6];
	__m128   fp[8];
};

struct VaContext
{
	VaRegSave reg_save_area;
	VaList    va_list;
};

struct VaCharX16
{
	char x[16];
};

struct VaShortX8
{
	short x[8]; // NOLINT(google-runtime-int)
};

struct VaIntX4
{
	int x[4];
};

struct VaFloatX4
{
	float x[4];
};

#pragma pack()

template <class T, uint64_t Align, uint64_t Size>
T VaArg_overflow_arg_area(VaList* l)
{
	auto  ptr            = ((reinterpret_cast<uint64_t>(l->overflow_arg_area) + (Align - 1)) & ~(Align - 1));
	auto* addr           = reinterpret_cast<T*>(ptr);
	l->overflow_arg_area = reinterpret_cast<void*>(ptr + Size);
	return *addr;
}

template <class T, uint32_t Size>
T VaArg_reg_save_area_gp(VaList* l)
{
	auto* addr = reinterpret_cast<T*>(static_cast<uint8_t*>(l->reg_save_area) + l->gp_offset);
	l->gp_offset += Size;
	return *addr;
}

template <class T, uint32_t Size>
T VaArg_reg_save_area_fp(VaList* l)
{
	auto* addr = reinterpret_cast<T*>(static_cast<uint8_t*>(l->reg_save_area) + l->fp_offset);
	l->fp_offset += Size;
	return *addr;
}

template <>
inline VaFloatX4 VaArg_reg_save_area_fp<VaFloatX4, 32>(VaList* l)
{
	auto* addr = reinterpret_cast<VaFloatX4*>(static_cast<uint8_t*>(l->reg_save_area) + l->fp_offset);
	l->fp_offset += 32;
	VaFloatX4 ret = {{addr[0].x[0], addr[0].x[1], addr[1].x[0], addr[1].x[1]}};
	return ret;
}

inline int VaArg_int(VaList* l)
{
	if (l->gp_offset <= 40)
	{
		return VaArg_reg_save_area_gp<int, 8>(l);
	}
	return VaArg_overflow_arg_area<int, 1, 8>(l);
}

inline double VaArg_double(VaList* l)
{
	if (l->fp_offset <= 160)
	{
		return VaArg_reg_save_area_fp<double, 16>(l);
	}
	return VaArg_overflow_arg_area<double, 1, 8>(l);
}

inline long double VaArg_long_double(VaList* l)
{
	return VaArg_overflow_arg_area<long double, 16, 16>(l);
}

inline wint_t VaArg_wint_t(VaList* l)
{
	if (l->gp_offset <= 40)
	{
		return VaArg_reg_save_area_gp<wint_t, 8>(l);
	}
	return VaArg_overflow_arg_area<wint_t, 1, 8>(l);
}

inline VaCharX16 VaArg_char_x16(VaList* l)
{
	if (l->gp_offset <= 32)
	{
		return VaArg_reg_save_area_gp<VaCharX16, 16>(l);
	}
	return VaArg_overflow_arg_area<VaCharX16, 1, 16>(l);
}

inline long VaArg_long(VaList* l) // NOLINT(google-runtime-int)
{
	if (l->gp_offset <= 40)
	{
		return VaArg_reg_save_area_gp<long, 8>(l); // NOLINT(google-runtime-int)
	}
	return VaArg_overflow_arg_area<long, 1, 8>(l); // NOLINT(google-runtime-int)
}

inline intmax_t VaArg_intmax_t(VaList* l)
{
	if (l->gp_offset <= 40)
	{
		return VaArg_reg_save_area_gp<intmax_t, 8>(l);
	}
	return VaArg_overflow_arg_area<intmax_t, 1, 8>(l);
}

inline long long VaArg_long_long(VaList* l) // NOLINT(google-runtime-int)
{
	if (l->gp_offset <= 40)
	{
		return VaArg_reg_save_area_gp<long long, 8>(l); // NOLINT(google-runtime-int)
	}
	return VaArg_overflow_arg_area<long long, 1, 8>(l); // NOLINT(google-runtime-int)
}

inline ptrdiff_t VaArg_ptrdiff_t(VaList* l)
{
	if (l->gp_offset <= 40)
	{
		return VaArg_reg_save_area_gp<ptrdiff_t, 8>(l);
	}
	return VaArg_overflow_arg_area<ptrdiff_t, 1, 8>(l);
}

inline size_t VaArg_size_t(VaList* l)
{
	if (l->gp_offset <= 40)
	{
		return VaArg_reg_save_area_gp<size_t, 8>(l);
	}
	return VaArg_overflow_arg_area<size_t, 1, 8>(l);
}

inline VaShortX8 VaArg_ShortX8(VaList* l)
{
	if (l->gp_offset <= 32)
	{
		return VaArg_reg_save_area_gp<VaShortX8, 16>(l);
	}
	return VaArg_overflow_arg_area<VaShortX8, 1, 16>(l);
}

inline VaIntX4 VaArg_IntX4(VaList* l)
{
	if (l->gp_offset <= 32)
	{
		return VaArg_reg_save_area_gp<VaIntX4, 16>(l);
	}
	return VaArg_overflow_arg_area<VaIntX4, 1, 16>(l);
}

inline VaFloatX4 VaArg_FloatX4(VaList* l)
{
	if (l->fp_offset <= 144)
	{
		return VaArg_reg_save_area_fp<VaFloatX4, 32>(l);
	}
	return VaArg_overflow_arg_area<VaFloatX4, 1, 16>(l);
}

template <class T>
T* VaArg_ptr(VaList* l)
{
	if (l->gp_offset <= 40)
	{
		return VaArg_reg_save_area_gp<T*, 8>(l);
	}
	return VaArg_overflow_arg_area<T*, 1, 8>(l);
}

} // namespace Kyty::Libs

#endif /* EMULATOR_INCLUDE_EMULATOR_LIBS_VACONTEXT_H_ */
