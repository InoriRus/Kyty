#ifndef INCLUDE_KYTY_CORE_SUBSYSTEMS_H_
#define INCLUDE_KYTY_CORE_SUBSYSTEMS_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Singleton.h"

#include <initializer_list>

namespace Kyty::Core {

class Subsystem;
class SubsystemsListPrivate;
class SubsystemPrivate;

class SubsystemsList
{
public:
	SubsystemsList();
	virtual ~SubsystemsList();
	void SetArgs(int argc, char* argv[]);

	// void Add(Subsystem* s, const char* name, ...);
	void Add(Subsystem* s, std::initializer_list<Subsystem*> deps);

	bool InitAll(bool print_msg = false);
	void DestroyAll(bool print_msg = false);

	int*   GetArgc();
	char** GetArgv();

	[[nodiscard]] const char* GetFailName() const;
	[[nodiscard]] const char* GetFailMsg() const;

	void ShutdownAll();

	static SubsystemsList* Instance() { return Core::Singleton<SubsystemsList>::Instance(); }

	KYTY_CLASS_NO_COPY(SubsystemsList);

private:
	SubsystemsListPrivate* m_p;
};

using SubsystemsListSingleton = Kyty::Core::Singleton<SubsystemsList>;

class Subsystem
{
public:
	Subsystem();
	virtual ~Subsystem();

	virtual const char* Id()                                       = 0;
	virtual void        Init(SubsystemsList* parent)               = 0;
	virtual void        Destroy(SubsystemsList* parent)            = 0;
	virtual void        UnexpectedShutdown(SubsystemsList* parent) = 0;

	friend class SubsystemsListPrivate;

	KYTY_CLASS_NO_COPY(Subsystem);

protected:
	void Fail(const char* format, ...) KYTY_FORMAT_PRINTF(2, 3);

private:
	SubsystemPrivate* m_p;
};

#define KYTY_SUBSYSTEM_DEFINE(s)                                                                                                           \
	class s##Subsystem: public Core::Subsystem                                                                                             \
	{                                                                                                                                      \
	public:                                                                                                                                \
		static Subsystem* Instance() { return Core::Singleton<s##Subsystem>::Instance(); }                                                 \
		const char*       Id() { return #s; }                                                                                              \
		void              Init(Core::SubsystemsList* parent);                                                                              \
		void              Destroy(Core::SubsystemsList* parent);                                                                           \
		void              UnexpectedShutdown(Core::SubsystemsList* parent);                                                                \
	};                                                                                                                                     \
	typedef Core::Singleton<s##Subsystem> s##SubsystemSingleton;

#define KYTY_SUBSYSTEM_INIT(s)                void s##Subsystem::Init([[maybe_unused]] Core::SubsystemsList* parent)
#define KYTY_SUBSYSTEM_DESTROY(s)             void s##Subsystem::Destroy([[maybe_unused]] Core::SubsystemsList* parent)
#define KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(s) void s##Subsystem::UnexpectedShutdown([[maybe_unused]] Core::SubsystemsList* parent)

#define KYTY_SUBSYSTEM_ARGC parent->GetArgc()
#define KYTY_SUBSYSTEM_ARGV parent->GetArgv()

#if KYTY_COMPILER == KYTY_COMPILER_MSVC
#define KYTY_SUBSYSTEM_FAIL(f, ...)                                                                                                        \
	this->Fail(f, __VA_ARGS__);                                                                                                            \
	return;
#else
#define KYTY_SUBSYSTEM_FAIL(f, s...)                                                                                                       \
	this->Fail(f, ##s);                                                                                                                    \
	return;
#endif

//#define KYTY_SUBSYSTEM_ADD(list, s, ...) list.Add(s##SubsystemSingleton::Instance(), #s, __VA_ARGS__);
//#define KYTY_SUBSYSTEM_ADD2(list, ...)   list.Add2(s##SubsystemSingleton::Instance(), __VA_ARGS__);

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_SUBSYSTEMS_H_ */
