#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("DiscMap", 1, "DiscMap", 1, 1);

namespace DiscMap {

/*
DISC_MAP_ERROR_INVALID_ARGUMENT    = 0x81100001;
DISC_MAP_ERROR_LOCATION_NOT_MAPPED = 0x81100002;
DISC_MAP_ERROR_FILE_NOT_FOUND      = 0x81100003;
DISC_MAP_ERROR_NO_BITMAP_INFO      = 0x81100004;
*/

static KYTY_SYSV_ABI int DiscMapIsRequestOnHDD(const char* file, uint64_t a2, uint64_t a3, const int* a4)
{
	PRINT_NAME();

	printf("\tfile = %s\n", file);
	printf("\ta2 = %016" PRIx64 "\n", a2);
	printf("\ta3 = %016" PRIx64 "\n", a3);
	printf("\t*a4 = %08" PRIx32 "\n", *a4);

	return 0;
}

static KYTY_SYSV_ABI int Unknown1(const char* file, uint64_t a2, uint64_t a3, const uint64_t* a4, const uint64_t* a5, const uint64_t* a6)
{
	PRINT_NAME();

	printf("\tfile = %s\n", file);
	printf("\ta2 = %016" PRIx64 "\n", a2);
	printf("\ta3 = %016" PRIx64 "\n", a3);
	printf("\t*a4 = %016" PRIx64 "\n", *a4);
	printf("\t*a5 = %016" PRIx64 "\n", *a5);
	printf("\t*a6 = %016" PRIx64 "\n", *a6);

	return 0;
}

static KYTY_SYSV_ABI int Unknown2(const char* file, uint64_t a2, uint64_t a3, const uint64_t* a4, const uint64_t* a5, const uint64_t* a6)
{
	PRINT_NAME();

	printf("\tfile = %s\n", file);
	printf("\ta2 = %016" PRIx64 "\n", a2);
	printf("\ta3 = %016" PRIx64 "\n", a3);
	printf("\t*a4 = %016" PRIx64 "\n", *a4);
	printf("\t*a5 = %016" PRIx64 "\n", *a5);
	printf("\t*a6 = %016" PRIx64 "\n", *a6);

	return 0;
}

} // namespace DiscMap

LIB_DEFINE(InitDiscMap_1)
{
	LIB_FUNC("lbQKqsERhtE", DiscMap::DiscMapIsRequestOnHDD);
	LIB_FUNC("fJgP+wqifno", DiscMap::Unknown1);
	LIB_FUNC("ioKMruft1ek", DiscMap::Unknown2);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
