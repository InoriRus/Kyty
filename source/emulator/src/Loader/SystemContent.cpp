#include "Emulator/Loader/SystemContent.h"

#include "Kyty/Core/ByteBuffer.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/Singleton.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Graphics/Image.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Loader {

class Psf
{
public:
	Psf() = default;
	virtual ~Psf();

	KYTY_CLASS_NO_COPY(Psf);

	void Open(const String& file_name);
	void Close();

	bool IsValid();

	bool GetParamInt(const char* name, int32_t* value);
	bool GetParamString(const char* name, String* value);
	bool GetParamString(const char* name, char* value, size_t value_size);

	void DbgPrint();

private:
	struct ParamInfo
	{
		uint16_t name_offset;
		uint16_t type;
		uint32_t size1;
		uint32_t size2;
		uint32_t value_offset;
	};

	Core::File  m_f;
	Core::Mutex m_mutex;
	bool        m_opened           = false;
	uint32_t    m_name_tbl_offset  = 0;
	uint32_t    m_value_tbl_offset = 0;
	uint32_t    m_params_num       = 0;
	ParamInfo*  m_params           = nullptr;
	char*       m_name_tbl         = nullptr;
};

class PlayGo
{
public:
	PlayGo() = default;
	virtual ~PlayGo();

	KYTY_CLASS_NO_COPY(PlayGo);

	void Open(const String& file_name);
	void Close();

	bool IsValid();

	bool GetChunksNum(uint32_t* num);

	void DbgPrint();

private:
	Core::File  m_f;
	Core::Mutex m_mutex;
	bool        m_opened     = false;
	uint16_t    m_chunks_num = 0;
};

struct SystemContent
{
	String                 psf_path;
	Psf                    psf;
	String                 playgo_path;
	PlayGo                 playgo;
	String                 icon_path;
	Libs::Graphics::Image* icon = nullptr;
};

Psf::~Psf()
{
	Close();
}

void Psf::Open(const String& file_name)
{
	Core::LockGuard lock(m_mutex);

	Close();

	m_f.Open(file_name, Core::File::Mode::Read);

	if (m_f.IsInvalid())
	{
		printf("Can't open %s\n", file_name.C_Str());
		return;
	}

	uint32_t magic1 = 0;
	uint32_t magic2 = 0;

	m_f.Read(&magic1, 4);
	m_f.Read(&magic2, 4);

	if (magic1 != 0x46535000 && magic2 != 0x00000101)
	{
		printf("invalid file: magic1 = %08" PRIx32 ", magic2 = %08" PRIx32 "\n", magic1, magic2);
		return;
	}

	m_f.Read(&m_name_tbl_offset, 4);
	m_f.Read(&m_value_tbl_offset, 4);
	m_f.Read(&m_params_num, 4);

	EXIT_NOT_IMPLEMENTED(m_name_tbl_offset == 0);
	EXIT_NOT_IMPLEMENTED(m_value_tbl_offset == 0);
	EXIT_NOT_IMPLEMENTED(m_value_tbl_offset <= m_name_tbl_offset);
	EXIT_NOT_IMPLEMENTED(m_params_num == 0);

	uint32_t name_tbl_size = m_value_tbl_offset - m_name_tbl_offset;

	m_params   = new ParamInfo[m_params_num];
	m_name_tbl = new char[name_tbl_size];

	for (uint32_t i = 0; i < m_params_num; i++)
	{
		m_f.Read(&m_params[i].name_offset, 2);
		m_f.Read(&m_params[i].type, 2);
		m_f.Read(&m_params[i].size1, 4);
		m_f.Read(&m_params[i].size2, 4);
		m_f.Read(&m_params[i].value_offset, 4);

		if (m_params[i].type != 0x0204 && m_params[i].type != 0x0404)
		{
			printf("unknown type %02" PRIx16 "\n", m_params[i].type);
			return;
		}
	}

	m_f.Seek(m_name_tbl_offset);
	m_f.Read(m_name_tbl, name_tbl_size);

	m_opened = true;
}

void Psf::Close()
{
	Core::LockGuard lock(m_mutex);

	if (!m_f.IsInvalid())
	{
		m_f.Close();
	}
	delete[] m_params;
	delete[] m_name_tbl;
	m_name_tbl = nullptr;
	m_params   = nullptr;
	m_opened   = false;
}

bool Psf::IsValid()
{
	Core::LockGuard lock(m_mutex);

	return m_opened;
}

void Psf::DbgPrint()
{
	Core::LockGuard lock(m_mutex);

	if (m_opened)
	{
		for (uint32_t i = 0; i < m_params_num; i++)
		{
			printf("%s, ", m_name_tbl + m_params[i].name_offset);

			switch (m_params[i].type)
			{
				case 0x0204:
				{
					m_f.Seek(m_value_tbl_offset + m_params[i].value_offset);
					auto buf = m_f.Read(m_params[i].size1);
					auto str = String::FromUtf8(reinterpret_cast<const char*>(buf.GetDataConst())).ReplaceChar(U'\n', U',');
					printf("string = %s\n", str.C_Str());
					break;
				}
				case 0x0404:
				{
					uint32_t value = 0;
					m_f.Seek(m_value_tbl_offset + m_params[i].value_offset);
					m_f.Read(&value, 4);
					printf("int = 0x%08" PRIx32 "\n", value);
					break;
				}
				default: EXIT("unknown type %02" PRIx16 "\n", m_params[i].type);
			}
		}
	}
}

