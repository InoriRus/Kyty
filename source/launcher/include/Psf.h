#ifndef LAUNCHER_INCLUDE_PSF_H_
#define LAUNCHER_INCLUDE_PSF_H_

#include "Common.h"

#include <QFile>
#include <QString>

class Psf
{
public:
	Psf()          = default;
	virtual ~Psf() = default;

	KYTY_QT_CLASS_NO_COPY(Psf);

	void Load(const QString& file_name);

	[[nodiscard]] const QMap<QString, QVariant>& GetMap() const { return m_map; }

private:
	QMap<QString, QVariant> m_map;
};

inline void Psf::Load(const QString& file_name)
{
	struct ParamInfo
	{
		uint16_t name_offset;
		uint16_t type;
		uint32_t size1;
		uint32_t size2;
		uint32_t value_offset;
	};

	QFile f(file_name);
	if (!f.open(QIODevice::ReadOnly))
	{
		return;
	}

	QDataStream s(&f);

	s.setByteOrder(QDataStream::LittleEndian);

	uint32_t magic1 = 0;
	uint32_t magic2 = 0;

	s >> magic1;
	s >> magic2;

	if (magic1 != 0x46535000 && magic2 != 0x00000101)
	{
		return;
	}

	uint32_t name_tbl_offset  = 0;
	uint32_t value_tbl_offset = 0;
	uint32_t params_num       = 0;

	s >> name_tbl_offset;
	s >> value_tbl_offset;
	s >> params_num;

	if (name_tbl_offset == 0 || value_tbl_offset == 0 || value_tbl_offset <= name_tbl_offset || params_num == 0)
	{
		return;
	}

	uint32_t name_tbl_size = value_tbl_offset - name_tbl_offset;

	auto* params   = new ParamInfo[params_num];
	auto* name_tbl = new char[name_tbl_size];

	for (uint32_t i = 0; i < params_num; i++)
	{
		s >> params[i].name_offset;
		s >> params[i].type;
		s >> params[i].size1;
		s >> params[i].size2;
		s >> params[i].value_offset;
	}

	s.device()->seek(name_tbl_offset);
	s.readRawData(name_tbl, static_cast<int>(name_tbl_size));

	for (uint32_t i = 0; i < params_num; i++)
	{
		QString name = name_tbl + params[i].name_offset;

		switch (params[i].type)
		{
			case 0x0204:
			{
				s.device()->seek(value_tbl_offset + params[i].value_offset);
				QByteArray buf(static_cast<int>(params[i].size1), Qt::Uninitialized);
				s.readRawData(buf.data(), static_cast<int>(params[i].size1));
				QString value(buf);
				m_map.insert(name, value);
				break;
			}
			case 0x0404:
			{
				uint32_t value = 0;
				s.device()->seek(value_tbl_offset + params[i].value_offset);
				s >> value;
				m_map.insert(name, value);
				break;
			}
			default:;
		}
	}

	delete[] params;
	delete[] name_tbl;
}

#endif /* LAUNCHER_INCLUDE_PSF_H_ */
