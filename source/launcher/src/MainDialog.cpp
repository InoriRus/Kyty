#include "MainDialog.h"

#include "Common.h"
#include "Configuration.h"
#include "ConfigurationItem.h"
#include "ConfigurationListWidget.h"

#include <QApplication>
#include <QByteArray>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExp>
#include <QSettings>
#include <QStringList>
#include <QTextStream>
#include <QVariant>
#include <Q_STARTUPINFO>
#include <QtCore>

#include "ui_main_dialog.h"

#include <windows.h> // IWYU pragma: keep

// IWYU pragma: no_include <minwindef.h>
// IWYU pragma: no_include <processthreadsapi.h>
// IWYU pragma: no_include <winbase.h>

class QWidget;

constexpr char SCRIPT_EXE[]    = "fc_script.exe";
constexpr char CMD_EXE[]       = "cmd.exe";
constexpr char CONEMU_EXE[]    = "C:/Program Files/ConEmu/ConEmu64.exe";
constexpr char KYTY_MOUNT[]    = "kyty_mount";
constexpr char KYTY_EXECUTE[]  = "kyty_execute";
constexpr char KYTY_LOAD_ELF[] = "kyty_load_elf";
// constexpr char  KYTY_LOAD_SYMBOLS[]           = "kyty_load_symbols";
constexpr char  KYTY_LOAD_SYMBOLS_ALL[]       = "kyty_load_symbols_all";
constexpr char  KYTY_LOAD_PARAM_SFO[]         = "kyty_load_param_sfo";
constexpr char  KYTY_INIT[]                   = "kyty_init";
constexpr char  KYTY_LUA_FILE[]               = "kyty_run.lua";
constexpr DWORD CMD_X_CHARS                   = 175;
constexpr DWORD CMD_Y_CHARS                   = 1000;
constexpr char  SETTINGS_MAIN_DIALOG[]        = "MainDialog";
constexpr char  SETTINGS_MAIN_LAST_GEOMETRY[] = "geometry";

class DetachableProcess: public QProcess
{
public:
	explicit DetachableProcess(QObject* parent = nullptr): QProcess(parent) {}
	void Detach()
	{
		this->waitForStarted();
		setProcessState(QProcess::NotRunning);
	}
};

class MainDialogPrivate: public QObject
{
	Q_OBJECT
	KYTY_QT_CLASS_NO_COPY(MainDialogPrivate);

public:
	MainDialogPrivate() = default;
	~MainDialogPrivate() override;

	void Setup(MainDialog* main_dialog);

	/*slots:*/

	void Update();
	void FindInterpreter();
	void Run();

	[[nodiscard]] const QString& GetInterpreter() const { return m_interpreter; }

	static void WriteSettings(QSettings* s);
	static void ReadSettings(QSettings* s);

private:
	static QByteArray g_last_geometry;

	Ui::MainDialog*    m_ui          = {nullptr};
	MainDialog*        m_main_dialog = nullptr;
	QString            m_interpreter;
	DetachableProcess  m_process;
	ConfigurationItem* m_running_item = nullptr;
};

QByteArray MainDialogPrivate::g_last_geometry;

MainDialog::MainDialog(QWidget* parent): QDialog(parent), m_p(new MainDialogPrivate)
{
	m_p->Setup(this);
}

MainDialogPrivate::~MainDialogPrivate()
{
	delete m_ui;
}

void MainDialogPrivate::Setup(MainDialog* main_dialog)
{
	m_ui = new Ui::MainDialog;
	m_ui->setupUi(main_dialog);

	setParent(main_dialog);
	m_main_dialog = main_dialog;
	m_ui->widget->SetMainDialog(main_dialog);

	main_dialog->setWindowFlags(Qt::Dialog /*| Qt::MSWindowsFixedSizeDialogHint*/);

	connect(main_dialog, &MainDialog::Start, this, &MainDialogPrivate::FindInterpreter, Qt::QueuedConnection);
	connect(m_ui->pushButton_Run, &QPushButton::clicked, this, &MainDialogPrivate::Run);
	connect(m_ui->widget, &ConfigurationListWidget::Select, this, &MainDialogPrivate::Update);
	connect(m_ui->widget, &ConfigurationListWidget::Run, this, &MainDialogPrivate::Run);
	connect(main_dialog, &MainDialog::Resize,
	        [=]()
	        {
		        g_last_geometry = m_main_dialog->saveGeometry();
		        m_ui->widget->WriteSettings();
	        });

	connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
	        [=](int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/)
	        {
		        m_running_item->SetRunning(false);
		        Update();
	        });

	connect(main_dialog, &MainDialog::Quit, [=]() { m_process.Detach(); });

	m_ui->label_settings_file->setText(tr("Settings file: ") + m_ui->widget->GetSettingsFile());

	m_main_dialog->restoreGeometry(g_last_geometry);

	Update();
}

