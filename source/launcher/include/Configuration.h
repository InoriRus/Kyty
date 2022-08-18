#ifndef LAUNCHER_INCLUDE_CONFIGURATION_H_
#define LAUNCHER_INCLUDE_CONFIGURATION_H_

#include "Common.h"

#include <QByteArray>
#include <QChar>
#include <QMetaEnum>
#include <QMetaType>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

#define KYTY_CFG_SETL(n) s->setValue(#n, n);
#define KYTY_CFG_SET(n)  s->setValue(#n, QVariant::fromValue(n).toString());
#define KYTY_CFG_GET(n)  n = s->value(#n).value<decltype(n)>();
#define KYTY_CFG_GETL(n) n = s->value(#n).toStringList();


template <class T>
inline QStringList EnumToList()
{
	QStringList ret;
	auto        me    = QMetaEnum::fromType<T>();
	int         count = me.keyCount();
	for (int i = 0; i < count; i++)
	{
		auto key = QString(me.key(i));
		ret << (key.startsWith('R') && key.size() > 2 && key.at(1).isDigit() ? key.remove('R').toLower() : key);
	}
	return ret;
}

template <class T>
T TextToEnum(const QString& text)
{
	auto me = QMetaEnum::fromType<T>();
	return static_cast<T>(me.keyToValue(((text.size() > 1 && text.at(0).isDigit()) ? 'R' + text.toUpper() : text).toUtf8().data()));
}

template <class T>
QString EnumToText(T value)
{
	auto me  = QMetaEnum::fromType<T>();
	auto key = QString(me.valueToKey(static_cast<int>(value)));
	return (key.startsWith('R') && key.size() > 2 && key.at(1).isDigit() ? key.remove('R').toLower() : key);
}

namespace Kyty {

class Configuration: public QObject
{
	Q_OBJECT

public:
	enum class Resolution
	{
		R1280X720,
		R1920X1080,
	};
	Q_ENUM(Resolution)

	enum class ShaderOptimizationType
	{
		None,
		Size,
		Performance
	};
	Q_ENUM(ShaderOptimizationType)

	enum class ShaderLogDirection
	{
		Silent,
		Console,
		File
	};
	Q_ENUM(ShaderLogDirection)

	enum class ProfilerDirection
	{
		None,
		File,
		Network,
		FileAndNetwork
	};
	Q_ENUM(ProfilerDirection)

	enum class LogDirection
	{
		Silent,
		Console,
		File
	};
	Q_ENUM(LogDirection)

	Configuration() = default;

	QString name;
	QString basedir;    /* Game base directory */
	QString param_file; /* Path to param.sfo */

	Resolution             screen_resolution           = Resolution::R1280X720;
	bool                   neo                         = true;
	bool                   vulkan_validation_enabled   = true;
	bool                   shader_validation_enabled   = true;
	ShaderOptimizationType shader_optimization_type    = ShaderOptimizationType::Performance;
	ShaderLogDirection     shader_log_direction        = ShaderLogDirection::Silent;
	QString                shader_log_folder           = "_Shaders";
	bool                   command_buffer_dump_enabled = false;
	QString                command_buffer_dump_folder  = "_Buffers";
	LogDirection           printf_direction            = LogDirection::Silent;
	QString                printf_output_file          = "_kyty.txt";
	ProfilerDirection      profiler_direction          = ProfilerDirection::None;
	QString                profiler_output_file        = "_profile.prof";

	QStringList elfs;
	QStringList elfs_selected;
	// QStringList libs          = KYTY_LIBS;
	// QStringList libs_selected = KYTY_LIBS;

	void WriteSettings(QSettings* s) const
	{
		KYTY_CFG_SET(name);
		KYTY_CFG_SET(basedir);
		KYTY_CFG_SET(param_file);
		KYTY_CFG_SET(screen_resolution);
		KYTY_CFG_SET(neo);
		KYTY_CFG_SET(vulkan_validation_enabled);
		KYTY_CFG_SET(shader_validation_enabled);
		KYTY_CFG_SET(shader_optimization_type);
		KYTY_CFG_SET(shader_log_direction);
		KYTY_CFG_SET(shader_log_folder);
		KYTY_CFG_SET(command_buffer_dump_enabled);
		KYTY_CFG_SET(command_buffer_dump_folder);
		KYTY_CFG_SET(printf_direction);
		KYTY_CFG_SET(printf_output_file);
		KYTY_CFG_SET(profiler_direction);
		KYTY_CFG_SET(profiler_output_file);
		KYTY_CFG_SETL(elfs);
		KYTY_CFG_SETL(elfs_selected);
		// KYTY_CFG_SETL(libs);
		// KYTY_CFG_SETL(libs_selected);
	}

	void ReadSettings(QSettings* s)
	{
		KYTY_CFG_GET(name);
		KYTY_CFG_GET(basedir);
		KYTY_CFG_GET(param_file);
		KYTY_CFG_GET(screen_resolution);
		KYTY_CFG_GET(neo);
		KYTY_CFG_GET(vulkan_validation_enabled);
		KYTY_CFG_GET(shader_validation_enabled);
		KYTY_CFG_GET(shader_optimization_type);
		KYTY_CFG_GET(shader_log_direction);
		KYTY_CFG_GET(shader_log_folder);
		KYTY_CFG_GET(command_buffer_dump_enabled);
		KYTY_CFG_GET(command_buffer_dump_folder);
		KYTY_CFG_GET(printf_direction);
		KYTY_CFG_GET(printf_output_file);
		KYTY_CFG_GET(profiler_direction);
		KYTY_CFG_GET(profiler_output_file);
		KYTY_CFG_GETL(elfs);
		KYTY_CFG_GETL(elfs_selected);
		// KYTY_CFG_GETL(libs);
		// KYTY_CFG_GETL(libs_selected);
	}
};

} // namespace Kyty

Q_DECLARE_METATYPE(Kyty::Configuration*)

#endif /* LAUNCHER_INCLUDE_CONFIGURATION_H_ */
