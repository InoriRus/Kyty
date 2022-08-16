#include "ConfigurationEditDialog.h"

#include "Configuration.h"
#include "ConfigurationListWidget.h"
#include "MainDialog.h"
#include "MandatoryLineEdit.h"
#include "Psf.h"

#include <QByteArray>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFlags>
#include <QInternal>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPicture>
#include <QPushButton>
#include <QSettings>
#include <QStringList>
#include <QVariant>

#include "ui_configuration_edit_dialog.h"

constexpr char SETTINGS_CFG_DIALOG[]        = "ConfigurationEditDialog";
constexpr char SETTINGS_CFG_LAST_GEOMETRY[] = "geometry";
constexpr char SETTINGS_LAST_BASE_DIR[]     = "last_base_dir";
constexpr char COLOR_SELECTED[]             = "#e2ffe2";
constexpr char COLOR_NOT_SELECTED[]         = "#ffffff";
constexpr char COLOR_ODD_ROW[]              = "#fff5e5";

static void ChangeColor(QListWidgetItem* item)
{
	if (item->checkState() == Qt::Checked)
	{
		item->setBackground(QColor(COLOR_SELECTED));
	} else
	{
		item->setBackground(QColor(COLOR_NOT_SELECTED));
	}
}

ConfigurationEditDialog::ConfigurationEditDialog(Kyty::Configuration* info, ConfigurationListWidget* parent)
    : QDialog(parent, Qt::WindowCloseButtonHint), m_ui(new Ui::ConfigurationEditDialog), m_info(info), m_parent(parent)
{
	m_ui->setupUi(this);

	connect(m_ui->ok_button, &QPushButton::clicked, this, &ConfigurationEditDialog::save);
	connect(m_ui->clear_button, &QPushButton::clicked, this, &ConfigurationEditDialog::clear);
	connect(m_ui->test_button, &QPushButton::clicked, this, &ConfigurationEditDialog::test);
	connect(m_ui->browse_base_dir_button, &QPushButton::clicked, this, &ConfigurationEditDialog::browse_base_path);
	connect(m_ui->browse_param_file_button, &QPushButton::clicked, this, &ConfigurationEditDialog::browse_param_file);
	connect(m_ui->comboBox_shader_log_direction, &QComboBox::currentTextChanged,
	        [=](const QString& text)
	        {
		        auto log = TextToEnum<Kyty::Configuration::ShaderLogDirection>(text);
		        m_ui->lineEdit_shader_log_folder->setEnabled(log == Kyty::Configuration::ShaderLogDirection::File);
	        });
	connect(m_ui->checkBox_cmd_dump, &QCheckBox::toggled, [=](bool flag) { m_ui->lineEdit_cmd_dump_folder->setEnabled(flag); });
	connect(m_ui->comboBox_printf_direction, &QComboBox::currentTextChanged,
	        [=](const QString& text)
	        {
		        auto log = TextToEnum<Kyty::Configuration::LogDirection>(text);
		        m_ui->lineEdit_printf_file->setEnabled(log == Kyty::Configuration::LogDirection::File);
	        });
	connect(m_ui->comboBox_profiler_direction, &QComboBox::currentTextChanged,
	        [=](const QString& text)
	        {
		        auto log = TextToEnum<Kyty::Configuration::ProfilerDirection>(text);
		        m_ui->lineEdit_profiler_file->setEnabled(log == Kyty::Configuration::ProfilerDirection::File ||
		                                                 log == Kyty::Configuration::ProfilerDirection::FileAndNetwork);
	        });
	connect(m_ui->base_directory_lineedit, &QLineEdit::textChanged, this, &ConfigurationEditDialog::scan_elfs);
	connect(m_ui->param_file_lineedit, &QLineEdit::textChanged, this, &ConfigurationEditDialog::load_param_sfo);
	connect(m_ui->listWidget_elfs, &QListWidget::itemChanged, this, &ChangeColor);
	// connect(m_ui->listWidget_libs, &QListWidget::itemChanged, this, &ChangeColor);
	connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
	        [=](int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/) { m_ui->test_button->setEnabled(true); });

	layout()->setSizeConstraint(QLayout::SetFixedSize);

	restoreGeometry(g_last_geometry);

	m_ui->params_table->setWordWrap(false);
	m_ui->params_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_ui->params_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_ui->params_table->horizontalHeader()->setStretchLastSection(true);
	m_ui->params_table->setStyleSheet("selection-background-color: lightgray; selection-color: black;");

	Init();

	// scan_libs();
	scan_elfs();
	load_param_sfo();
}

