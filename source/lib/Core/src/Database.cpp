#include "Kyty/Core/Database.h"

#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Math/Rand.h"

#include "sqlite3_options.h"
#include "sqlite3secure.h"

// IWYU pragma: no_include "sqlite3.h"

extern "C" {
void wxsqlite3_codec_data_set(sqlite3* db, const char* z_db_name, const char* param_name, const unsigned char* data);
}

namespace Kyty::Core::Database {

using Math::Rand;

#define KYTY_SQL(x)                                                                                                                        \
	{                                                                                                                                      \
		int status = x;                                                                                                                    \
		m_p->SetError(status != SQLITE_OK);                                                                                                \
	}

struct StatementPrivate
{
	explicit StatementPrivate(ConnectionPrivate* p): m_p(p) {}

	void                           Prepare(const char* sql_text);
	[[nodiscard]] Statement::State Step() const;
	void                           Reset() const;
	void                           Finalize();
	void                           ClearBindings() const;
	void                           BindBlob(int index, const ByteBuffer& blob) const;
	void                           BindDouble(int index, double d) const;
	void                           BindInt(int index, int i) const;
	void                           BindInt64(int index, int64_t i) const;
	void                           BindString(int index, const String& str) const;
	void                           BindString(int index, const char* str) const;
	void                           BindNull(int index) const;
	int                            GetIndex(const char* name) const;

	void DbgTest() const;

	sqlite3_stmt*      p_stmt = nullptr;
	ConnectionPrivate* m_p    = nullptr;
	const char*        text   = nullptr;
};

struct ConnectionPrivate
{
	explicit ConnectionPrivate(Connection* ap): parent(ap) { Reset(); }
	void Reset();
	void SetError(bool error) const;
	void SetError(bool error, const String& msg) const;