void MainDialogPrivate::FindInterpreter()
{
	QFile file(QDir(".").absoluteFilePath(SCRIPT_EXE));
	if (file.exists())
	{
		m_interpreter = file.fileName();
	} else
	{
		QFile file(QDir("..").absoluteFilePath(SCRIPT_EXE));
		if (file.exists())
		{
			m_interpreter = file.fileName();
		}
	}

	bool found = QFile::exists(m_interpreter);

	if (found)
	{
		m_ui->label_Interpreter->setText(tr("Emulator: ") + m_interpreter);

		QProcess test;
		test.setProgram(m_interpreter);
		test.start();
		test.waitForFinished();

		auto output = QString(test.readAllStandardOutput());
		auto lines  = output.split(QRegExp("[\r\n]"), Qt::SkipEmptyParts);

		if (lines.count() >= 2)
		{
			m_ui->label_Version->setText(tr("Version: ") + (lines.at(0).startsWith("exe_name") ? lines.at(1) : lines.at(0)));
		} else
		{
			found = false;
		}

		found = found && lines.contains(QString("Lua function: ") + KYTY_MOUNT);
		found = found && lines.contains(QString("Lua function: ") + KYTY_EXECUTE);
		found = found && lines.contains(QString("Lua function: ") + KYTY_LOAD_ELF);
		found = found && lines.contains(QString("Lua function: ") + KYTY_LOAD_SYMBOLS_ALL);
		found = found && lines.contains(QString("Lua function: ") + KYTY_LOAD_PARAM_SFO);
		found = found && lines.contains(QString("Lua function: ") + KYTY_INIT);
	}

	if (!found)
	{
		QMessageBox::critical(m_main_dialog, tr("Error"), tr("Can't find emulator"));
		QApplication::quit();
		return;
	}

	QFile console(CONEMU_EXE);
	bool  con_emu = console.exists();

	m_ui->radioButton_Cmd->setEnabled(true);
	m_ui->radioButton_Cmd->setChecked(true);
	m_ui->radioButton_ConEmu->setEnabled(con_emu);
	m_ui->radioButton_ConEmu->setChecked(false);

	Update();
}

static bool CreateLuaScript(Kyty::Configuration* info, const QString& file_name)
{
	QFile file(file_name);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream s(&file);

		auto r = EnumToText(info->screen_resolution).split('x');

		if (r.size() != 2)
		{
			file.close();
			return false;
		}

		s << "local cfg = {\n";
		s << "\t ScreenWidth = " << r.at(0) << ";\n";
		s << "\t ScreenHeight = " << r.at(1) << ";\n";
		s << "\t Neo = " << (info->neo ? "true" : "false") << ";\n";
		s << "\t VulkanValidationEnabled = " << (info->vulkan_validation_enabled ? "true" : "false") << ";\n";
		s << "\t ShaderValidationEnabled = " << (info->shader_validation_enabled ? "true" : "false") << ";\n";
		s << "\t ShaderOptimizationType = '" << EnumToText(info->shader_optimization_type) << "';\n";
		s << "\t ShaderLogDirection = '" << EnumToText(info->shader_log_direction) << "';\n";
		s << "\t ShaderLogFolder = '" << info->shader_log_folder << "';\n";
		s << "\t CommandBufferDumpEnabled = " << (info->command_buffer_dump_enabled ? "true" : "false") << ";\n";
		s << "\t CommandBufferDumpFolder = '" << info->command_buffer_dump_folder << "';\n";
		s << "\t PrintfDirection = '" << EnumToText(info->printf_direction) << "';\n";
		s << "\t PrintfOutputFile = '" << info->printf_output_file << "';\n";
		s << "\t ProfilerDirection = '" << EnumToText(info->profiler_direction) << "';\n";
		s << "\t ProfilerOutputFile = '" << info->profiler_output_file << "';\n";
		s << "\t SpirvDebugPrintfEnabled = false;\n";
		s << "}\n";

		s << KYTY_INIT << "(cfg);\n";
		s << KYTY_MOUNT << "('" << info->basedir << "', '/app0');\n";

		s << KYTY_LOAD_PARAM_SFO << "('" << info->param_file << "');\n";

		for (const auto& elf: info->elfs_selected)
		{
			s << KYTY_LOAD_ELF << "('/app0/" << elf << "');\n";
		}

		//		for (const auto& lib: info->libs_selected)
		//		{
		//			s << KYTY_LOAD_SYMBOLS << "('" << lib << "');\n";
		//		}

		s << KYTY_LOAD_SYMBOLS_ALL << "();\n";

		s << KYTY_EXECUTE << "();\n";

		file.close();
		return true;
	}
	return false;
}

