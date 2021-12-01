#ifndef _3RDPARTY_SQLITE_INCLUDE_SQLITE3_OPTIONS_H_
#define _3RDPARTY_SQLITE_INCLUDE_SQLITE3_OPTIONS_H_

#define SQLITE_HAS_CODEC 1
#define SQLITE_ENABLE_COLUMN_METADATA 1 /* Access to meta-data about tables and queries */
#define SQLITE_THREADSAFE             1 /* In serialized mode, SQLite can be safely used by multiple threads with no restriction. */
#define SQLITE_TEMP_STORE             3 /* Always use memory */
#define SQLITE_OS_OTHER               1 /* Omit its built-in operating system interfaces */
#define SQLITE_OMIT_AUTOINIT          1 /* SQLite will not automatically initialize itself */

#endif /* _3RDPARTY_SQLITE_INCLUDE_SQLITE3_OPTIONS_H_ */