bool Psf::GetParamInt(const char* name, int32_t* value)
{
	Core::LockGuard lock(m_mutex);

	if (m_opened)
	{
		for (uint32_t i = 0; i < m_params_num; i++)
		{
			if (strcmp(name, m_name_tbl + m_params[i].name_offset) == 0 && m_params[i].type == 0x0404 && m_params[i].size1 == 4)
			{
				m_f.Seek(m_value_tbl_offset + m_params[i].value_offset);
				m_f.Read(value, 4);

				return true;
			}
		}
	}

	return false;
}

bool Psf::GetParamString(const char* name, String* value)
{
	Core::LockGuard lock(m_mutex);

	if (m_opened)
	{
		for (uint32_t i = 0; i < m_params_num; i++)
		{
			if (strcmp(name, m_name_tbl + m_params[i].name_offset) == 0 && m_params[i].type == 0x0204)
			{
				m_f.Seek(m_value_tbl_offset + m_params[i].value_offset);

				auto buf = m_f.Read(m_params[i].size1);
				*value   = String::FromUtf8(reinterpret_cast<const char*>(buf.GetDataConst()));

				return true;
			}
		}
	}

	return false;
}

bool Psf::GetParamString(const char* name, char* value, size_t value_size)
{
	Core::LockGuard lock(m_mutex);

	if (m_opened)
	{
		for (uint32_t i = 0; i < m_params_num; i++)
		{
			if (strcmp(name, m_name_tbl + m_params[i].name_offset) == 0 && m_params[i].type == 0x0204 && m_params[i].size1 <= value_size)
			{
				m_f.Seek(m_value_tbl_offset + m_params[i].value_offset);
				m_f.Read(value, m_params[i].size1);

				return true;
			}
		}
	}

	return false;
}

PlayGo::~PlayGo()
{
	Close();
}

void PlayGo::Open(const String& file_name)
{
	Core::LockGuard lock(m_mutex);

	Close();

	m_f.Open(file_name, Core::File::Mode::Read);

	if (m_f.IsInvalid())
	{
		printf("Can't open %s\n", file_name.C_Str());
		return;
	}

	uint32_t magic1 = 0;

	m_f.Read(&magic1, 4);

	if (magic1 != 0x6f676c70)
	{
		printf("invalid file: magic1 = %08" PRIx32 "\n", magic1);
		return;
	}

	m_f.Seek(10);
	m_f.Read(&m_chunks_num, 2);

	m_opened = true;
}

void PlayGo::Close()
{
	Core::LockGuard lock(m_mutex);

	if (!m_f.IsInvalid())
	{
		m_f.Close();
	}
	m_opened = false;
}

bool PlayGo::IsValid()
{
	Core::LockGuard lock(m_mutex);

	return m_opened;
}

bool PlayGo::GetChunksNum(uint32_t* num)
{
	Core::LockGuard lock(m_mutex);

	if (m_opened)
	{
		*num = m_chunks_num;
		return true;
	}
	return false;
}

void PlayGo::DbgPrint()
{
	Core::LockGuard lock(m_mutex);

	if (m_opened)
	{
		printf("PlayGo: chunks num = %" PRIu16 "\n", m_chunks_num);
	}
}

void SystemContentLoadParamSfo(const String& file_name)
{
	auto* sc = Core::Singleton<SystemContent>::Instance();

	if (!Core::File::IsFileExisting(file_name))
	{
		EXIT("Can't find file: %s\n", file_name.C_Str());
	}

	sc->psf.Open(file_name);

	if (!sc->psf.IsValid())
	{
		EXIT("invalid file: %s\n", file_name.C_Str());
	}

	sc->psf_path = file_name;
	sc->psf.DbgPrint();

	sc->icon_path = file_name.DirectoryWithoutFilename() + U"icon0.png";
	delete sc->icon;

	if (Core::File::IsFileExisting(sc->icon_path))
	{
		sc->icon = new Libs::Graphics::Image(sc->icon_path);
		sc->icon->Load();
	} else
	{
		sc->icon = nullptr;
	}

	sc->playgo_path = file_name.DirectoryWithoutFilename() + U"playgo-chunk.dat";
	sc->playgo.Open(sc->playgo_path);

	if (sc->playgo.IsValid())
	{
		sc->playgo.DbgPrint();
	}
}

bool SystemContentParamSfoGetInt(const char* name, int32_t* value)
{
	if (name == nullptr || value == nullptr)
	{
		return false;
	}

	auto* sc = Core::Singleton<SystemContent>::Instance();

	return sc->psf.GetParamInt(name, value);
}

bool SystemContentParamSfoGetString(const char* name, String* value)
{
	if (name == nullptr || value == nullptr)
	{
		return false;
	}

	auto* sc = Core::Singleton<SystemContent>::Instance();

	return sc->psf.GetParamString(name, value);
}

bool SystemContentParamSfoGetString(const char* name, char* value, size_t value_size)
{
	if (name == nullptr || value == nullptr)
	{
		return false;
	}

	auto* sc = Core::Singleton<SystemContent>::Instance();

	return sc->psf.GetParamString(name, value, value_size);
}

Libs::Graphics::Image* SystemContentGetIcon()
{
	auto* sc = Core::Singleton<SystemContent>::Instance();

	return sc->icon;
}

bool SystemContentGetChunksNum(uint32_t* num)
{
	if (num == nullptr)
	{
		return false;
	}

	auto* sc = Core::Singleton<SystemContent>::Instance();

	return sc->playgo.GetChunksNum(num);
}

} // namespace Kyty::Loader

#endif // KYTY_EMU_ENABLED
