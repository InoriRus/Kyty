/*
** Name:        sqlite3secure.c
** Purpose:     Amalgamation of the wxSQLite3 encryption extension for SQLite
** Author:      Ulrich Telle
** Created:     2006-12-06
** Copyright:   (c) 2006-2019 Ulrich Telle
** License:     LGPL-3.0+ WITH WxWindows-exception-3.1
*/

/*
** Enable SQLite debug assertions if requested
*/
#ifndef SQLITE_DEBUG
#if defined(SQLITE_ENABLE_DEBUG) && (SQLITE_ENABLE_DEBUG == 1)
#define SQLITE_DEBUG 1
#endif
#endif

/*
** To enable the extension functions define SQLITE_ENABLE_EXTFUNC on compiling this module
** To enable the reading CSV files define SQLITE_ENABLE_CSV on compiling this module
** To enable the SHA3 support define SQLITE_ENABLE_SHA3 on compiling this module
** To enable the CARRAY support define SQLITE_ENABLE_CARRAY on compiling this module
** To enable the FILEIO support define SQLITE_ENABLE_FILEIO on compiling this module
** To enable the SERIES support define SQLITE_ENABLE_SERIES on compiling this module
*/
#if defined(SQLITE_HAS_CODEC)      || \
    defined(SQLITE_ENABLE_EXTFUNC) || \
    defined(SQLITE_ENABLE_CSV)     || \
    defined(SQLITE_ENABLE_SHA3)    || \
    defined(SQLITE_ENABLE_CARRAY)  || \
    defined(SQLITE_ENABLE_FILEIO)  || \
    defined(SQLITE_ENABLE_SERIES)
#define sqlite3_open    sqlite3_open_internal
#define sqlite3_open16  sqlite3_open16_internal
#define sqlite3_open_v2 sqlite3_open_v2_internal
#endif

/*
** Enable the user authentication feature
*/
#ifndef SQLITE_USER_AUTHENTICATION
#define SQLITE_USER_AUTHENTICATION 1
#endif

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>

/* SQLite functions only needed on Win32 */
extern void sqlite3_win32_write_debug(const char *, int);
extern char *sqlite3_win32_unicode_to_utf8(LPCWSTR);
extern char *sqlite3_win32_mbcs_to_utf8(const char *);
extern char *sqlite3_win32_mbcs_to_utf8_v2(const char *, int);
extern char *sqlite3_win32_utf8_to_mbcs(const char *);
extern char *sqlite3_win32_utf8_to_mbcs_v2(const char *, int);
extern LPWSTR sqlite3_win32_utf8_to_unicode(const char *);
#endif

#include "sqlite3.c"

/*
** Crypto algorithms
*/
#include "md5.c"
#include "sha1.c"
#include "sha2.c"
#include "fastpbkdf2.c"

/* Prototypes for several crypto functions to make pedantic compilers happy */
void chacha20_xor(unsigned char* data, size_t n, const unsigned char key[32], const unsigned char nonce[12], uint32_t counter);
void poly1305(const unsigned char* msg, size_t n, const unsigned char key[32], unsigned char tag[16]);
int poly1305_tagcmp(const unsigned char tag1[16], const unsigned char tag2[16]);
void chacha20_rng(void* out, size_t n);

#include "chacha20poly1305.c"

#ifdef SQLITE_USER_AUTHENTICATION
#include "sqlite3userauth.h"
#include "userauth.c"
#endif

#if defined(SQLITE_HAS_CODEC)      || \
    defined(SQLITE_ENABLE_EXTFUNC) || \
    defined(SQLITE_ENABLE_CSV)     || \
    defined(SQLITE_ENABLE_SHA3)    || \
    defined(SQLITE_ENABLE_CARRAY)  || \
    defined(SQLITE_ENABLE_FILEIO)  || \
    defined(SQLITE_ENABLE_SERIES)
#undef sqlite3_open
#undef sqlite3_open16
#undef sqlite3_open_v2
#endif

#ifndef SQLITE_OMIT_DISKIO
#ifdef SQLITE_HAS_CODEC

/*
** Get the codec argument for this pager
*/
static void*
mySqlite3PagerGetCodec(Pager *pPager)
{
#if (SQLITE_VERSION_NUMBER >= 3006016)
  return sqlite3PagerGetCodec(pPager);
#else
  return (pPager->xCodec) ? pPager->pCodecArg : NULL;
#endif
}

/*
** Set the codec argument for this pager
*/
static void
mySqlite3PagerSetCodec(Pager *pPager,
                       void *(*xCodec)(void*,void*,Pgno,int),
                       void (*xCodecSizeChng)(void*,int,int),
                       void (*xCodecFree)(void*),
                       void *pCodec)
{
  sqlite3PagerSetCodec(pPager, xCodec, xCodecSizeChng, xCodecFree, pCodec);
}

/*
** Declare function prototype for registering the codec extension functions
*/
static int
registerCodecExtensions(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

/*
** Codec implementation
*/
#include "rijndael.c"
#include "codec.c"
#include "codecext.c"

#endif /* SQLITE_HAS_CODEC */
#endif /* SQLITE_OMIT_DISKIO */

/*
** Extension functions
*/
#ifdef SQLITE_ENABLE_EXTFUNC
/* Prototype for initialization function of EXTENSIONFUNCTIONS extension */
int RegisterExtensionFunctions(sqlite3 *db);
#include "extensionfunctions.c"
#endif

/*
** CSV import
*/
#ifdef SQLITE_ENABLE_CSV
/* Prototype for initialization function of CSV extension */
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_csv_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);
#include "csv.c"
#endif

