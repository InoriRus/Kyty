#ifndef INCLUDE_KYTY_CORE_DATABASE_H_
#define INCLUDE_KYTY_CORE_DATABASE_H_

#include "Kyty/Core/ByteBuffer.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Vector.h"

namespace Kyty::Core::Database {

struct StatementPrivate;
struct ConnectionPrivate;

void Init();

class Statement final
{
public:
	enum class State
	{
		Done,
		Row,
		Error
	};
	enum class Type
	{
		Integer,
		Float,
		Text,
		Blob,
		Null
	};

	State Step();

	void Reset();

	void ClearBindings();
	void BindBlob(const char* name, const ByteBuffer& blob) { BindBlob(GetIndex(name), blob); }
	void BindBlob(const String& name, const ByteBuffer& blob) { BindBlob(GetIndex(name), blob); }
	void BindBlob(int index, const ByteBuffer& blob);
	void BindDouble(const char* name, double d) { BindDouble(GetIndex(name), d); }
	void BindDouble(const String& name, double d) { BindDouble(GetIndex(name), d); }
	void BindDouble(int index, double d);
	void BindInt(const char* name, int i) { BindInt(GetIndex(name), i); }
	void BindInt(const String& name, int i) { BindInt(GetIndex(name), i); }
	void BindInt(int index, int i);
	void BindInt64(const char* name, int64_t i) { BindInt64(GetIndex(name), i); }
	void BindInt64(const String& name, int64_t i) { BindInt64(GetIndex(name), i); }
	void BindInt64(int index, int64_t i);
	void BindString(const char* name, const String& str) { BindString(GetIndex(name), str); }
	void BindString(const String& name, const String& str) { BindString(GetIndex(name), str); }
	void BindString(int index, const String& str);
	void BindString(const char* name, const char* str) { BindString(GetIndex(name), str); }
	void BindString(const String& name, const char* str) { BindString(GetIndex(name), str); }
	void BindString(int index, const char* str);
	void BindNull(const char* name) { BindNull(GetIndex(name)); }
	void BindNull(const String& name) { BindNull(GetIndex(name)); }
	void BindNull(int index);
	int  GetIndex(const char* name);
	int  GetIndex(const String& name);

	int         GetColumnCount();
	const char* GetColumnName(int index);
	const char* GetColumnNameDatabase(int index);
	const char* GetColumnNameTable(int index);
	const char* GetColumnNameField(int index);
	ByteBuffer  GetColumnBlob(int index);
	double      GetColumnDouble(int index);
	int         GetColumnInt(int index);
	int64_t     GetColumnInt64(int index);
	String      GetColumnString(int index);
	Type        GetColumnType(int index);

	void DbgTest();

	KYTY_CLASS_NO_COPY(Statement);

	friend class Connection;

protected:
	~Statement();
	explicit Statement(ConnectionPrivate* c);

private:
	StatementPrivate* m_p;
};

class Connection
{
public:
	enum class Mode
	{
		ReadOnly,
		ReadWrite
	};
	struct ExecRow
	{
		StringList names;
		StringList values;
	};
	class ExecResult: public Vector<ExecRow>
	{
	public:
		using Vector<ExecRow>::Vector;
		using Vector<ExecRow>::At;
		[[nodiscard]] bool CheckSize(uint32_t columns_num, uint32_t rows_num) const
		{
			return Size() == rows_num && (rows_num != 0u ? At(0).values.Size() == columns_num : false);
		}
		[[nodiscard]] const String& At(uint32_t column, uint32_t row) const { return At(row).values.At(column); }
	};

	Connection();
	virtual ~Connection();

	bool CreateInMemory();
	bool Create(const String& file_name);
	bool Open(const String& file_name, Mode mode);
	void Close();

	void CopyTo(Connection* db);

	void                     SetPassword(const String& password, int legacy);
	[[nodiscard]] ByteBuffer GetSalt() const;
	[[nodiscard]] ByteBuffer GetKey() const;
	void                     SetKey(const ByteBuffer& key);

	[[nodiscard]] bool IsInvalid() const;

	[[nodiscard]] bool          IsError() const { return m_error; }
	[[nodiscard]] int           GetErrorCode() const { return m_error_code; }
	[[nodiscard]] const String& GetErrorMsg() const { return m_error_msg; }
	[[nodiscard]] int           GetExtErrorCode() const { return m_ext_error_code; }

	Statement* Prepare(const char* sql_text);
	Statement* Prepare(const String& sql_text) { return Prepare(sql_text.C_Str()); }
	void       CloseAllStatements();
	ExecResult Exec(const char* sql_text);
	ExecResult Exec(const String& sql_text) { return Exec(sql_text.C_Str()); }

	void BeginTransaction();
	void EndTransaction();
	void Vacuum();

	int64_t GetLastInsertRowid();
	int     Changes();

	KYTY_CLASS_NO_COPY(Connection);

	friend struct ConnectionPrivate;

private:
	bool   m_error {false};
	int    m_error_code {};
	int    m_ext_error_code {};
	String m_error_msg;

	ConnectionPrivate* m_p {nullptr};
};

} // namespace Kyty::Core::Database

#endif /* INCLUDE_KYTY_CORE_DATABASE_H_ */