	sqlite3*           db {nullptr};
	Connection*        parent;
	Vector<Statement*> statements;
};

static void errorLogCallback(void* /*pArg*/, int i_err_code, const char* z_msg)
{
	printf("sqlite: (%d) %s\n", i_err_code, z_msg);
}

#if SQLITE_OS_OTHER

constexpr int MAXPATHNAME             = 512;
constexpr int SQLITE_DEMOVFS_BUFFERSZ = 8192;

struct KytyFile
{
	sqlite3_file  base;          /* Base class. Must be first. */
	File*         fd;            /* File descriptor */
	char*         a_buffer;      /* Pointer to malloc'd buffer */
	int           n_buffer;      /* Valid bytes of data in zBuffer */
	sqlite3_int64 i_buffer_ofst; /* Offset in file of zBuffer[0] */
};

static int KytyDirectWrite(KytyFile* p, const void* z_buf, int i_amt, sqlite_int64 i_ofst)
{
	uint32_t n_write = 0; /* Return value from write() */

	if (!p->fd->Seek(i_ofst))
	{
		return SQLITE_IOERR_WRITE; // NOLINT(hicpp-signed-bitwise)
	}

	p->fd->Write(z_buf, i_amt, &n_write);
	if (static_cast<int>(n_write) != i_amt)
	{
		return SQLITE_IOERR_WRITE; // NOLINT(hicpp-signed-bitwise)
	}

	return SQLITE_OK;
}

static int KytyFlushBuffer(KytyFile* p)
{
	int rc = SQLITE_OK;
	if (p->n_buffer != 0)
	{
		rc          = KytyDirectWrite(p, p->a_buffer, p->n_buffer, p->i_buffer_ofst);
		p->n_buffer = 0;
	}
	return rc;
}

static int KytyClose(sqlite3_file* p_file)
{
	int   rc = 0;
	auto* p  = reinterpret_cast<KytyFile*>(p_file);
	rc       = KytyFlushBuffer(p);
	sqlite3_free(p->a_buffer);
	p->fd->Close();
	Delete(p->fd);
	return rc;
}

static int KytyRead(sqlite3_file* p_file, void* z_buf, int i_amt, sqlite_int64 i_ofst)
{
	auto*    p      = reinterpret_cast<KytyFile*>(p_file);
	uint32_t n_read = 0;

	if (int rc = KytyFlushBuffer(p); rc != SQLITE_OK)
	{
		return rc;
	}

	if (!p->fd->Seek(i_ofst))
	{
		return SQLITE_IOERR_READ; // NOLINT(hicpp-signed-bitwise)
	}
	p->fd->Read(z_buf, i_amt, &n_read);

	if (static_cast<int>(n_read) == i_amt)
	{
		return SQLITE_OK;
	}

	if (static_cast<int>(n_read) >= 0)
	{
		if (static_cast<int>(n_read) < i_amt)
		{
			memset(&(static_cast<char*>(z_buf))[n_read], 0, i_amt - n_read);
		}
		return SQLITE_IOERR_SHORT_READ; // NOLINT(hicpp-signed-bitwise)
	}

	return SQLITE_IOERR_READ; // NOLINT(hicpp-signed-bitwise)
}

static int KytyWrite(sqlite3_file* p_file, const void* z_buf, int i_amt, sqlite_int64 i_ofst)
{
	auto* p = reinterpret_cast<KytyFile*>(p_file);

	if (p->a_buffer != nullptr)
	{
		const char*   z = static_cast<const char*>(z_buf); /* Pointer to remaining data to write */
		int           n = i_amt;                           /* Number of bytes at z */
		sqlite3_int64 i = i_ofst;                          /* File offset to write to */

		while (n > 0)
		{
			int n_copy = 0; /* Number of bytes to copy into buffer */

			/* If the buffer is full, or if this data is not being written directly
			 ** following the data already buffered, flush the buffer. Flushing
			 ** the buffer is a no-op if it is empty.
			 */
			if (p->n_buffer == SQLITE_DEMOVFS_BUFFERSZ || p->i_buffer_ofst + p->n_buffer != i)
			{
				int rc = KytyFlushBuffer(p);
				if (rc != SQLITE_OK)
				{
					return rc;
				}
			}
			ASSERT(p->n_buffer == 0 || p->i_buffer_ofst + p->n_buffer == i);
			p->i_buffer_ofst = i - p->n_buffer;

			/* Copy as much data as possible into the buffer. */
			n_copy = SQLITE_DEMOVFS_BUFFERSZ - p->n_buffer;
			if (n_copy > n)
			{
				n_copy = n;
			}
			std::memcpy(&p->a_buffer[p->n_buffer], z, n_copy);
			p->n_buffer += n_copy;

			n -= n_copy;
			i += n_copy;
			z += n_copy;
		}
	} else
	{
		return KytyDirectWrite(p, z_buf, i_amt, i_ofst);
	}

	return SQLITE_OK;
}

static int KytyTruncate(sqlite3_file* p_file, sqlite_int64 size)
{
	auto* p = reinterpret_cast<KytyFile*>(p_file);

	if (int rc = KytyFlushBuffer(p); rc != SQLITE_OK)
	{
		return rc;
	}

	p->fd->Truncate(size);

	return SQLITE_OK;
}

static int KytySync(sqlite3_file* p_file, int /*flags*/)
{
	auto* p = reinterpret_cast<KytyFile*>(p_file);

	if (int rc = KytyFlushBuffer(p); rc != SQLITE_OK)
	{
		return rc;
	}

	return (p->fd->Flush() ? SQLITE_OK : SQLITE_IOERR_FSYNC); // NOLINT(hicpp-signed-bitwise)
}

static int KytyFileSize(sqlite3_file* p_file, sqlite_int64* p_size)
{
	auto* p = reinterpret_cast<KytyFile*>(p_file);

	if (int rc = KytyFlushBuffer(p); rc != SQLITE_OK)
	{
		return rc;
	}

	*p_size = static_cast<sqlite_int64>(p->fd->Size());
	return SQLITE_OK;
}

static int KytyLock(sqlite3_file* /*pFile*/, int /*eLock*/)
{
	return SQLITE_OK;
}

static int KytyUnlock(sqlite3_file* /*pFile*/, int /*eLock*/)
{
	return SQLITE_OK;
}

static int KytyCheckReservedLock(sqlite3_file* /*pFile*/, int* p_res_out)
{
	*p_res_out = 0;
	return SQLITE_OK;
}

static int KytyFileControl(sqlite3_file* /*pFile*/, int /*op*/, void* /*pArg*/)
{
	return SQLITE_NOTFOUND;
}

static int KytySectorSize(sqlite3_file* /*pFile*/)
{
	return 0;
}

static int KytyDeviceCharacteristics(sqlite3_file* /*pFile*/)
{
	return 0;
}

static int KytyOpen(sqlite3_vfs* /*pVfs*/, const char* z_name, sqlite3_file* p_file, int flags, int* p_out_flags)
{
	static const sqlite3_io_methods io = {
	    1,                        /* iVersion */
	    KytyClose,                /* xClose */
	    KytyRead,                 /* xRead */
	    KytyWrite,                /* xWrite */
	    KytyTruncate,             /* xTruncate */
	    KytySync,                 /* xSync */
	    KytyFileSize,             /* xFileSize */
	    KytyLock,                 /* xLock */
	    KytyUnlock,               /* xUnlock */
	    KytyCheckReservedLock,    /* xCheckReservedLock */
	    KytyFileControl,          /* xFileControl */
	    KytySectorSize,           /* xSectorSize */
	    KytyDeviceCharacteristics /* xDeviceCharacteristics */
	};

	auto* p     = reinterpret_cast<KytyFile*>(p_file);
	char* a_buf = nullptr;

	if (z_name == nullptr)
	{
		return SQLITE_IOERR;
	}

	if ((flags & SQLITE_OPEN_MAIN_JOURNAL) != 0) // NOLINT(hicpp-signed-bitwise)
	{
		a_buf = static_cast<char*>(sqlite3_malloc(SQLITE_DEMOVFS_BUFFERSZ));
		if (a_buf == nullptr)
		{
			return SQLITE_NOMEM;
		}
	}

	String file_name = String::FromUtf8(z_name);

	memset(p, 0, sizeof(KytyFile));
	p->fd = new File;

	if ((flags & 0xFF) == (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_EXCLUSIVE) || // NOLINT(hicpp-signed-bitwise)
	    (flags & 0xFF) == (SQLITE_OPEN_CREATE | SQLITE_OPEN_EXCLUSIVE))                           // NOLINT(hicpp-signed-bitwise)
	{
		if (!File::IsFileExisting(file_name))
		{
			p->fd->Create(file_name);
		}
	} else if ((flags & 0xFF) == (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)) // NOLINT(hicpp-signed-bitwise)
	{
		if (!File::IsFileExisting(file_name))
		{
			p->fd->Create(file_name);
		} else
		{
			p->fd->Open(file_name, File::Mode::ReadWrite);
		}
	} else if ((flags & 0xFF) == SQLITE_OPEN_READONLY) // NOLINT(hicpp-signed-bitwise)
	{
		p->fd->Open(file_name, File::Mode::Read);
	} else if ((flags & 0xFF) == SQLITE_OPEN_READWRITE) // NOLINT(hicpp-signed-bitwise)
	{
		p->fd->Open(file_name, File::Mode::ReadWrite);
	} else
	{
		printf("unknown flags: %d\n", flags);
	}

	if (p->fd->IsInvalid())
	{
		Delete(p->fd);
		sqlite3_free(a_buf);
		return SQLITE_CANTOPEN;
	}
	p->a_buffer = a_buf;

	if (p_out_flags != nullptr)
	{
		*p_out_flags = flags;
	}
	p->base.pMethods = &io;
	return SQLITE_OK;
}

static int KytyDelete(sqlite3_vfs* /*pVfs*/, const char* z_path, int /*dirSync*/)
{
	String file_name = String::FromUtf8(z_path);
	if (!File::DeleteFile(file_name))
	{
		return SQLITE_IOERR_DELETE; // NOLINT(hicpp-signed-bitwise)
	}
	if (File::IsFileExisting(file_name))
	{
		return SQLITE_IOERR_DELETE; // NOLINT(hicpp-signed-bitwise)
	}
	return SQLITE_OK;
}

static int KytyAccess(sqlite3_vfs* /*pVfs*/, const char* z_path, int flags, int* p_res_out)
{
	bool   ok        = false;
	String file_name = String::FromUtf8(z_path);

	switch (flags)
	{
		case SQLITE_ACCESS_READ:
		case SQLITE_ACCESS_EXISTS: ok = File::IsFileExisting(file_name); break;
		case SQLITE_ACCESS_READWRITE:
			if (File::IsFileExisting(file_name))
			{
				File f;
				f.Open(file_name, File::Mode::ReadWrite);
				ok = !f.IsInvalid();
				f.Close();
			}
			break;
		default: printf("Invalid flags: %d", flags);
	}

	*p_res_out = static_cast<int>(!!ok);

	return SQLITE_OK;
}

static int KytyFullPathname(sqlite3_vfs* /*pVfs*/, const char* z_path, int n_path_out, char* z_path_out)
{
	sqlite3_snprintf(n_path_out, z_path_out, "%s", z_path);
	z_path_out[n_path_out - 1] = '\0';
	return SQLITE_OK;
}

static void* KytyDlOpen(sqlite3_vfs* /*pVfs*/, const char* /*zPath*/)
{
	return nullptr;
}

static void KytyDlError(sqlite3_vfs* /*pVfs*/, int n_byte, char* z_err_msg)
{
	sqlite3_snprintf(n_byte, z_err_msg, "Loadable extensions are not supported");
	z_err_msg[n_byte - 1] = '\0';
}

static void (*KytyDlSym(sqlite3_vfs* /*pVfs*/, void* /*pH*/, const char* /*z*/))()
{
	return nullptr;
}

static void KytyDlClose(sqlite3_vfs* /*pVfs*/, void* /*pHandle*/)
{
	// return;
}

static int KytyRandomness(sqlite3_vfs* /*pVfs*/, int n_byte, char* z_byte)
{
	for (int i = 0; i < n_byte; i++)
	{
		z_byte[i] = static_cast<char>(Rand::Uint() & 0xFFu);
	}
	return SQLITE_OK;
}

static int KytySleep(sqlite3_vfs* /*pVfs*/, int n_micro)
{
	Thread::SleepMicro(n_micro);
	return n_micro;
}

static int KytyCurrentTime(sqlite3_vfs* /*pVfs*/, double* p_time)
{
	*p_time = DateTime::FromSystemUTC().ToSQLiteJulian();
	return SQLITE_OK;
}

static int KytyCurrentTimeInt64(sqlite3_vfs* /*pVfs*/, sqlite3_int64* pi_now)
{
	*pi_now = DateTime::FromSystemUTC().ToSQLiteJulianInt64();
	return SQLITE_OK;
}

static int KytyGetLastError(sqlite3_vfs* /*pVfs*/, int /*nBuf*/, char* /*zBuf*/)
{
	return 0;
}

static sqlite3_vfs* KytyVfs()
{
	static sqlite3_vfs vfs = {
	    2,                    /* iVersion */
	    sizeof(KytyFile),     /* szOsFile */
	    MAXPATHNAME,          /* mxPathname */
	    nullptr,              /* pNext */
	    "Kyty",               /* zName */
	    nullptr,              /* pAppData */
	    KytyOpen,             /* xOpen */
	    KytyDelete,           /* xDelete */
	    KytyAccess,           /* xAccess */
	    KytyFullPathname,     /* xFullPathname */
	    KytyDlOpen,           /* xDlOpen */
	    KytyDlError,          /* xDlError */
	    KytyDlSym,            /* xDlSym */
	    KytyDlClose,          /* xDlClose */
	    KytyRandomness,       /* xRandomness */
	    KytySleep,            /* xSleep */
	    KytyCurrentTime,      /* xCurrentTime */
	    KytyGetLastError,     /* xGetLastError */
	    KytyCurrentTimeInt64, /* xCurrentTimeInt64 */
	};
	return &vfs;
}

extern "C" {

int sqlite3_os_init()
{
	int ret = sqlite3_vfs_register(KytyVfs(), 1);
	return ret;
}

int sqlite3_os_end()
{
	return SQLITE_OK;
}
}

#endif

void Init()
{
	if (sqlite3_config(SQLITE_CONFIG_LOG, errorLogCallback, 0) != SQLITE_OK)
	{
		EXIT("sqlite3_config() failed\n");
	}

	if (sqlite3_initialize() != SQLITE_OK)
	{
		EXIT("sqlite3_initialize() failed\n");
	}

	if (sqlite3_threadsafe() == 0)
	{
		EXIT("sqlite3 is not multi-threaded\n");
	}
}

void StatementPrivate::Prepare(const char* sql_text)
{
	EXIT_IF(text || p_stmt);
	EXIT_IF(!sql_text);

	text = sql_text;

	KYTY_SQL(sqlite3_prepare_v2(m_p->db, sql_text, -1, &p_stmt, nullptr));

	if ((p_stmt == nullptr) && !m_p->parent->IsError())
	{
		m_p->SetError(true, U"sqlite3_prepare_v2 failed: !pStmt");
	}
}

void StatementPrivate::DbgTest() const
{
	int size = sqlite3_data_count(p_stmt);

	for (int i = 0; i < size; i++)
	{
		printf("database = %s, table = %s, origin = %s, as = %s\n", sqlite3_column_database_name(p_stmt, i),
		       sqlite3_column_table_name(p_stmt, i), sqlite3_column_origin_name(p_stmt, i), sqlite3_column_name(p_stmt, i));
	}
}

Statement::State StatementPrivate::Step() const
{
	EXIT_IF(!text);

	Statement::State state = Statement::State::Done;

	if (int status = sqlite3_step(p_stmt); status == SQLITE_ROW)
	{
		state = Statement::State::Row;
	} else if (status != SQLITE_DONE)
	{
		m_p->SetError(true /*, U"sqlite3_step failed"*/);
		state = Statement::State::Error;
	}

	return state;
}

void StatementPrivate::Reset() const
{
	EXIT_IF(!text);

	KYTY_SQL(sqlite3_reset(p_stmt));
}

void StatementPrivate::Finalize()
{
	EXIT_IF(!text);

	KYTY_SQL(sqlite3_finalize(p_stmt));

	p_stmt = nullptr;
	text   = nullptr;
}

void StatementPrivate::ClearBindings() const
{
	KYTY_SQL(sqlite3_clear_bindings(p_stmt));
}

void StatementPrivate::BindBlob(int index, const ByteBuffer& blob) const
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	KYTY_SQL(sqlite3_bind_blob(p_stmt, index, blob.GetDataConst(), blob.Size(), SQLITE_TRANSIENT));
}