/*
** SHA3
*/
#ifdef SQLITE_ENABLE_SHA3
/* Prototype for initialization function of SHA3 extension */
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_shathree_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);
#include "shathree.c"
#endif

/*
** CARRAY
*/
#ifdef SQLITE_ENABLE_CARRAY
/* Prototype for initialization function of CARRAY extension */
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_carray_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);
#include "carray.c"
#endif

/*
** FILEIO
*/
#ifdef SQLITE_ENABLE_FILEIO
/* Prototype for initialization function of FILEIO extension */
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_fileio_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

/* MinGW specifics */
#if (!defined(_WIN32) && !defined(WIN32)) || defined(__MINGW32__)
# include <unistd.h>
# include <dirent.h>
# if defined(__MINGW32__)
#  define DIRENT dirent
#  ifndef S_ISLNK
#   define S_ISLNK(mode) (0)
#  endif
# endif
#endif

#include "test_windirent.c"
#include "fileio.c"
#endif

/*
** SERIES
*/
#ifdef SQLITE_ENABLE_SERIES
/* Prototype for initialization function of SERIES extension */
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_series_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);
#include "series.c"
#endif

#if defined(SQLITE_HAS_CODEC)      || \
    defined(SQLITE_ENABLE_EXTFUNC) || \
    defined(SQLITE_ENABLE_CSV)     || \
    defined(SQLITE_ENABLE_SHA3)    || \
    defined(SQLITE_ENABLE_CARRAY)  || \
    defined(SQLITE_ENABLE_FILEIO)  || \
    defined(SQLITE_ENABLE_SERIES)

static int
registerCodecExtensions(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
  int rc = SQLITE_OK;
  CodecParameter* codecParameterTable = NULL;

  if (sqlite3FindFunction(db, "wxsqlite3_config_table", 0, SQLITE_UTF8, 0) != NULL)
  {
    /* Return if codec extension functions are already defined */
    return rc;
  }

  codecParameterTable = CloneCodecParameterTable();
  rc = (codecParameterTable != NULL) ? SQLITE_OK : SQLITE_NOMEM;
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_create_function_v2(db, "wxsqlite3_config_table", 0, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                    codecParameterTable, wxsqlite3_config_table, 0, 0, (void(*)(void*)) FreeCodecParameterTable);
  }
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_create_function(db, "wxsqlite3_config", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                 codecParameterTable, wxsqlite3_config_params, 0, 0);
  }
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_create_function(db, "wxsqlite3_config", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                 codecParameterTable, wxsqlite3_config_params, 0, 0);
  }
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_create_function(db, "wxsqlite3_config", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                 codecParameterTable, wxsqlite3_config_params, 0, 0);
  }
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_create_function(db, "wxsqlite3_codec_data", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                 NULL, wxsqlite3_codec_data_sql, 0, 0);
  }
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_create_function(db, "wxsqlite3_codec_data", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                 NULL, wxsqlite3_codec_data_sql, 0, 0);
  }
  return rc;
}

static int
registerAllExtensions(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
  int rc = SQLITE_OK;
#ifdef SQLITE_HAS_CODEC
  /*
  ** Register the encryption extension functions and
  ** configure the encryption extension from URI parameters as default
  */
  rc = CodecConfigureFromUri(db, NULL, 1);
#endif
#ifdef SQLITE_ENABLE_EXTFUNC
  if (rc == SQLITE_OK)
  {
    rc = RegisterExtensionFunctions(db);
  }
#endif
#ifdef SQLITE_ENABLE_CSV
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_csv_init(db, NULL, NULL);
  }
#endif
#ifdef SQLITE_ENABLE_SHA3
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_shathree_init(db, NULL, NULL);
  }
#endif
#ifdef SQLITE_ENABLE_CARRAY
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_carray_init(db, NULL, NULL);
  }
#endif
#ifdef SQLITE_ENABLE_FILEIO
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_fileio_init(db, NULL, NULL);
  }
#endif
#ifdef SQLITE_ENABLE_SERIES
  if (rc == SQLITE_OK)
  {
    rc = sqlite3_series_init(db, NULL, NULL);
  }
#endif
  return rc;
}

/* Prototypes for sqlite3_open function variants to make pedantic compilers happy */
SQLITE_API int sqlite3_open(const char *filename, sqlite3 **ppDb);
SQLITE_API int sqlite3_open16(const void *filename, sqlite3 **ppDb);
SQLITE_API int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs);

SQLITE_API int sqlite3_open(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
)
{
  int ret = sqlite3_open_internal(filename, ppDb);
  if (ret == 0)
  {
    ret = registerAllExtensions(*ppDb, NULL, NULL);
  }
  return ret;
}

SQLITE_API int sqlite3_open16(
  const void *filename,   /* Database filename (UTF-16) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
)
{
  int ret = sqlite3_open16_internal(filename, ppDb);
  if (ret == 0)
  {
    ret = registerAllExtensions(*ppDb, NULL, NULL);
  }
  return ret;
}

SQLITE_API int sqlite3_open_v2(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb,         /* OUT: SQLite db handle */
  int flags,              /* Flags */
  const char *zVfs        /* Name of VFS module to use */
)
{
  int ret = sqlite3_open_v2_internal(filename, ppDb, flags, zVfs);
  if (ret == 0)
  {
    ret = registerAllExtensions(*ppDb, NULL, NULL);
  }
  return ret;
}

#endif
