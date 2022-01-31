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
			*GetErrorAddr() = LibKernel::KernelToPosix(result);                                                                            \
			return -1;                                                                                                                     \
		}                                                                                                                                  \
		return 0;                                                                                                                          \
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

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LIBS_ERRNO_H_ */