QString    ConfigurationEditDialog::g_last_base_dir;
QByteArray ConfigurationEditDialog::g_last_geometry;

ConfigurationEditDialog::~ConfigurationEditDialog()
{
	delete m_ui;
}

void ConfigurationEditDialog::WriteSettings(QSettings* s)
{
	s->beginGroup(SETTINGS_CFG_DIALOG);

	if (!g_last_base_dir.isEmpty())
	{
		s->setValue(SETTINGS_LAST_BASE_DIR, g_last_base_dir);
	}

	if (!g_last_geometry.isEmpty())
	{
		s->setValue(SETTINGS_CFG_LAST_GEOMETRY, g_last_geometry);
	}

	s->endGroup();
}

void ConfigurationEditDialog::ReadSettings(QSettings* s)
{
	s->beginGroup(SETTINGS_CFG_DIALOG);

	g_last_base_dir = s->value(SETTINGS_LAST_BASE_DIR, g_last_base_dir).toString();
	g_last_geometry = s->value(SETTINGS_CFG_LAST_GEOMETRY, g_last_geometry).toByteArray();

	s->endGroup();
}

template <class T>
static void ListInit(QComboBox* combo, T value)
{
	combo->clear();
	combo->addItems(EnumToList<T>());
	combo->setCurrentText(EnumToText(value));
}

void ConfigurationEditDialog::Init()
{
	m_ui->name_lineedit->setText(m_info->name);
	m_ui->base_directory_lineedit->setText(m_info->basedir);
	m_ui->param_file_lineedit->setText(m_info->param_file);
	ListInit(m_ui->comboBox_screen_resolution, m_info->screen_resolution);
	m_ui->checkBox_neo->setChecked(m_info->neo);
	m_ui->checkBox_shader_validation->setChecked(m_info->shader_validation_enabled);
	m_ui->checkBox_vulkan_validation->setChecked(m_info->vulkan_validation_enabled);
	ListInit(m_ui->comboBox_shader_optimization_type, m_info->shader_optimization_type);
	ListInit(m_ui->comboBox_shader_log_direction, m_info->shader_log_direction);
	m_ui->lineEdit_shader_log_folder->setText(m_info->shader_log_folder);
	m_ui->checkBox_cmd_dump->setChecked(m_info->command_buffer_dump_enabled);
	m_ui->lineEdit_cmd_dump_folder->setText(m_info->command_buffer_dump_folder);
	ListInit(m_ui->comboBox_printf_direction, m_info->printf_direction);
	m_ui->lineEdit_printf_file->setText(m_info->printf_output_file);
	ListInit(m_ui->comboBox_profiler_direction, m_info->profiler_direction);
	m_ui->lineEdit_profiler_file->setText(m_info->profiler_output_file);
}

void ConfigurationEditDialog::SetTitle(const QString& str)
{
	setWindowTitle(str);
}

void ConfigurationEditDialog::moveEvent(QMoveEvent* event)
{
	QDialog::moveEvent(event);
	g_last_geometry = saveGeometry();
}

static void UpdateList(QStringList* list, QStringList* list_selected, QListWidget* list_widget)
{
	list->clear();
	list_selected->clear();
	int libs_count = list_widget->count();
	for (int i = 0; i < libs_count; i++)
	{
		auto* item = list_widget->item(i);
		if ((item->flags() & Qt::ItemIsEnabled) != 0)
		{
			*list << item->text();
			if (item->checkState() == Qt::Checked)
			{
				*list_selected << item->text();
			}
		}
	}
}