void StatementPrivate::BindDouble(int index, double d) const
{
	KYTY_SQL(sqlite3_bind_double(p_stmt, index, d));
}

void StatementPrivate::BindInt(int index, int i) const
{
	KYTY_SQL(sqlite3_bind_int(p_stmt, index, i));
}

void StatementPrivate::BindInt64(int index, int64_t i) const
{
	KYTY_SQL(sqlite3_bind_int64(p_stmt, index, i));
}

void StatementPrivate::BindString(int index, const String& str) const
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	KYTY_SQL(sqlite3_bind_text(p_stmt, index, str.C_Str(), -1, SQLITE_TRANSIENT));
}

void StatementPrivate::BindString(int index, const char* str) const
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	KYTY_SQL(sqlite3_bind_text(p_stmt, index, str, -1, SQLITE_TRANSIENT));
}

void StatementPrivate::BindNull(int index) const
{
	KYTY_SQL(sqlite3_bind_null(p_stmt, index));
}

int StatementPrivate::GetIndex(const char* name) const
{
	return sqlite3_bind_parameter_index(p_stmt, name);
}

void ConnectionPrivate::SetError(bool error) const
{
	parent->m_error          = error;
	parent->m_error_code     = (error ? sqlite3_errcode(db) : SQLITE_OK);
	parent->m_ext_error_code = (error ? sqlite3_extended_errcode(db) : SQLITE_OK);
	parent->m_error_msg      = (error ? String::FromUtf8(sqlite3_errmsg(db)) : U"");
}

