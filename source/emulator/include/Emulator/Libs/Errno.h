#ifndef EMULATOR_INCLUDE_EMULATOR_LIBS_ERRNO_H_
#define EMULATOR_INCLUDE_EMULATOR_LIBS_ERRNO_H_

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

constexpr int OK = 0;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define POSIX_CALL(func)                                                                                                                   \
	[&]()                                                                                                                                  \
	{                                                                                                                                      \
		auto result = func;                                                                                                                \
		if (result != OK)                                                                                                                  \
		{                                                                                                                                  \
			*Posix::GetErrorAddr() = LibKernel::KernelToPosix(result);                                                                     \
			return -1;                                                                                                                     \
		}                                                                                                                                  \
		return 0;                                                                                                                          \
	}()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define POSIX_N_CALL(func)                                                                                                                 \
	[&]()                                                                                                                                  \
	{                                                                                                                                      \
		auto result = func;                                                                                                                \
		if (result < 0)                                                                                                                    \
		{                                                                                                                                  \
			*Posix::GetErrorAddr() = LibKernel::KernelToPosix(static_cast<int>(result));                                                   \
			return static_cast<decltype(result)>(-1);                                                                                      \
		}                                                                                                                                  \
		return result;                                                                                                                     \
	}()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define POSIX_PTHREAD_CALL(func)                                                                                                           \
	[&]()                                                                                                                                  \
	{                                                                                                                                      \
		auto result = func;                                                                                                                \
		if (result != OK)                                                                                                                  \
		{                                                                                                                                  \
			return LibKernel::KernelToPosix(result);                                                                                       \
		}                                                                                                                                  \
		return 0;                                                                                                                          \
	}()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define POSIX_NET_CALL(func)                                                                                                               \
	[&]()                                                                                                                                  \
	{                                                                                                                                      \
		auto result = func;                                                                                                                \
		if (result != OK)                                                                                                                  \
		{                                                                                                                                  \
			*Posix::GetErrorAddr() = Network::NetToPosix(result);                                                                          \
			return -1;                                                                                                                     \
		}                                                                                                                                  \
		return 0;                                                                                                                          \
	}()

