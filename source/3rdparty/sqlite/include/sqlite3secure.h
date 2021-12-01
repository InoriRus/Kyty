/*
** Name:        sqlite3secure.h
** Purpose:     Header file for SQLite codecs
** Author:      Ulrich Telle
** Created:     2018-03-30
** Copyright:   (c) 2018 Ulrich Telle
** License:     LGPL-3.0+ WITH WxWindows-exception-3.1
*/

#ifndef SQLITE3SECURE_H_
#define SQLITE3SECURE_H_

#include "sqlite3.h"

// Symbols for ciphers
#define CODEC_TYPE_UNKNOWN   0
#define CODEC_TYPE_AES128    1
#define CODEC_TYPE_AES256    2
#define CODEC_TYPE_CHACHA20  3
#define CODEC_TYPE_SQLCIPHER 4
#define CODEC_TYPE_MAX       4

// Define Windows specific SQLite API functions (not defined in sqlite3.h)
#if SQLITE_VERSION_NUMBER >= 3007014
#if SQLITE_OS_WIN == 1
#ifdef __cplusplus
extern "C" {
#endif
#if SQLITE_VERSION_NUMBER >= 3024000
SQLITE_API int sqlite3_win32_set_directory(unsigned long type, void* zValue);
#else
SQLITE_API int sqlite3_win32_set_directory(DWORD type, LPCWSTR zValue);
#endif
#ifdef __cplusplus
}
#endif
#endif
#endif

#ifdef SQLITE_HAS_CODEC

// Define functions for the configuration of the wxSQLite3 encryption extension
#ifdef __cplusplus
extern "C" {
#endif
SQLITE_API int wxsqlite3_config(sqlite3* db, const char* paramName, int newValue);
SQLITE_API int wxsqlite3_config_cipher(sqlite3* db, const char* cipherName, const char* paramName, int newValue);
SQLITE_API unsigned char* wxsqlite3_codec_data(sqlite3* db, const char* zDbName, const char* paramName);
#ifdef __cplusplus
}

#endif

#endif

#endif