void ConnectionPrivate::SetError(bool error, const String& msg) const
{
	parent->m_error          = error;
	parent->m_error_code     = (error ? sqlite3_errcode(db) : SQLITE_OK);
	parent->m_ext_error_code = (error ? sqlite3_extended_errcode(db) : SQLITE_OK);
	parent->m_error_msg      = msg;
}

void ConnectionPrivate::Reset()
{
	db = nullptr;
	SetError(false);
}

Connection::Connection() // @suppress("Class members should be properly initialized")
    : m_p(new ConnectionPrivate(this))
{
	m_p->Reset();
}

Connection::~Connection()
{
	EXIT_IF(m_p->db);

	Delete(m_p);
}

void Connection::SetPassword(const String& password, int legacy)
{
	EXIT_IF(!m_p->db);

	if (int ret = wxsqlite3_config(m_p->db, "default:cipher", CODEC_TYPE_SQLCIPHER); ret != CODEC_TYPE_SQLCIPHER)
	{
		m_p->SetError(true, U"wxsqlite3_config failed");
	}

	if (int ret = wxsqlite3_config_cipher(m_p->db, "sqlcipher", "legacy", legacy); ret != legacy)
	{
		m_p->SetError(true, U"wxsqlite3_config_cipher failed");
	}

	String::Utf8 utf8 = password.utf8_str();

	KYTY_SQL(sqlite3_key(m_p->db, utf8.GetDataConst(), utf8.Size()));
}

