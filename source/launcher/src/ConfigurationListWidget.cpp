#include "ConfigurationListWidget.h"

#include "Common.h"
#include "Configuration.h"
#include "ConfigurationEditDialog.h"
#include "ConfigurationItem.h"
#include "MainDialog.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QCursor>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QToolButton>
#include <QtCore>

#include "ui_configuration_list_widget.h"

constexpr char CONF_FILE_NAME[]    = "Kyty.ini";
constexpr char CONF_ORG_NAME[]     = "Kyty";
constexpr char CONF_APP_NAME[]     = "Kyty";
constexpr char CONF_SECTION_NAME[] = "ConfigurationList";

ConfigurationListWidget::ConfigurationListWidget(QWidget* parent): QWidget(parent), m_ui(new Ui::ConfigurationListWidget)
{
	m_ui->setupUi(this);

	m_ui->delete_button->setEnabled(false);
	m_ui->edit_button->setEnabled(false);

	m_ui->cfgs_list->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(m_ui->add_button, &QPushButton::clicked, this, &ConfigurationListWidget::add_configuration);
	connect(m_ui->edit_button, &QPushButton::clicked, this, &ConfigurationListWidget::edit_configuration);
	connect(m_ui->delete_button, &QPushButton::clicked, this, &ConfigurationListWidget::delete_configuartion);
	connect(m_ui->cfgs_list, &QListWidget::itemClicked, this, &ConfigurationListWidget::list_itemClicked);
	connect(m_ui->cfgs_list, &QListWidget::customContextMenuRequested, this, &ConfigurationListWidget::show_context_menu);

	m_ui->cfgs_list->setDragDropMode(QAbstractItemView::InternalMove);
	connect(m_ui->cfgs_list->model(), &QAbstractItemModel::rowsMoved, this, &ConfigurationListWidget::WriteSettings);

	ReadSettings();
}

ConfigurationListWidget::~ConfigurationListWidget()
{
	delete m_ui;
}

void ConfigurationListWidget::WriteSettings()
{
	QSettings* s = nullptr;
	QFile      file(QDir(".").absoluteFilePath(CONF_FILE_NAME));
	if (file.exists())
	{
		s = new QSettings(CONF_FILE_NAME, QSettings::IniFormat);
	} else
	{
		s = new QSettings(QSettings::IniFormat, QSettings::SystemScope, CONF_ORG_NAME, CONF_APP_NAME);
	}

	if (s != nullptr)
	{
		MainDialog::WriteSettings(s);
		ConfigurationEditDialog::WriteSettings(s);

		s->remove(CONF_SECTION_NAME);
		s->beginWriteArray(CONF_SECTION_NAME);
		for (int i = 0; i < m_ui->cfgs_list->count(); i++)
		{
			auto* item = static_cast<ConfigurationItem*>(m_ui->cfgs_list->item(i));
			s->setArrayIndex(i);
			item->GetInfo()->WriteSettings(s);
		}
		s->endArray();
	}

	delete s;
}

void ConfigurationListWidget::ReadSettings()
{
	QSettings* s = nullptr;
	QFile      file(QDir(".").absoluteFilePath(CONF_FILE_NAME));
	if (file.exists())
	{
		s = new QSettings(CONF_FILE_NAME, QSettings::IniFormat);
	} else
	{
		s = new QSettings(QSettings::IniFormat, QSettings::SystemScope, CONF_ORG_NAME, CONF_APP_NAME);
	}

	if (s != nullptr)
	{
		m_settings_file = s->fileName();

		MainDialog::ReadSettings(s);
		ConfigurationEditDialog::ReadSettings(s);

		int size = s->beginReadArray(CONF_SECTION_NAME);

		for (int i = 0; i < size; i++)
		{
			s->setArrayIndex(i);
			auto* info = new Kyty::Configuration;
			info->ReadSettings(s);
			new ConfigurationItem(info, m_ui->cfgs_list);
		}
		s->endArray();
	}

	delete s;
}

void ConfigurationListWidget::add_configuration()
{
	auto* info = new Kyty::Configuration;

	ConfigurationEditDialog dlg(info, this);
	dlg.SetTitle(tr("New configuration"));

	if (dlg.exec() == QDialog::Accepted)
	{
		new ConfigurationItem(info, m_ui->cfgs_list);
		WriteSettings();
	}
}

void ConfigurationListWidget::edit_configuration()
{
	auto* item = static_cast<ConfigurationItem*>(m_ui->cfgs_list->currentItem());

	ConfigurationEditDialog dlg(item->GetInfo(), this);
	dlg.SetTitle(tr("Edit configuration"));

	if (dlg.exec() == QDialog::Accepted)
	{
		WriteSettings();
		item->Update();
	}
}

void ConfigurationListWidget::delete_configuartion()
{
	if (QMessageBox::Yes == QMessageBox::question(this, tr("Delete configuration"), tr("Do you want to delete configuration?")))
	{
		auto* item = static_cast<ConfigurationItem*>(m_ui->cfgs_list->takeItem(m_ui->cfgs_list->currentRow()));
		delete item;
		WriteSettings();
	}
}

void ConfigurationListWidget::run_configuration()
{
	emit Run();
}

void ConfigurationListWidget::list_itemClicked(QListWidgetItem* witem)
{
	auto* item = static_cast<ConfigurationItem*>(witem);

	m_ui->delete_button->setEnabled(!item->IsRunning());
	m_ui->edit_button->setEnabled(!item->IsRunning());

	m_selected_item = item;

	emit Select();
}

void ConfigurationListWidget::show_context_menu(const QPoint& pos)
{
	auto* item = static_cast<ConfigurationItem*>(m_ui->cfgs_list->itemAt(pos));

	if (item != nullptr)
	{
		list_itemClicked(item);
	}

	QMenu menu;

	QAction* action_run = menu.addAction(tr("Run"), this, SLOT(run_configuration()));
	menu.addSeparator();
	/*QAction *action_new = */ menu.addAction(QIcon(":/add"), tr("New..."), this, SLOT(add_configuration()));
	QAction* action_edit   = menu.addAction(QIcon(":/edit"), tr("Edit..."), this, SLOT(edit_configuration()));
	QAction* action_delete = menu.addAction(QIcon(":/delete"), tr("Delete"), this, SLOT(delete_configuartion()));

	if (item != nullptr)
	{
		action_run->setDisabled(item->IsRunning());
		action_edit->setDisabled(item->IsRunning());
		action_delete->setDisabled(item->IsRunning());
	} else
	{
		action_run->setDisabled(true);
		action_edit->setDisabled(true);
		action_delete->setDisabled(true);
	}

	if (!m_run_enabled)
	{
		action_run->setDisabled(true);
	}

	menu.exec(QCursor::pos());
}
