#ifndef CONFIGURATION_LIST_WIDGET_H
#define CONFIGURATION_LIST_WIDGET_H

#include "Common.h"

#include <QString>
#include <QWidget>

class ConfigurationItem;
class QListWidgetItem;
class QPoint;
class MainDialog;

namespace Ui {
class ConfigurationListWidget;
} // namespace Ui

class ConfigurationListWidget: public QWidget
{
	Q_OBJECT
	KYTY_QT_CLASS_NO_COPY(ConfigurationListWidget);

public:
	explicit ConfigurationListWidget(QWidget* parent = nullptr);
	~ConfigurationListWidget() override;

	void               SetRunEnabled(bool flag) { m_run_enabled = flag; }
	[[nodiscard]] bool IsRunEnabled() const { return m_run_enabled; }

	[[nodiscard]] const ConfigurationItem* GetSelectedItem() const { return m_selected_item; }
	ConfigurationItem*                     GetSelectedItem() { return m_selected_item; }

	void        SetMainDialog(MainDialog* main_dialog) { m_main_dialog = main_dialog; }
	MainDialog* GetMainDialog() { return m_main_dialog; }

	[[nodiscard]] const QString& GetSettingsFile() const { return m_settings_file; }

signals:

	void Run();
	void Select();

public slots:
	void WriteSettings();
	void ReadSettings();

protected slots:

	void add_configuration();
	void edit_configuration();
	void delete_configuartion();
	void run_configuration();
	void list_itemClicked(QListWidgetItem* witem);
	void show_context_menu(const QPoint& pos);

private:
	ConfigurationItem*           m_selected_item = nullptr;
	bool                         m_run_enabled   = true;
	Ui::ConfigurationListWidget* m_ui            = nullptr;
	MainDialog*                  m_main_dialog   = nullptr;
	QString                      m_settings_file;
};

#endif // CONFIGURATION_LIST_WIDGET_H