Core::ByteBuffer Connection::GetSalt() const
{
	ByteBuffer ret(16);
	ret.Memset(0);
	auto* salt     = wxsqlite3_codec_data(m_p->db, nullptr, "raw:cipher_salt");
	auto* salt_ptr = salt;
	for (auto& b: ret)
	{
		b = static_cast<Byte>(/*21 ^ */ (*salt_ptr++));
	}
	sqlite3_free(salt);
	return ret;
}

ByteBuffer Connection::GetKey() const
{
	ByteBuffer ret(32);
	ret.Memset(0);
	auto* key     = wxsqlite3_codec_data(m_p->db, nullptr, "raw:cipher_key");
	auto* key_ptr = key;
	for (auto& b: ret)
	{
		b = static_cast<Byte>(/*21 ^ */ (*key_ptr++));
	}
	sqlite3_free(key);
	return ret;
}

void Connection::SetKey(const ByteBuffer& key)
{
	EXIT_IF(key.Size() != 32);

	wxsqlite3_codec_data_set(m_p->db, nullptr, "cipher_key", reinterpret_cast<const unsigned char*>(key.GetDataConst()));
}

bool Connection::CreateInMemory()
{
	EXIT_IF(m_p->db);

	KYTY_SQL(sqlite3_open_v2(":memory:", &m_p->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr)); // NOLINT(hicpp-signed-bitwise)

	if (IsError())
	{
		Close();
		return false;
	}

	return true;
}

