#ifndef EMULATOR_INCLUDE_EMULATOR_LOADER_SYMBOLDATABASE_H_
#define EMULATOR_INCLUDE_EMULATOR_LOADER_SYMBOLDATABASE_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Loader {

enum class SymbolType
{
	Unknown,
	Func,
	Object,
	TlsModule,
	NoType,
};

struct SymbolRecord
{
	String   name;
	String   dbg_name;
	uint64_t vaddr;
};

struct SymbolResolve
{
	String     name;
	String     library;
	int        library_version;
	String     module;
	int        module_version_major;
	int        module_version_minor;
	SymbolType type;
};

class SymbolDatabase
{
public:
	SymbolDatabase()          = default;
	virtual ~SymbolDatabase() = default;

	void Add(const SymbolResolve& s, uint64_t vaddr);
	void Add(const SymbolResolve& s, uint64_t vaddr, const String& dbg_name);

	[[nodiscard]] const SymbolRecord* Find(const SymbolResolve& s) const;

	void DbgDump(const String& folder, const String& file_name);

	KYTY_CLASS_NO_COPY(SymbolDatabase);

	static String GenerateName(const SymbolResolve& s);

private:
	Vector<SymbolRecord>            m_symbols;
	Core::Hashmap<String, uint32_t> m_map;
};

} // namespace Kyty::Loader

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LOADER_SYMBOLDATABASE_H_ */