void MainDialog::RunInterpreter(QProcess* process, Kyty::Configuration* info, bool con_emu)
{
	const auto& interpreter = m_p->GetInterpreter();

	QFileInfo f(interpreter);
	auto      dir = f.absoluteDir();

	auto lua_file_name = dir.filePath(KYTY_LUA_FILE);

	if (!CreateLuaScript(info, lua_file_name))
	{
		QMessageBox::critical(this, tr("Error"), tr("Can't create file:\n") + lua_file_name);
		QApplication::quit();
		return;
	}

	if (con_emu)
	{
		process->setProgram(CONEMU_EXE);
		process->setArguments({"-run", /*CMD_EXE, "/K", */ \"\"interpreter\"\", \"\"lua_file_name\"\"});
	} else
	{
		process->setProgram(CMD_EXE);
		process->setArguments({"/K", \"\"interpreter\"\", lua_file_name});
	}
	process->setWorkingDirectory(dir.path());
	process->setCreateProcessArgumentsModifier(
	    [](QProcess::CreateProcessArguments* args)
	    {
		    args->flags |= static_cast<uint32_t>(CREATE_NEW_CONSOLE);
		    args->startupInfo->dwFlags &= ~static_cast<DWORD>(STARTF_USESTDHANDLES);
		    args->startupInfo->dwFlags |= static_cast<DWORD>(STARTF_USECOUNTCHARS);
		    args->startupInfo->dwXCountChars = CMD_X_CHARS;
		    args->startupInfo->dwYCountChars = CMD_Y_CHARS;
		    // args->startupInfo->dwFlags |= static_cast<DWORD>(STARTF_USEFILLATTRIBUTE);
		    // args->startupInfo->dwFillAttribute =
		    //     static_cast<DWORD>(BACKGROUND_BLUE) | static_cast<DWORD>(FOREGROUND_RED) | static_cast<DWORD>(FOREGROUND_INTENSITY);
	    });
	process->start();
	process->waitForFinished(100);
}

void MainDialog::WriteSettings(QSettings* s)
{
	MainDialogPrivate::WriteSettings(s);
}

void MainDialog::ReadSettings(QSettings* s)
{
	MainDialogPrivate::ReadSettings(s);
}

void MainDialog::resizeEvent(QResizeEvent* event)
{
	emit Resize();
	QDialog::resizeEvent(event);
}

void MainDialogPrivate::WriteSettings(QSettings* s)
{
	s->beginGroup(SETTINGS_MAIN_DIALOG);

	if (!g_last_geometry.isEmpty())
	{
		s->setValue(SETTINGS_MAIN_LAST_GEOMETRY, g_last_geometry);
	}

	s->endGroup();
}

void MainDialogPrivate::ReadSettings(QSettings* s)
{
	s->beginGroup(SETTINGS_MAIN_DIALOG);

	g_last_geometry = s->value(SETTINGS_MAIN_LAST_GEOMETRY, g_last_geometry).toByteArray();

	s->endGroup();
}

void MainDialogPrivate::Run()
{
	m_running_item = m_ui->widget->GetSelectedItem();

	m_running_item->SetRunning(true);

	m_main_dialog->RunInterpreter(&m_process, m_running_item->GetInfo(), m_ui->radioButton_ConEmu->isChecked());

	Update();
}

void MainDialogPrivate::Update()
{
	const auto* item = m_ui->widget->GetSelectedItem();

	bool run_enabled = (m_process.state() == QProcess::NotRunning && item != nullptr);

	if (run_enabled && item != nullptr)
	{
		const auto* info = item->GetInfo();
		auto        dir  = info->basedir;
		run_enabled      = !dir.isEmpty() && QDir(dir).exists();
	}

	m_ui->pushButton_Run->setEnabled(run_enabled);
	m_ui->widget->SetRunEnabled(run_enabled);
}

#include "MainDialog.moc"