bool Connection::Create(const String& file_name)
{
	EXIT_IF(m_p->db);

	if (file_name.IsEmpty())
	{
		return false;
	}

	if (File::IsFileExisting(file_name))
	{
		// if (!File::DeleteFile(file_name) || File::IsFileExisting(file_name))
		{
			return false;
		}
	}

	// NOLINTNEXTLINE(hicpp-signed-bitwise)
	KYTY_SQL(sqlite3_open_v2(file_name.C_Str(), &m_p->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr));

	if (IsError() || !File::IsFileExisting(file_name))
	{
		Close();
		return false;
	}

	if (IsError())
	{
		Close();
		return false;
	}

	return true;
}

bool Connection::Open(const String& file_name, Mode mode)
{
	EXIT_IF(m_p->db);

	if (file_name.IsEmpty())
	{
		return false;
	}

	if (!(File::IsFileExisting(file_name) || File::IsAssetFileExisting(file_name)))
	{
		return false;
	}

	int flags = 0;

	if (mode == Mode::ReadOnly)
	{
		flags = SQLITE_OPEN_READONLY;
	} else if (mode == Mode::ReadWrite)
	{
		flags = SQLITE_OPEN_READWRITE;
	}

	KYTY_SQL(sqlite3_open_v2(file_name.C_Str(), &m_p->db, flags, nullptr));

	if (IsError())
	{
		Close();
		return false;
	}

	return true;
}

void Connection::CopyTo(Connection* db)
{
	EXIT_IF(db == nullptr);

	sqlite3_backup* p_backup = sqlite3_backup_init(db->m_p->db, "main", m_p->db, "main");

	if (p_backup != nullptr)
	{
		KYTY_SQL(sqlite3_backup_step(p_backup, -1));
		KYTY_SQL(sqlite3_backup_finish(p_backup));
	} else
	{
		m_p->SetError(true, U"sqlite3_backup_init() failed");
	}
}

void Connection::Close()
{
	CloseAllStatements();

	if (m_p->db != nullptr)
	{
		[[maybe_unused]] int status = sqlite3_close(m_p->db);
		EXIT_IF(status != SQLITE_OK);
		m_p->Reset();
	}
}

bool Connection::IsInvalid() const
{
	return m_p->db == nullptr;
}

Statement* Connection::Prepare(const char* sql_text)
{
	EXIT_IF(!m_p->db);

	auto* s = new Statement(m_p);

	s->m_p->Prepare(sql_text);

	m_p->statements.Add(s);

	// printf("prepare\n");

	return s;
}