static void UpdateInfo(Kyty::Configuration* info, Ui::ConfigurationEditDialog* ui)
{
	info->name                      = ui->name_lineedit->text();
	info->basedir                   = ui->base_directory_lineedit->text();
	info->param_file                = ui->param_file_lineedit->text();
	info->screen_resolution         = TextToEnum<Kyty::Configuration::Resolution>(ui->comboBox_screen_resolution->currentText());
	info->neo                       = ui->checkBox_neo->isChecked();
	info->vulkan_validation_enabled = ui->checkBox_vulkan_validation->isChecked();
	info->shader_validation_enabled = ui->checkBox_shader_validation->isChecked();
	info->shader_optimization_type =
	    TextToEnum<Kyty::Configuration::ShaderOptimizationType>(ui->comboBox_shader_optimization_type->currentText());
	info->shader_log_direction = TextToEnum<Kyty::Configuration::ShaderLogDirection>(ui->comboBox_shader_log_direction->currentText());
	info->shader_log_folder    = ui->lineEdit_shader_log_folder->text();
	info->command_buffer_dump_enabled = ui->checkBox_cmd_dump->isChecked();
	info->command_buffer_dump_folder  = ui->lineEdit_cmd_dump_folder->text();
	info->printf_direction            = TextToEnum<Kyty::Configuration::LogDirection>(ui->comboBox_printf_direction->currentText());
	info->printf_output_file          = ui->lineEdit_printf_file->text();
	info->profiler_direction          = TextToEnum<Kyty::Configuration::ProfilerDirection>(ui->comboBox_profiler_direction->currentText());
	info->profiler_output_file        = ui->lineEdit_profiler_file->text();

	info->elfs.clear();
	info->elfs_selected.clear();
	int libs_count = ui->listWidget_elfs->count();
	for (int i = 0; i < libs_count; i++)
	{
		auto* item = ui->listWidget_elfs->item(i);
		if ((item->flags() & Qt::ItemIsEnabled) != 0)
		{
			info->elfs << item->text();
			if (item->checkState() == Qt::Checked)
			{
				info->elfs_selected << item->text();
			}
		}
	}

	UpdateList(&info->elfs, &info->elfs_selected, ui->listWidget_elfs);
	// UpdateList(&info->libs, &info->libs_selected, ui->listWidget_libs);
}

void ConfigurationEditDialog::update_info()
{
	UpdateInfo(m_info, m_ui);
}

void ConfigurationEditDialog::adjust_size()
{
	this->adjustSize();
}

void ConfigurationEditDialog::save()
{
	if (MandatoryLineEdit::FindEmpty(this))
	{
		QMessageBox::critical(this, tr("Save failed"), tr("Please fill all mandatory fields"));
		return;
	}

	update_info();

	emit accept();
}

void ConfigurationEditDialog::browse_base_path()
{
	QString text = m_ui->base_directory_lineedit->text();

	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), text.isEmpty() ? g_last_base_dir : text);

	if (!dir.isEmpty())
	{
		m_ui->base_directory_lineedit->setText(dir);
		g_last_base_dir = dir;
	}
}

void ConfigurationEditDialog::browse_param_file()
{
	QString text = m_ui->param_file_lineedit->text();

	if (text.isEmpty())
	{
		text = m_ui->base_directory_lineedit->text();
	}

	QString file = QFileDialog::getOpenFileName(this, tr("Select param.sfo"), text.isEmpty() ? g_last_base_dir : text, "param.sfo");

	if (!file.isEmpty())
	{
		m_ui->param_file_lineedit->setText(file);
	}
}

void ConfigurationEditDialog::scan_elfs()
{
	auto dir        = m_ui->base_directory_lineedit->text();
	auto qdir       = QDir(dir);
	bool dir_exists = !dir.isEmpty() && qdir.exists();

	QStringList elfs;

	if (dir_exists)
	{
		QDirIterator it(dir, QStringList({"*.elf", "*.prx", "*.sprx", "eboot.bin"}), QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext())
		{
			auto file = it.next();
			elfs << qdir.relativeFilePath(file);
		}
	}

	m_ui->listWidget_elfs->clear();

	// int selected_num = 0;

	for (const auto& elf: m_info->elfs)
	{
		bool not_enabled = !elfs.contains(elf);
		bool selected    = m_info->elfs_selected.contains(elf);

		auto* item = new QListWidgetItem(elf, m_ui->listWidget_elfs);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

		if (not_enabled)
		{
			item->setCheckState(Qt::Unchecked);
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		} else
		{
			item->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
		}

		// selected_num += (selected ? 1 : 0);
	}

	for (const auto& elf: elfs)
	{
		if (!m_info->elfs.contains(elf))
		{
			auto* item = new QListWidgetItem(elf, m_ui->listWidget_elfs);
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(Qt::Unchecked);
		}
	}

	m_ui->listWidget_elfs->sortItems();

	m_ui->test_button->setEnabled(m_process.state() == QProcess::NotRunning && dir_exists);
}