namespace Kyty::Libs {

namespace Posix {

KYTY_SYSV_ABI int* GetErrorAddr();

constexpr int POSIX_EPERM             = 1;
constexpr int POSIX_ENOENT            = 2;
constexpr int POSIX_ESRCH             = 3;
constexpr int POSIX_EINTR             = 4;
constexpr int POSIX_EIO               = 5;
constexpr int POSIX_ENXIO             = 6;
constexpr int POSIX_E2BIG             = 7;
constexpr int POSIX_ENOEXEC           = 8;
constexpr int POSIX_EBADF             = 9;
constexpr int POSIX_ECHILD            = 10;
constexpr int POSIX_EDEADLK           = 11;
constexpr int POSIX_ENOMEM            = 12;
constexpr int POSIX_EACCES            = 13;
constexpr int POSIX_EFAULT            = 14;
constexpr int POSIX_ENOTBLK           = 15;
constexpr int POSIX_EBUSY             = 16;
constexpr int POSIX_EEXIST            = 17;
constexpr int POSIX_EXDEV             = 18;
constexpr int POSIX_ENODEV            = 19;
constexpr int POSIX_ENOTDIR           = 20;
constexpr int POSIX_EISDIR            = 21;
constexpr int POSIX_EINVAL            = 22;
constexpr int POSIX_ENFILE            = 23;
constexpr int POSIX_EMFILE            = 24;
constexpr int POSIX_ENOTTY            = 25;
constexpr int POSIX_ETXTBSY           = 26;
constexpr int POSIX_EFBIG             = 27;
constexpr int POSIX_ENOSPC            = 28;
constexpr int POSIX_ESPIPE            = 29;
constexpr int POSIX_EROFS             = 30;
constexpr int POSIX_EMLINK            = 31;
constexpr int POSIX_EPIPE             = 32;
constexpr int POSIX_EDOM              = 33;
constexpr int POSIX_ERANGE            = 34;
constexpr int POSIX_EAGAIN            = 35;
constexpr int POSIX_EWOULDBLOCK       = 35;
constexpr int POSIX_EINPROGRESS       = 36;
constexpr int POSIX_EALREADY          = 37;
constexpr int POSIX_ENOTSOCK          = 38;
constexpr int POSIX_EDESTADDRREQ      = 39;
constexpr int POSIX_EMSGSIZE          = 40;
constexpr int POSIX_EPROTOTYPE        = 41;
constexpr int POSIX_ENOPROTOOPT       = 42;
constexpr int POSIX_EPROTONOSUPPORT   = 43;
constexpr int POSIX_ESOCKTNOSUPPORT   = 44;
constexpr int POSIX_EOPNOTSUPP        = 45;
constexpr int POSIX_ENOTSUP           = 45;
constexpr int POSIX_EPFNOSUPPORT      = 46;
constexpr int POSIX_EAFNOSUPPORT      = 47;
constexpr int POSIX_EADDRINUSE        = 48;
constexpr int POSIX_EADDRNOTAVAIL     = 49;
constexpr int POSIX_ENETDOWN          = 50;
constexpr int POSIX_ENETUNREACH       = 51;
constexpr int POSIX_ENETRESET         = 52;
constexpr int POSIX_ECONNABORTED      = 53;
constexpr int POSIX_ECONNRESET        = 54;
constexpr int POSIX_ENOBUFS           = 55;
constexpr int POSIX_EISCONN           = 56;
constexpr int POSIX_ENOTCONN          = 57;
constexpr int POSIX_ESHUTDOWN         = 58;
constexpr int POSIX_ETOOMANYREFS      = 59;
constexpr int POSIX_ETIMEDOUT         = 60;
constexpr int POSIX_ECONNREFUSED      = 61;
constexpr int POSIX_ELOOP             = 62;
constexpr int POSIX_ENAMETOOLONG      = 63;
constexpr int POSIX_EHOSTDOWN         = 64;
constexpr int POSIX_EHOSTUNREACH      = 65;
constexpr int POSIX_ENOTEMPTY         = 66;
constexpr int POSIX_EPROCLIM          = 67;
constexpr int POSIX_EUSERS            = 68;
constexpr int POSIX_EDQUOT            = 69;
constexpr int POSIX_ESTALE            = 70;
constexpr int POSIX_EREMOTE           = 71;
constexpr int POSIX_EBADRPC           = 72;
constexpr int POSIX_ERPCMISMATCH      = 73;
constexpr int POSIX_EPROGUNAVAIL      = 74;
constexpr int POSIX_EPROGMISMATCH     = 75;
constexpr int POSIX_EPROCUNAVAIL      = 76;
constexpr int POSIX_ENOLCK            = 77;
constexpr int POSIX_ENOSYS            = 78;
constexpr int POSIX_EFTYPE            = 79;
constexpr int POSIX_EAUTH             = 80;
constexpr int POSIX_ENEEDAUTH         = 81;
constexpr int POSIX_EIDRM             = 82;
constexpr int POSIX_ENOMSG            = 83;
constexpr int POSIX_EOVERFLOW         = 84;
constexpr int POSIX_ECANCELED         = 85;
constexpr int POSIX_EILSEQ            = 86;
constexpr int POSIX_ENOATTR           = 87;
constexpr int POSIX_EDOOFUS           = 88;
constexpr int POSIX_EBADMSG           = 89;
constexpr int POSIX_EMULTIHOP         = 90;
constexpr int POSIX_ENOLINK           = 91;
constexpr int POSIX_EPROTO            = 92;
constexpr int POSIX_ENOTCAPABLE       = 93;
constexpr int POSIX_ECAPMODE          = 94;
constexpr int POSIX_ENOBLK            = 95;
constexpr int POSIX_EICV              = 96;
constexpr int POSIX_ENOPLAYGOENT      = 97;
constexpr int POSIX_EREVOKE           = 98;
constexpr int POSIX_ESDKVERSION       = 99;
constexpr int POSIX_ESTART            = 100;
constexpr int POSIX_ESTOP             = 101;
constexpr int POSIX_EINVALID2MB       = 102;
constexpr int POSIX_ELAST             = 102;
constexpr int POSIX_EADHOC            = 160;
constexpr int POSIX_EINACTIVEDISABLED = 163;
constexpr int POSIX_ENETNODATA        = 164;
constexpr int POSIX_ENETDESC          = 165;
constexpr int POSIX_ENETDESCTIMEDOUT  = 166;
constexpr int POSIX_ENETINTR          = 167;
constexpr int POSIX_ERETURN           = 205;
constexpr int POSIX_EFPOS             = 152;
constexpr int POSIX_ENODATA           = 1040;
constexpr int POSIX_ENOSR             = 1050;
constexpr int POSIX_ENOSTR            = 1051;
constexpr int POSIX_ENOTRECOVERABLE   = 1056;
constexpr int POSIX_EOTHER            = 1062;
constexpr int POSIX_EOWNERDEAD        = 1064;
constexpr int POSIX_ETIME             = 1074;

} // namespace Posix

namespace LibKernel {

inline int KernelToPosix(int kernel_errno)
{
	return kernel_errno > -2147352576 && kernel_errno <= -2147352475 ? kernel_errno + 2147352576 : 1062;
}

constexpr int KERNEL_ERROR_UNKNOWN         = -2147352576; /* 0x80020000 */
constexpr int KERNEL_ERROR_EPERM           = -2147352575; /* 0x80020001 */
constexpr int KERNEL_ERROR_ENOENT          = -2147352574; /* 0x80020002 */
constexpr int KERNEL_ERROR_ESRCH           = -2147352573; /* 0x80020003 */
constexpr int KERNEL_ERROR_EINTR           = -2147352572; /* 0x80020004 */
constexpr int KERNEL_ERROR_EIO             = -2147352571; /* 0x80020005 */
constexpr int KERNEL_ERROR_ENXIO           = -2147352570; /* 0x80020006 */
constexpr int KERNEL_ERROR_E2BIG           = -2147352569; /* 0x80020007 */
constexpr int KERNEL_ERROR_ENOEXEC         = -2147352568; /* 0x80020008 */
constexpr int KERNEL_ERROR_EBADF           = -2147352567; /* 0x80020009 */
constexpr int KERNEL_ERROR_ECHILD          = -2147352566; /* 0x8002000A */
constexpr int KERNEL_ERROR_EDEADLK         = -2147352565; /* 0x8002000B */
constexpr int KERNEL_ERROR_ENOMEM          = -2147352564; /* 0x8002000C */
constexpr int KERNEL_ERROR_EACCES          = -2147352563; /* 0x8002000D */
constexpr int KERNEL_ERROR_EFAULT          = -2147352562; /* 0x8002000E */
constexpr int KERNEL_ERROR_ENOTBLK         = -2147352561; /* 0x8002000F */
constexpr int KERNEL_ERROR_EBUSY           = -2147352560; /* 0x80020010 */
constexpr int KERNEL_ERROR_EEXIST          = -2147352559; /* 0x80020011 */
constexpr int KERNEL_ERROR_EXDEV           = -2147352558; /* 0x80020012 */
constexpr int KERNEL_ERROR_ENODEV          = -2147352557; /* 0x80020013 */
constexpr int KERNEL_ERROR_ENOTDIR         = -2147352556; /* 0x80020014 */
constexpr int KERNEL_ERROR_EISDIR          = -2147352555; /* 0x80020015 */
constexpr int KERNEL_ERROR_EINVAL          = -2147352554; /* 0x80020016 */
constexpr int KERNEL_ERROR_ENFILE          = -2147352553; /* 0x80020017 */
constexpr int KERNEL_ERROR_EMFILE          = -2147352552; /* 0x80020018 */
constexpr int KERNEL_ERROR_ENOTTY          = -2147352551; /* 0x80020019 */
constexpr int KERNEL_ERROR_ETXTBSY         = -2147352550; /* 0x8002001A */
constexpr int KERNEL_ERROR_EFBIG           = -2147352549; /* 0x8002001B */
constexpr int KERNEL_ERROR_ENOSPC          = -2147352548; /* 0x8002001C */
constexpr int KERNEL_ERROR_ESPIPE          = -2147352547; /* 0x8002001D */
constexpr int KERNEL_ERROR_EROFS           = -2147352546; /* 0x8002001E */
constexpr int KERNEL_ERROR_EMLINK          = -2147352545; /* 0x8002001F */
constexpr int KERNEL_ERROR_EPIPE           = -2147352544; /* 0x80020020 */
constexpr int KERNEL_ERROR_EDOM            = -2147352543; /* 0x80020021 */
constexpr int KERNEL_ERROR_ERANGE          = -2147352542; /* 0x80020022 */
constexpr int KERNEL_ERROR_EAGAIN          = -2147352541; /* 0x80020023 */
constexpr int KERNEL_ERROR_EWOULDBLOCK     = -2147352541; /* 0x80020023 */
constexpr int KERNEL_ERROR_EINPROGRESS     = -2147352540; /* 0x80020024 */
constexpr int KERNEL_ERROR_EALREADY        = -2147352539; /* 0x80020025 */
constexpr int KERNEL_ERROR_ENOTSOCK        = -2147352538; /* 0x80020026 */
constexpr int KERNEL_ERROR_EDESTADDRREQ    = -2147352537; /* 0x80020027 */
constexpr int KERNEL_ERROR_EMSGSIZE        = -2147352536; /* 0x80020028 */
constexpr int KERNEL_ERROR_EPROTOTYPE      = -2147352535; /* 0x80020029 */
constexpr int KERNEL_ERROR_ENOPROTOOPT     = -2147352534; /* 0x8002002A */
constexpr int KERNEL_ERROR_EPROTONOSUPPORT = -2147352533; /* 0x8002002B */
constexpr int KERNEL_ERROR_ESOCKTNOSUPPORT = -2147352532; /* 0x8002002C */
constexpr int KERNEL_ERROR_EOPNOTSUPP      = -2147352531; /* 0x8002002D */
constexpr int KERNEL_ERROR_ENOTSUP         = -2147352531; /* 0x8002002D */
constexpr int KERNEL_ERROR_EPFNOSUPPORT    = -2147352530; /* 0x8002002E */
constexpr int KERNEL_ERROR_EAFNOSUPPORT    = -2147352529; /* 0x8002002F */
constexpr int KERNEL_ERROR_EADDRINUSE      = -2147352528; /* 0x80020030 */
constexpr int KERNEL_ERROR_EADDRNOTAVAIL   = -2147352527; /* 0x80020031 */
constexpr int KERNEL_ERROR_ENETDOWN        = -2147352526; /* 0x80020032 */
constexpr int KERNEL_ERROR_ENETUNREACH     = -2147352525; /* 0x80020033 */
constexpr int KERNEL_ERROR_ENETRESET       = -2147352524; /* 0x80020034 */
constexpr int KERNEL_ERROR_ECONNABORTED    = -2147352523; /* 0x80020035 */
constexpr int KERNEL_ERROR_ECONNRESET      = -2147352522; /* 0x80020036 */
constexpr int KERNEL_ERROR_ENOBUFS         = -2147352521; /* 0x80020037 */
constexpr int KERNEL_ERROR_EISCONN         = -2147352520; /* 0x80020038 */
constexpr int KERNEL_ERROR_ENOTCONN        = -2147352519; /* 0x80020039 */
constexpr int KERNEL_ERROR_ESHUTDOWN       = -2147352518; /* 0x8002003A */
constexpr int KERNEL_ERROR_ETOOMANYREFS    = -2147352517; /* 0x8002003B */
constexpr int KERNEL_ERROR_ETIMEDOUT       = -2147352516; /* 0x8002003C */
constexpr int KERNEL_ERROR_ECONNREFUSED    = -2147352515; /* 0x8002003D */
constexpr int KERNEL_ERROR_ELOOP           = -2147352514; /* 0x8002003E */
constexpr int KERNEL_ERROR_ENAMETOOLONG    = -2147352513; /* 0x8002003F */
constexpr int KERNEL_ERROR_EHOSTDOWN       = -2147352512; /* 0x80020040 */
constexpr int KERNEL_ERROR_EHOSTUNREACH    = -2147352511; /* 0x80020041 */
constexpr int KERNEL_ERROR_ENOTEMPTY       = -2147352510; /* 0x80020042 */
constexpr int KERNEL_ERROR_EPROCLIM        = -2147352509; /* 0x80020043 */
constexpr int KERNEL_ERROR_EUSERS          = -2147352508; /* 0x80020044 */
constexpr int KERNEL_ERROR_EDQUOT          = -2147352507; /* 0x80020045 */
constexpr int KERNEL_ERROR_ESTALE          = -2147352506; /* 0x80020046 */
constexpr int KERNEL_ERROR_EREMOTE         = -2147352505; /* 0x80020047 */
constexpr int KERNEL_ERROR_EBADRPC         = -2147352504; /* 0x80020048 */
constexpr int KERNEL_ERROR_ERPCMISMATCH    = -2147352503; /* 0x80020049 */
constexpr int KERNEL_ERROR_EPROGUNAVAIL    = -2147352502; /* 0x8002004A */
constexpr int KERNEL_ERROR_EPROGMISMATCH   = -2147352501; /* 0x8002004B */
constexpr int KERNEL_ERROR_EPROCUNAVAIL    = -2147352500; /* 0x8002004C */
constexpr int KERNEL_ERROR_ENOLCK          = -2147352499; /* 0x8002004D */
constexpr int KERNEL_ERROR_ENOSYS          = -2147352498; /* 0x8002004E */
constexpr int KERNEL_ERROR_EFTYPE          = -2147352497; /* 0x8002004F */
constexpr int KERNEL_ERROR_EAUTH           = -2147352496; /* 0x80020050 */
constexpr int KERNEL_ERROR_ENEEDAUTH       = -2147352495; /* 0x80020051 */
constexpr int KERNEL_ERROR_EIDRM           = -2147352494; /* 0x80020052 */
constexpr int KERNEL_ERROR_ENOMSG          = -2147352493; /* 0x80020053 */
constexpr int KERNEL_ERROR_EOVERFLOW       = -2147352492; /* 0x80020054 */
constexpr int KERNEL_ERROR_ECANCELED       = -2147352491; /* 0x80020055 */
constexpr int KERNEL_ERROR_EILSEQ          = -2147352490; /* 0x80020056 */
constexpr int KERNEL_ERROR_ENOATTR         = -2147352489; /* 0x80020057 */
constexpr int KERNEL_ERROR_EDOOFUS         = -2147352488; /* 0x80020058 */
constexpr int KERNEL_ERROR_EBADMSG         = -2147352487; /* 0x80020059 */
constexpr int KERNEL_ERROR_EMULTIHOP       = -2147352486; /* 0x8002005A */
constexpr int KERNEL_ERROR_ENOLINK         = -2147352485; /* 0x8002005B */
constexpr int KERNEL_ERROR_EPROTO          = -2147352484; /* 0x8002005C */
constexpr int KERNEL_ERROR_ENOTCAPABLE     = -2147352483; /* 0x8002005D */
constexpr int KERNEL_ERROR_ECAPMODE        = -2147352482; /* 0x8002005E */
constexpr int KERNEL_ERROR_ENOBLK          = -2147352481; /* 0x8002005F */
constexpr int KERNEL_ERROR_EICV            = -2147352480; /* 0x80020060 */
constexpr int KERNEL_ERROR_ENOPLAYGOENT    = -2147352479; /* 0x80020061 */
constexpr int KERNEL_ERROR_EREVOKE         = -2147352478; /* 0x80020062 */
constexpr int KERNEL_ERROR_ESDKVERSION     = -2147352477; /* 0x80020063 */
constexpr int KERNEL_ERROR_ESTART          = -2147352476; /* 0x80020064 */
constexpr int KERNEL_ERROR_ESTOP           = -2147352475; /* 0x80020065 */

} // namespace LibKernel

namespace VideoOut {

constexpr int VIDEO_OUT_ERROR_INVALID_VALUE                    = -2144796671; /* 0x80290001 */
constexpr int VIDEO_OUT_ERROR_INVALID_ADDRESS                  = -2144796670; /* 0x80290002 */
constexpr int VIDEO_OUT_ERROR_INVALID_PIXEL_FORMAT             = -2144796669; /* 0x80290003 */
constexpr int VIDEO_OUT_ERROR_INVALID_PITCH                    = -2144796668; /* 0x80290004 */
constexpr int VIDEO_OUT_ERROR_INVALID_RESOLUTION               = -2144796667; /* 0x80290005 */
constexpr int VIDEO_OUT_ERROR_INVALID_FLIP_MODE                = -2144796666; /* 0x80290006 */
constexpr int VIDEO_OUT_ERROR_INVALID_TILING_MODE              = -2144796665; /* 0x80290007 */
constexpr int VIDEO_OUT_ERROR_INVALID_ASPECT_RATIO             = -2144796664; /* 0x80290008 */
constexpr int VIDEO_OUT_ERROR_RESOURCE_BUSY                    = -2144796663; /* 0x80290009 */
constexpr int VIDEO_OUT_ERROR_INVALID_INDEX                    = -2144796662; /* 0x8029000A */
constexpr int VIDEO_OUT_ERROR_INVALID_HANDLE                   = -2144796661; /* 0x8029000B */
constexpr int VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE              = -2144796660; /* 0x8029000C */
constexpr int VIDEO_OUT_ERROR_INVALID_EVENT                    = -2144796659; /* 0x8029000D */
constexpr int VIDEO_OUT_ERROR_NO_EMPTY_SLOT                    = -2144796657; /* 0x8029000F */
constexpr int VIDEO_OUT_ERROR_SLOT_OCCUPIED                    = -2144796656; /* 0x80290010 */
constexpr int VIDEO_OUT_ERROR_FLIP_QUEUE_FULL                  = -2144796654; /* 0x80290012 */
constexpr int VIDEO_OUT_ERROR_INVALID_MEMORY                   = -2144796653; /* 0x80290013 */
constexpr int VIDEO_OUT_ERROR_MEMORY_NOT_PHYSICALLY_CONTIGUOUS = -2144796652; /* 0x80290014 */
constexpr int VIDEO_OUT_ERROR_MEMORY_INVALID_ALIGNMENT         = -2144796651; /* 0x80290015 */
constexpr int VIDEO_OUT_ERROR_UNSUPPORTED_OUTPUT_MODE          = -2144796650; /* 0x80290016 */
constexpr int VIDEO_OUT_ERROR_OVERFLOW                         = -2144796649; /* 0x80290017 */
constexpr int VIDEO_OUT_ERROR_NO_DEVICE                        = -2144796648; /* 0x80290018 */
constexpr int VIDEO_OUT_ERROR_UNAVAILABLE_OUTPUT_MODE          = -2144796647; /* 0x80290019 */
constexpr int VIDEO_OUT_ERROR_INVALID_OPTION                   = -2144796646; /* 0x8029001A */
constexpr int VIDEO_OUT_ERROR_PORT_UNSUPPORTED_FUNCTION        = -2144796645; /* 0x8029001B */
constexpr int VIDEO_OUT_ERROR_UNSUPPORTED_OPERATION            = -2144796644; /* 0x8029001C */
constexpr int VIDEO_OUT_ERROR_FATAL                            = -2144796417; /* 0x802900FF */
constexpr int VIDEO_OUT_ERROR_UNKNOWN                          = -2144796418; /* 0x802900FE */
constexpr int VIDEO_OUT_ERROR_ENOMEM                           = -2144792564; /* 0x8029100C */

} // namespace VideoOut

namespace Audio {

constexpr int AUDIO_OUT_ERROR_NOT_OPENED          = -2144993279; /* 0x80260001 */
constexpr int AUDIO_OUT_ERROR_BUSY                = -2144993278; /* 0x80260002 */
constexpr int AUDIO_OUT_ERROR_INVALID_PORT        = -2144993277; /* 0x80260003 */
constexpr int AUDIO_OUT_ERROR_INVALID_POINTER     = -2144993276; /* 0x80260004 */
constexpr int AUDIO_OUT_ERROR_PORT_FULL           = -2144993275; /* 0x80260005 */
constexpr int AUDIO_OUT_ERROR_INVALID_SIZE        = -2144993274; /* 0x80260006 */
constexpr int AUDIO_OUT_ERROR_INVALID_FORMAT      = -2144993273; /* 0x80260007 */
constexpr int AUDIO_OUT_ERROR_INVALID_SAMPLE_FREQ = -2144993272; /* 0x80260008 */
constexpr int AUDIO_OUT_ERROR_INVALID_VOLUME      = -2144993271; /* 0x80260009 */
constexpr int AUDIO_OUT_ERROR_INVALID_PORT_TYPE   = -2144993270; /* 0x8026000A */
constexpr int AUDIO_OUT_ERROR_INVALID_CONF_TYPE   = -2144993268; /* 0x8026000C */
constexpr int AUDIO_OUT_ERROR_OUT_OF_MEMORY       = -2144993267; /* 0x8026000D */
constexpr int AUDIO_OUT_ERROR_ALREADY_INIT        = -2144993266; /* 0x8026000E */
constexpr int AUDIO_OUT_ERROR_NOT_INIT            = -2144993265; /* 0x8026000F */
constexpr int AUDIO_OUT_ERROR_MEMORY              = -2144993264; /* 0x80260010 */
constexpr int AUDIO_OUT_ERROR_SYSTEM_RESOURCE     = -2144993263; /* 0x80260011 */
constexpr int AUDIO_OUT_ERROR_TRANS_EVENT         = -2144993262; /* 0x80260012 */
constexpr int AUDIO_OUT_ERROR_INVALID_FLAG        = -2144993261; /* 0x80260013 */
constexpr int AUDIO_OUT_ERROR_INVALID_MIXLEVEL    = -2144993260; /* 0x80260014 */
constexpr int AUDIO_OUT_ERROR_INVALID_ARG         = -2144993259; /* 0x80260015 */
constexpr int AUDIO_OUT_ERROR_INVALID_PARAM       = -2144993258; /* 0x80260016 */

constexpr int AUDIO_IN_ERROR_FATAL           = -2144993024; /* 0x80260100 */
constexpr int AUDIO_IN_ERROR_INVALID_HANDLE  = -2144993023; /* 0x80260101 */
constexpr int AUDIO_IN_ERROR_INVALID_SIZE    = -2144993022; /* 0x80260102 */
constexpr int AUDIO_IN_ERROR_INVALID_FREQ    = -2144993021; /* 0x80260103 */
constexpr int AUDIO_IN_ERROR_INVALID_TYPE    = -2144993020; /* 0x80260104 */
constexpr int AUDIO_IN_ERROR_INVALID_POINTER = -2144993019; /* 0x80260105 */
constexpr int AUDIO_IN_ERROR_INVALID_PARAM   = -2144993018; /* 0x80260106 */
constexpr int AUDIO_IN_ERROR_PORT_FULL       = -2144993017; /* 0x80260107 */
constexpr int AUDIO_IN_ERROR_OUT_OF_MEMORY   = -2144993016; /* 0x80260108 */
constexpr int AUDIO_IN_ERROR_NOT_OPENED      = -2144993015; /* 0x80260109 */
constexpr int AUDIO_IN_ERROR_BUSY            = -2144993014; /* 0x8026010A */
constexpr int AUDIO_IN_ERROR_SYSTEM_MEMORY   = -2144993013; /* 0x8026010B */
constexpr int AUDIO_IN_ERROR_SYSTEM_IPC      = -2144993012; /* 0x8026010C */

} // namespace Audio

namespace SystemService {

constexpr int SYSTEM_SERVICE_ERROR_INTERNAL                        = -2136932351; /* 0x80A10001 */
constexpr int SYSTEM_SERVICE_ERROR_UNAVAILABLE                     = -2136932350; /* 0x80A10002 */
constexpr int SYSTEM_SERVICE_ERROR_PARAMETER                       = -2136932349; /* 0x80A10003 */
constexpr int SYSTEM_SERVICE_ERROR_NO_EVENT                        = -2136932348; /* 0x80A10004 */
constexpr int SYSTEM_SERVICE_ERROR_REJECTED                        = -2136932347; /* 0x80A10005 */
constexpr int SYSTEM_SERVICE_ERROR_NEED_DISPLAY_SAFE_AREA_SETTINGS = -2136932346; /* 0x80A10006 */
constexpr int SYSTEM_SERVICE_ERROR_INVALID_URI_LEN                 = -2136932345; /* 0x80A10007 */
constexpr int SYSTEM_SERVICE_ERROR_INVALID_URI_SCHEME              = -2136932344; /* 0x80A10008 */
constexpr int SYSTEM_SERVICE_ERROR_NO_APP_INFO                     = -2136932343; /* 0x80A10009 */
constexpr int SYSTEM_SERVICE_ERROR_NOT_FLAG_IN_PARAM_SFO           = -2136932342; /* 0x80A1000A */

} // namespace SystemService

namespace Network {

inline int NetToPosix(int kernel_errno)
{
	return kernel_errno > -2143223551 && kernel_errno <= -2143223316 ? kernel_errno + 2143223551 + 1 : 204;
}

constexpr int NET_ERROR_EPERM                    = -2143223551; /* 0x80410101 */
constexpr int NET_ERROR_ENOENT                   = -2143223550; /* 0x80410102 */
constexpr int NET_ERROR_ESRCH                    = -2143223549; /* 0x80410103 */
constexpr int NET_ERROR_EINTR                    = -2143223548; /* 0x80410104 */
constexpr int NET_ERROR_EIO                      = -2143223547; /* 0x80410105 */
constexpr int NET_ERROR_ENXIO                    = -2143223546; /* 0x80410106 */
constexpr int NET_ERROR_E2BIG                    = -2143223545; /* 0x80410107 */
constexpr int NET_ERROR_ENOEXEC                  = -2143223544; /* 0x80410108 */
constexpr int NET_ERROR_EBADF                    = -2143223543; /* 0x80410109 */
constexpr int NET_ERROR_ECHILD                   = -2143223542; /* 0x8041010A */
constexpr int NET_ERROR_EDEADLK                  = -2143223541; /* 0x8041010B */
constexpr int NET_ERROR_ENOMEM                   = -2143223540; /* 0x8041010C */
constexpr int NET_ERROR_EACCES                   = -2143223539; /* 0x8041010D */
constexpr int NET_ERROR_EFAULT                   = -2143223538; /* 0x8041010E */
constexpr int NET_ERROR_ENOTBLK                  = -2143223537; /* 0x8041010F */
constexpr int NET_ERROR_EBUSY                    = -2143223536; /* 0x80410110 */
constexpr int NET_ERROR_EEXIST                   = -2143223535; /* 0x80410111 */
constexpr int NET_ERROR_EXDEV                    = -2143223534; /* 0x80410112 */
constexpr int NET_ERROR_ENODEV                   = -2143223533; /* 0x80410113 */
constexpr int NET_ERROR_ENOTDIR                  = -2143223532; /* 0x80410114 */
constexpr int NET_ERROR_EISDIR                   = -2143223531; /* 0x80410115 */
constexpr int NET_ERROR_EINVAL                   = -2143223530; /* 0x80410116 */
constexpr int NET_ERROR_ENFILE                   = -2143223529; /* 0x80410117 */
constexpr int NET_ERROR_EMFILE                   = -2143223528; /* 0x80410118 */
constexpr int NET_ERROR_ENOTTY                   = -2143223527; /* 0x80410119 */
constexpr int NET_ERROR_ETXTBSY                  = -2143223526; /* 0x8041011A */
constexpr int NET_ERROR_EFBIG                    = -2143223525; /* 0x8041011B */
constexpr int NET_ERROR_ENOSPC                   = -2143223524; /* 0x8041011C */
constexpr int NET_ERROR_ESPIPE                   = -2143223523; /* 0x8041011D */
constexpr int NET_ERROR_EROFS                    = -2143223522; /* 0x8041011E */
constexpr int NET_ERROR_EMLINK                   = -2143223521; /* 0x8041011F */
constexpr int NET_ERROR_EPIPE                    = -2143223520; /* 0x80410120 */
constexpr int NET_ERROR_EDOM                     = -2143223519; /* 0x80410121 */
constexpr int NET_ERROR_ERANGE                   = -2143223518; /* 0x80410122 */
constexpr int NET_ERROR_EAGAIN                   = -2143223517; /* 0x80410123 */
constexpr int NET_ERROR_EWOULDBLOCK              = -2143223517; /* 0x80410123 */
constexpr int NET_ERROR_EINPROGRESS              = -2143223516; /* 0x80410124 */
constexpr int NET_ERROR_EALREADY                 = -2143223515; /* 0x80410125 */
constexpr int NET_ERROR_ENOTSOCK                 = -2143223514; /* 0x80410126 */
constexpr int NET_ERROR_EDESTADDRREQ             = -2143223513; /* 0x80410127 */
constexpr int NET_ERROR_EMSGSIZE                 = -2143223512; /* 0x80410128 */
constexpr int NET_ERROR_EPROTOTYPE               = -2143223511; /* 0x80410129 */
constexpr int NET_ERROR_ENOPROTOOPT              = -2143223510; /* 0x8041012A */
constexpr int NET_ERROR_EPROTONOSUPPORT          = -2143223509; /* 0x8041012B */
constexpr int NET_ERROR_ESOCKTNOSUPPORT          = -2143223508; /* 0x8041012C */
constexpr int NET_ERROR_EOPNOTSUPP               = -2143223507; /* 0x8041012D */
constexpr int NET_ERROR_ENOTSUP                  = -2143223507; /* 0x8041012D */
constexpr int NET_ERROR_EPFNOSUPPORT             = -2143223506; /* 0x8041012E */
constexpr int NET_ERROR_EAFNOSUPPORT             = -2143223505; /* 0x8041012F */
constexpr int NET_ERROR_EADDRINUSE               = -2143223504; /* 0x80410130 */
constexpr int NET_ERROR_EADDRNOTAVAIL            = -2143223503; /* 0x80410131 */
constexpr int NET_ERROR_ENETDOWN                 = -2143223502; /* 0x80410132 */
constexpr int NET_ERROR_ENETUNREACH              = -2143223501; /* 0x80410133 */
constexpr int NET_ERROR_ENETRESET                = -2143223500; /* 0x80410134 */
constexpr int NET_ERROR_ECONNABORTED             = -2143223499; /* 0x80410135 */
constexpr int NET_ERROR_ECONNRESET               = -2143223498; /* 0x80410136 */
constexpr int NET_ERROR_ENOBUFS                  = -2143223497; /* 0x80410137 */
constexpr int NET_ERROR_EISCONN                  = -2143223496; /* 0x80410138 */
constexpr int NET_ERROR_ENOTCONN                 = -2143223495; /* 0x80410139 */
constexpr int NET_ERROR_ESHUTDOWN                = -2143223494; /* 0x8041013A */
constexpr int NET_ERROR_ETOOMANYREFS             = -2143223493; /* 0x8041013B */
constexpr int NET_ERROR_ETIMEDOUT                = -2143223492; /* 0x8041013C */
constexpr int NET_ERROR_ECONNREFUSED             = -2143223491; /* 0x8041013D */
constexpr int NET_ERROR_ELOOP                    = -2143223490; /* 0x8041013E */
constexpr int NET_ERROR_ENAMETOOLONG             = -2143223489; /* 0x8041013F */
constexpr int NET_ERROR_EHOSTDOWN                = -2143223488; /* 0x80410140 */
constexpr int NET_ERROR_EHOSTUNREACH             = -2143223487; /* 0x80410141 */
constexpr int NET_ERROR_ENOTEMPTY                = -2143223486; /* 0x80410142 */
constexpr int NET_ERROR_EPROCLIM                 = -2143223485; /* 0x80410143 */
constexpr int NET_ERROR_EUSERS                   = -2143223484; /* 0x80410144 */
constexpr int NET_ERROR_EDQUOT                   = -2143223483; /* 0x80410145 */
constexpr int NET_ERROR_ESTALE                   = -2143223482; /* 0x80410146 */
constexpr int NET_ERROR_EREMOTE                  = -2143223481; /* 0x80410147 */
constexpr int NET_ERROR_EBADRPC                  = -2143223480; /* 0x80410148 */
constexpr int NET_ERROR_ERPCMISMATCH             = -2143223479; /* 0x80410149 */
constexpr int NET_ERROR_EPROGUNAVAIL             = -2143223478; /* 0x8041014A */
constexpr int NET_ERROR_EPROGMISMATCH            = -2143223477; /* 0x8041014B */
constexpr int NET_ERROR_EPROCUNAVAIL             = -2143223476; /* 0x8041014C */
constexpr int NET_ERROR_ENOLCK                   = -2143223475; /* 0x8041014D */
constexpr int NET_ERROR_ENOSYS                   = -2143223474; /* 0x8041014E */
constexpr int NET_ERROR_EFTYPE                   = -2143223473; /* 0x8041014F */
constexpr int NET_ERROR_EAUTH                    = -2143223472; /* 0x80410150 */
constexpr int NET_ERROR_ENEEDAUTH                = -2143223471; /* 0x80410151 */
constexpr int NET_ERROR_EIDRM                    = -2143223470; /* 0x80410152 */
constexpr int NET_ERROR_ENOMS                    = -2143223469; /* 0x80410153 */
constexpr int NET_ERROR_EOVERFLOW                = -2143223468; /* 0x80410154 */
constexpr int NET_ERROR_ECANCELED                = -2143223467; /* 0x80410155 */
constexpr int NET_ERROR_EPROTO                   = -2143223460; /* 0x8041015C */
constexpr int NET_ERROR_EADHOC                   = -2143223392; /* 0x804101A0 */
constexpr int NET_ERROR_ERESERVED161             = -2143223391; /* 0x804101A1 */
constexpr int NET_ERROR_ERESERVED162             = -2143223390; /* 0x804101A2 */
constexpr int NET_ERROR_EINACTIVEDISABLED        = -2143223389; /* 0x804101A3 */
constexpr int NET_ERROR_ENODATA                  = -2143223388; /* 0x804101A4 */
constexpr int NET_ERROR_EDESC                    = -2143223387; /* 0x804101A5 */
constexpr int NET_ERROR_EDESCTIMEDOUT            = -2143223386; /* 0x804101A6 */
constexpr int NET_ERROR_ENETINTR                 = -2143223385; /* 0x804101A7 */
constexpr int NET_ERROR_ENOTINIT                 = -2143223352; /* 0x804101C8 */
constexpr int NET_ERROR_ENOLIBMEM                = -2143223351; /* 0x804101C9 */
constexpr int NET_ERROR_ERESERVED202             = -2143223350; /* 0x804101CA */
constexpr int NET_ERROR_ECALLBACK                = -2143223349; /* 0x804101CB */
constexpr int NET_ERROR_EINTERNAL                = -2143223348; /* 0x804101CC */
constexpr int NET_ERROR_ERETURN                  = -2143223347; /* 0x804101CD */
constexpr int NET_ERROR_ENOALLOCMEM              = -2143223346; /* 0x804101CE */
constexpr int NET_ERROR_RESOLVER_EINTERNAL       = -2143223332; /* 0x804101DC */
constexpr int NET_ERROR_RESOLVER_EBUSY           = -2143223331; /* 0x804101DD */
constexpr int NET_ERROR_RESOLVER_ENOSPACE        = -2143223330; /* 0x804101DE */
constexpr int NET_ERROR_RESOLVER_EPACKET         = -2143223329; /* 0x804101DF */
constexpr int NET_ERROR_RESOLVER_ERESERVED224    = -2143223328; /* 0x804101E0 */
constexpr int NET_ERROR_RESOLVER_ENODNS          = -2143223327; /* 0x804101E1 */
constexpr int NET_ERROR_RESOLVER_ETIMEDOUT       = -2143223326; /* 0x804101E2 */
constexpr int NET_ERROR_RESOLVER_ENOSUPPORT      = -2143223325; /* 0x804101E3 */
constexpr int NET_ERROR_RESOLVER_EFORMAT         = -2143223324; /* 0x804101E4 */
constexpr int NET_ERROR_RESOLVER_ESERVERFAILURE  = -2143223323; /* 0x804101E5 */
constexpr int NET_ERROR_RESOLVER_ENOHOST         = -2143223322; /* 0x804101E6 */
constexpr int NET_ERROR_RESOLVER_ENOTIMPLEMENTED = -2143223321; /* 0x804101E7 */
constexpr int NET_ERROR_RESOLVER_ESERVERREFUSED  = -2143223320; /* 0x804101E8 */
constexpr int NET_ERROR_RESOLVER_ENORECORD       = -2143223319; /* 0x804101E9 */
constexpr int NET_ERROR_RESOLVER_EALIGNMENT      = -2143223318; /* 0x804101EA */
constexpr int NET_ERROR_RESOLVER_ENOTFOUND       = -2143223317; /* 0x804101EB */
constexpr int NET_ERROR_RESOLVER_ENOTINIT        = -2143223316; /* 0x804101EC */

constexpr int SSL_ERROR_BEFORE_INIT                             = -2137657343; /* 0x8095F001 */
constexpr int SSL_ERROR_ALREADY_INITED                          = -2137657342; /* 0x8095F002 */
constexpr int SSL_ERROR_BROKEN                                  = -2137657341; /* 0x8095F003 */
constexpr int SSL_ERROR_NOT_FOUND                               = -2137657340; /* 0x8095F004 */
constexpr int SSL_ERROR_INVALID_FORMAT                          = -2137657339; /* 0x8095F005 */
constexpr int SSL_ERROR_INVALID_ID                              = -2137657338; /* 0x8095F006 */
constexpr int SSL_ERROR_INVALID_VALUE                           = -2137657337; /* 0x8095F007 */
constexpr int SSL_ERROR_OUT_OF_SIZE                             = -2137657336; /* 0x8095F008 */
constexpr int SSL_ERROR_INTERNAL                                = -2137657335; /* 0x8095F009 */
constexpr int SSL_ERROR_INVALID_CERT                            = -2137657334; /* 0x8095F00A */
constexpr int SSL_ERROR_CN_CHECK                                = -2137657333; /* 0x8095F00B */
constexpr int SSL_ERROR_UNKNOWN_CA                              = -2137657332; /* 0x8095F00C */
constexpr int SSL_ERROR_NOT_AFTER_CHECK                         = -2137657331; /* 0x8095F00D */
constexpr int SSL_ERROR_NOT_BEFORE_CHECK                        = -2137657330; /* 0x8095F00E */
constexpr int SSL_ERROR_EAGAIN                                  = -2137657329; /* 0x8095F00F */
constexpr int SSL_ERROR_FATAL_ALERT                             = -2137657328; /* 0x8095F010 */
constexpr int SSL_ERROR_BUSY                                    = -2137657326; /* 0x8095F012 */
constexpr int SSL_ERROR_WANT_POLLIN                             = -2137657325; /* 0x8095F013 */
constexpr int SSL_ERROR_WANT_POLLOUT                            = -2137657324; /* 0x8095F014 */
constexpr int SSL_ERROR_INSUFFICIENT_STACKSIZE                  = -2137657323; /* 0x8095F015 */
constexpr int SSL_ERROR_APPLICATION_VERIFICATION_CALLBACK_CHECK = -2137657091; /* 0x8095F0FD */
constexpr int SSL_ERROR_OUT_OF_MEMORY                           = -2137712683; /* 0x809517D5 */
constexpr int SSL_ERROR_READ_TIMEOUT                            = -2137712880; /* 0x80951710 */
constexpr int SSL_ERROR_SOCKET_CLOSED                           = -2137712883; /* 0x8095170D */
constexpr int SSL_ERROR_BAD_LENGTH                              = -2137712781; /* 0x80951773 */
constexpr int SSL_ERROR_INDEX_OOB                               = -2137712776; /* 0x80951778 */
constexpr int SSL_ERROR_INVALID_ARG                             = -2137712774; /* 0x8095177A */
constexpr int SSL_ERROR_EOF                                     = -2137712772; /* 0x8095177C */
constexpr int SSL_ERROR_BAD_EXPONENT                            = -2137712771; /* 0x8095177D */
constexpr int SSL_ERROR_INCOMPLETE_SEARCH                       = -2137712770; /* 0x8095177E */
constexpr int SSL_ERROR_INTERNAL_ERROR                          = -2137712769; /* 0x8095177F */
constexpr int SSL_ERROR_BAD_CERT_LENGTH                         = -2137710382; /* 0x809520D2 */
constexpr int SSL_ERROR_BAD_SIGN_ALGO                           = -2137710383; /* 0x809520D1 */
constexpr int SSL_ERROR_MISMATCH_PUBLIC_KEYS                    = -2137710381; /* 0x809520D3 */
constexpr int SSL_ERROR_KEY_BLOB_CORRUPT                        = -2137710380; /* 0x809520D4 */

constexpr int HTTP_ERROR_BEFORE_INIT                 = -2143088639; /*(0x80431001)	*/
constexpr int HTTP_ERROR_ALREADY_INITED              = -2143088608; /*(0x80431020)	*/
constexpr int HTTP_ERROR_BUSY                        = -2143088607; /*(0x80431021)	*/
constexpr int HTTP_ERROR_OUT_OF_MEMORY               = -2143088606; /*(0x80431022)	*/
constexpr int HTTP_ERROR_NOT_FOUND                   = -2143088603; /*(0x80431025)	*/
constexpr int HTTP_ERROR_INVALID_VERSION             = -2143088534; /*(0x8043106a)	*/
constexpr int HTTP_ERROR_INVALID_ID                  = -2143088384; /*(0x80431100)	*/
constexpr int HTTP_ERROR_OUT_OF_SIZE                 = -2143088380; /*(0x80431104)	*/
constexpr int HTTP_ERROR_INVALID_VALUE               = -2143088130; /*(0x804311fe)	*/
constexpr int HTTP_ERROR_INVALID_URL                 = -2143080352; /*(0x80433060)	*/
constexpr int HTTP_ERROR_UNKNOWN_SCHEME              = -2143088543; /*(0x80431061)	*/
constexpr int HTTP_ERROR_NETWORK                     = -2143088541; /*(0x80431063)	*/
constexpr int HTTP_ERROR_BAD_RESPONSE                = -2143088540; /*(0x80431064)	*/
constexpr int HTTP_ERROR_BEFORE_SEND                 = -2143088539; /*(0x80431065)	*/
constexpr int HTTP_ERROR_AFTER_SEND                  = -2143088538; /*(0x80431066)	*/
constexpr int HTTP_ERROR_TIMEOUT                     = -2143088536; /*(0x80431068)	*/
constexpr int HTTP_ERROR_UNKNOWN_AUTH_TYPE           = -2143088535; /*(0x80431069)	*/
constexpr int HTTP_ERROR_UNKNOWN_METHOD              = -2143088533; /*(0x8043106b)	*/
constexpr int HTTP_ERROR_READ_BY_HEAD_METHOD         = -2143088529; /*(0x8043106f)	*/
constexpr int HTTP_ERROR_NOT_IN_COM                  = -2143088528; /*(0x80431070)	*/
constexpr int HTTP_ERROR_NO_CONTENT_LENGTH           = -2143088527; /*(0x80431071)	*/
constexpr int HTTP_ERROR_CHUNK_ENC                   = -2143088526; /*(0x80431072)	*/
constexpr int HTTP_ERROR_TOO_LARGE_RESPONSE_HEADER   = -2143088525; /*(0x80431073)	*/
constexpr int HTTP_ERROR_SSL                         = -2143088523; /*(0x80431075)	*/
constexpr int HTTP_ERROR_INSUFFICIENT_STACKSIZE      = -2143088522; /*(0x80431076)	*/
constexpr int HTTP_ERROR_ABORTED                     = -2143088512; /*(0x80431080)	*/
constexpr int HTTP_ERROR_UNKNOWN                     = -2143088511; /*(0x80431081)	*/
constexpr int HTTP_ERROR_EAGAIN                      = -2143088510; /*(0x80431082)	*/
constexpr int HTTP_ERROR_PROXY                       = -2143088508; /*(0x80431084)	*/
constexpr int HTTP_ERROR_BROKEN                      = -2143088507; /*(0x80431085)	*/
constexpr int HTTP_ERROR_PARSE_HTTP_NOT_FOUND        = -2143084507; /*(0x80432025)	*/
constexpr int HTTP_ERROR_PARSE_HTTP_INVALID_RESPONSE = -2143084448; /*(0x80432060)	*/
constexpr int HTTP_ERROR_PARSE_HTTP_INVALID_VALUE    = -2143084034; /*(0x804321fe)	*/
constexpr int HTTP_ERROR_RESOLVER_EPACKET            = -2143068159; /*(0x80436001)	*/
constexpr int HTTP_ERROR_RESOLVER_ENODNS             = -2143068158; /*(0x80436002)	*/
constexpr int HTTP_ERROR_RESOLVER_ETIMEDOUT          = -2143068157; /*(0x80436003)	*/
constexpr int HTTP_ERROR_RESOLVER_ENOSUPPORT         = -2143068156; /*(0x80436004)	*/
constexpr int HTTP_ERROR_RESOLVER_EFORMAT            = -2143068155; /*(0x80436005)	*/
constexpr int HTTP_ERROR_RESOLVER_ESERVERFAILURE     = -2143068154; /*(0x80436006)	*/
constexpr int HTTP_ERROR_RESOLVER_ENOHOST            = -2143068153; /*(0x80436007)	*/
constexpr int HTTP_ERROR_RESOLVER_ENOTIMPLEMENTED    = -2143068152; /*(0x80436008)	*/
constexpr int HTTP_ERROR_RESOLVER_ESERVERREFUSED     = -2143068151; /*(0x80436009)	*/
constexpr int HTTP_ERROR_RESOLVER_ENORECORD          = -2143068150; /*(0x8043600a)	*/
constexpr int HTTPS_ERROR_CERT                       = -2143072160; /*(0x80435060)	*/
constexpr int HTTPS_ERROR_HANDSHAKE                  = -2143072159; /*(0x80435061)	*/
constexpr int HTTPS_ERROR_IO                         = -2143072158; /*(0x80435062)	*/
constexpr int HTTPS_ERROR_INTERNAL                   = -2143072157; /*(0x80435063)	*/
constexpr int HTTPS_ERROR_PROXY                      = -2143072156; /*(0x80435064)	*/

} // namespace Network

namespace PlayGo {

constexpr int PLAYGO_ERROR_UNKNOWN             = -2135818239; /* 0x80B20001 */
constexpr int PLAYGO_ERROR_FATAL               = -2135818238; /* 0x80B20002 */
constexpr int PLAYGO_ERROR_NO_MEMORY           = -2135818237; /* 0x80B20003 */
constexpr int PLAYGO_ERROR_INVALID_ARGUMENT    = -2135818236; /* 0x80B20004 */
constexpr int PLAYGO_ERROR_NOT_INITIALIZED     = -2135818235; /* 0x80B20005 */
constexpr int PLAYGO_ERROR_ALREADY_INITIALIZED = -2135818234; /* 0x80B20006 */
constexpr int PLAYGO_ERROR_ALREADY_STARTED     = -2135818233; /* 0x80B20007 */
constexpr int PLAYGO_ERROR_NOT_STARTED         = -2135818232; /* 0x80B20008 */
constexpr int PLAYGO_ERROR_BAD_HANDLE          = -2135818231; /* 0x80B20009 */
constexpr int PLAYGO_ERROR_BAD_POINTER         = -2135818230; /* 0x80B2000A */
constexpr int PLAYGO_ERROR_BAD_SIZE            = -2135818229; /* 0x80B2000B */
constexpr int PLAYGO_ERROR_BAD_CHUNK_ID        = -2135818228; /* 0x80B2000C */
constexpr int PLAYGO_ERROR_BAD_SPEED           = -2135818227; /* 0x80B2000D */
constexpr int PLAYGO_ERROR_NOT_SUPPORT_PLAYGO  = -2135818226; /* 0x80B2000E */
constexpr int PLAYGO_ERROR_EPERM               = -2135818225; /* 0x80B2000F */
constexpr int PLAYGO_ERROR_BAD_LOCUS           = -2135818224; /* 0x80B20010 */
constexpr int PLAYGO_ERROR_NEED_DATA_DISC      = -2135818223; /* 0x80B20011 */

} // namespace PlayGo

namespace UserService {

constexpr int USER_SERVICE_ERROR_INTERNAL                = -2137653247; /* 0x80960001 */
constexpr int USER_SERVICE_ERROR_NOT_INITIALIZED         = -2137653246; /* 0x80960002 */
constexpr int USER_SERVICE_ERROR_ALREADY_INITIALIZED     = -2137653245; /* 0x80960003 */
constexpr int USER_SERVICE_ERROR_NO_MEMORY               = -2137653244; /* 0x80960004 */
constexpr int USER_SERVICE_ERROR_INVALID_ARGUMENT        = -2137653243; /* 0x80960005 */
constexpr int USER_SERVICE_ERROR_OPERATION_NOT_SUPPORTED = -2137653242; /* 0x80960006 */
constexpr int USER_SERVICE_ERROR_NO_EVENT                = -2137653241; /* 0x80960007 */
constexpr int USER_SERVICE_ERROR_NOT_LOGGED_IN           = -2137653239; /* 0x80960009 */
constexpr int USER_SERVICE_ERROR_BUFFER_TOO_SHORT        = -2137653238; /* 0x8096000A */

} // namespace UserService

namespace SaveData {

constexpr int SAVE_DATA_ERROR_PARAMETER                            = -2137063424; /* 0x809F0000 */
constexpr int SAVE_DATA_ERROR_NOT_INITIALIZED                      = -2137063423; /* 0x809F0001 */
constexpr int SAVE_DATA_ERROR_OUT_OF_MEMORY                        = -2137063422; /* 0x809F0002 */
constexpr int SAVE_DATA_ERROR_BUSY                                 = -2137063421; /* 0x809F0003 */
constexpr int SAVE_DATA_ERROR_NOT_MOUNTED                          = -2137063420; /* 0x809F0004 */
constexpr int SAVE_DATA_ERROR_NO_PERMISSION                        = -2137063419; /* 0x809F0005 */
constexpr int SAVE_DATA_ERROR_FINGERPRINT_MISMATCH                 = -2137063418; /* 0x809F0006 */
constexpr int SAVE_DATA_ERROR_EXISTS                               = -2137063417; /* 0x809F0007 */
constexpr int SAVE_DATA_ERROR_NOT_FOUND                            = -2137063416; /* 0x809F0008 */
constexpr int SAVE_DATA_ERROR_NO_SPACE_FS                          = -2137063414; /* 0x809F000A */
constexpr int SAVE_DATA_ERROR_INTERNAL                             = -2137063413; /* 0x809F000B */
constexpr int SAVE_DATA_ERROR_MOUNT_FULL                           = -2137063412; /* 0x809F000C */
constexpr int SAVE_DATA_ERROR_BAD_MOUNTED                          = -2137063411; /* 0x809F000D */
constexpr int SAVE_DATA_ERROR_FILE_NOT_FOUND                       = -2137063410; /* 0x809F000E */
constexpr int SAVE_DATA_ERROR_BROKEN                               = -2137063409; /* 0x809F000F */
constexpr int SAVE_DATA_ERROR_INVALID_LOGIN_USER                   = -2137063407; /* 0x809F0011 */
constexpr int SAVE_DATA_ERROR_MEMORY_NOT_READY                     = -2137063406; /* 0x809F0012 */
constexpr int SAVE_DATA_ERROR_BACKUP_BUSY                          = -2137063405; /* 0x809F0013 */
constexpr int SAVE_DATA_ERROR_NOT_REGIST_CALLBACK                  = -2137063403; /* 0x809F0015 */
constexpr int SAVE_DATA_ERROR_BUSY_FOR_SAVING                      = -2137063402; /* 0x809F0016 */
constexpr int SAVE_DATA_ERROR_LIMITATION_OVER                      = -2137063401; /* 0x809F0017 */
constexpr int SAVE_DATA_ERROR_EVENT_BUSY                           = -2137063400; /* 0x809F0018 */
constexpr int SAVE_DATA_ERROR_PARAMSFO_TRANSFER_TITLE_ID_NOT_FOUND = -2137063399; /* 0x809F0019 */

} // namespace SaveData

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LIBS_ERRNO_H_ */