static int exec_callback(void* data, int count, char** values, char** names)
{
	auto* r = static_cast<Connection::ExecResult*>(data);

	Connection::ExecRow row;

	for (int i = 0; i < count; i++)
	{
		row.names.Add(String::FromUtf8(names[i]));
		row.values.Add(String::FromUtf8(values[i]));
	}

	r->Add(row);

	return SQLITE_OK;
}

Connection::ExecResult Connection::Exec(const char* sql_text)
{
	ExecResult ret;
	char*      err = nullptr;

	KYTY_SQL(sqlite3_exec(m_p->db, sql_text, exec_callback, &ret, &err));

	if (err != nullptr)
	{
		m_p->SetError(true, String::FromUtf8(err));
		sqlite3_free(err);
	}

	return ret;
}

Statement::Statement(ConnectionPrivate* c): m_p(new StatementPrivate(c)) {}

Statement::~Statement()
{
	m_p->Finalize();
	Delete(m_p);
}

Statement::State Statement::Step()
{
	return m_p->Step();
}

void Statement::Reset()
{
	m_p->Reset();
}

void Statement::ClearBindings()
{
	m_p->ClearBindings();
}

void Statement::BindBlob(int index, const ByteBuffer& blob)
{
	m_p->BindBlob(index, blob);
}

void Statement::BindDouble(int index, double d)
{
	m_p->BindDouble(index, d);
}

void Statement::BindInt(int index, int i)
{
	m_p->BindInt(index, i);
}

void Statement::BindInt64(int index, int64_t i)
{
	m_p->BindInt64(index, i);
}

void Statement::BindString(int index, const String& str)
{
	m_p->BindString(index, str);
}

void Statement::BindString(int index, const char* str)
{
	m_p->BindString(index, str);
}

void Statement::BindNull(int index)
{
	m_p->BindNull(index);
}

int Statement::GetIndex(const char* name)
{
	return m_p->GetIndex(name);
}

int Statement::GetIndex(const String& name)
{
	return m_p->GetIndex(name.C_Str());
}

void Statement::DbgTest()
{
	m_p->DbgTest();
}

int Statement::GetColumnCount()
{
	return sqlite3_column_count(m_p->p_stmt);
}

const char* Statement::GetColumnName(int index)
{
	return sqlite3_column_name(m_p->p_stmt, index);
}

const char* Statement::GetColumnNameDatabase(int index)
{
	return sqlite3_column_database_name(m_p->p_stmt, index);
}

const char* Statement::GetColumnNameTable(int index)
{
	return sqlite3_column_table_name(m_p->p_stmt, index);
}

const char* Statement::GetColumnNameField(int index)
{
	return sqlite3_column_origin_name(m_p->p_stmt, index);
}

ByteBuffer Statement::GetColumnBlob(int index)
{
	return ByteBuffer(sqlite3_column_blob(m_p->p_stmt, index), sqlite3_column_bytes(m_p->p_stmt, index));
}

double Statement::GetColumnDouble(int index)
{
	return sqlite3_column_double(m_p->p_stmt, index);
}

int Statement::GetColumnInt(int index)
{
	return sqlite3_column_int(m_p->p_stmt, index);
}

int64_t Statement::GetColumnInt64(int index)
{
	return sqlite3_column_int64(m_p->p_stmt, index);
}

String Statement::GetColumnString(int index)
{
	return String::FromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(m_p->p_stmt, index)));
}

Statement::Type Statement::GetColumnType(int index)
{
	switch (sqlite3_column_type(m_p->p_stmt, index))
	{
		case SQLITE_INTEGER: return Type::Integer;
		case SQLITE_FLOAT: return Type::Float;
		case SQLITE_BLOB: return Type::Blob;
		case SQLITE_TEXT: return Type::Text;

		case SQLITE_NULL:
		default: return Type::Null;
	}
}

int64_t Connection::GetLastInsertRowid()
{
	return sqlite3_last_insert_rowid(m_p->db);
}

int Connection::Changes()
{
	return sqlite3_changes(m_p->db);
}

void Connection::BeginTransaction()
{
	Exec("BEGIN TRANSACTION");
}

void Connection::EndTransaction()
{
	Exec("END TRANSACTION");
}

void Connection::Vacuum()
{
	Exec("VACUUM");
}

void Connection::CloseAllStatements()
{
	if (m_p->db != nullptr)
	{
		for (auto& s: m_p->statements)
		{
			delete s;
			s = nullptr;
		}
	}
}

} // namespace Kyty::Core::Database
