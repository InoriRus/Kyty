#ifndef CONFIGURATION_EDIT_DIALOG_H
#define CONFIGURATION_EDIT_DIALOG_H

#include "Common.h"

#include <QDialog>
#include <QProcess>
#include <QString>
class QByteArray;
class QMoveEvent;
class QSettings;
class ConfigurationListWidget;

namespace Kyty {
class Configuration;
} // namespace Kyty

namespace Ui {
class ConfigurationEditDialog;
} // namespace Ui

class ConfigurationEditDialog: public QDialog
{
	Q_OBJECT
	KYTY_QT_CLASS_NO_COPY(ConfigurationEditDialog);

public:
	explicit ConfigurationEditDialog(Kyty::Configuration* info, ConfigurationListWidget* parent = nullptr);
	~ConfigurationEditDialog() override;

	static void WriteSettings(QSettings* s);
	static void ReadSettings(QSettings* s);

	void SetTitle(const QString& str);

private:
	Ui::ConfigurationEditDialog* m_ui   = nullptr;
	Kyty::Configuration*         m_info = nullptr;
	QProcess                     m_process;
	ConfigurationListWidget*     m_parent = nullptr;

protected:
	void Init();

	void moveEvent(QMoveEvent* event) override;

	static QString    g_last_base_dir;
	static QByteArray g_last_geometry;

	/*slots:*/

	void update_info();
	void adjust_size();
	void save();
	void browse_base_path();
	void browse_param_file();
	void scan_elfs();
	// void scan_libs();
	void load_param_sfo();
	void clear();
	void test();
};

#endif // CONFIGURATION_EDIT_DIALOG_H