// void ConfigurationEditDialog::scan_libs()
//{
//	QStringList libs = KYTY_LIBS;
//
//	m_ui->listWidget_libs->clear();
//
//	// int selected_num = 0;
//
//	for (const auto& lib: m_info->libs)
//	{
//		bool not_enabled = !libs.contains(lib);
//		bool selected    = m_info->libs_selected.contains(lib);
//
//		auto* item = new QListWidgetItem(lib, m_ui->listWidget_libs);
//		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
//
//		if (not_enabled)
//		{
//			item->setCheckState(Qt::Unchecked);
//			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
//		} else
//		{
//			item->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
//		}
//
//		// selected_num += (selected ? 1 : 0);
//	}
//
//	for (const auto& lib: libs)
//	{
//		if (!m_info->libs.contains(lib))
//		{
//			auto* item = new QListWidgetItem(lib, m_ui->listWidget_libs);
//			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
//			item->setCheckState(Qt::Unchecked);
//		}
//	}
//
//	m_ui->listWidget_libs->sortItems();
// }

void ConfigurationEditDialog::load_param_sfo()
{
	auto file_name = m_ui->param_file_lineedit->text();

	m_ui->image->clear();
	m_ui->params_table->clear();
	m_ui->params_table->setRowCount(0);
	m_ui->params_table->setColumnCount(2);
	m_ui->params_table->setHorizontalHeaderLabels({tr("Parameter"), tr("Value")});
	m_ui->params_table->resizeColumnsToContents();

	QFile param(file_name);

	if (param.exists())
	{
		Psf psf;
		psf.Load(file_name);

		const auto& map = psf.GetMap();

		m_ui->params_table->setColumnCount(2);
		m_ui->params_table->setRowCount(map.size());

		int index = 0;
		for (auto& m: map.toStdMap())
		{
			auto* item1 = new QTableWidgetItem(m.first);
			item1->setFlags(item1->flags() ^ Qt::ItemIsEditable);
			auto* item2 = new QTableWidgetItem(
			    m.second.type() == QVariant::UInt ? QString("0x%1").arg(m.second.toUInt(), 8, 16, QLatin1Char('0')) : m.second.toString());
			item2->setFlags(item2->flags() ^ Qt::ItemIsEditable);
			if (index % 2 != 0)
			{
				item1->setBackground(QColor(COLOR_ODD_ROW));
				item2->setBackground(QColor(COLOR_ODD_ROW));
			}
			m_ui->params_table->setItem(index, 0, item1);
			m_ui->params_table->setItem(index, 1, item2);

			index++;
		}

		m_ui->params_table->resizeRowsToContents();
		m_ui->params_table->resizeColumnToContents(0);

		QFile image(QFileInfo(file_name).absoluteDir().filePath("pic1.png"));

		if (image.exists())
		{
			QPixmap picture;
			picture.load(image.fileName());
			m_ui->image->setPixmap(picture.scaledToWidth(m_ui->image->width(), Qt::SmoothTransformation));
		}
	}
}

void ConfigurationEditDialog::clear()
{
	auto* old_info = m_info;

	m_info = new Kyty::Configuration;
	Init();
	delete m_info;

	m_info = old_info;

	scan_elfs();
	// scan_libs();
	load_param_sfo();
}

void ConfigurationEditDialog::test()
{
	m_ui->test_button->setEnabled(false);

	auto* info = new Kyty::Configuration;

	UpdateInfo(info, m_ui);

	m_parent->GetMainDialog()->RunInterpreter(&m_process, info, false);

	delete info;
}
