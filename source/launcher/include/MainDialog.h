#ifndef MAIN_DIALOG_H
#define MAIN_DIALOG_H

#include "Common.h"

#include <QDialog>
#include <QString>

class QWidget;
class QProcess;
class MainDialogPrivate;
class QSettings;
class QResizeEvent;

namespace Kyty {
class Configuration;
} // namespace Kyty

class MainDialog: public QDialog
{
	Q_OBJECT
	KYTY_QT_CLASS_NO_COPY(MainDialog);

signals:
	void Start();
	void Quit();
	void Resize();

public:
	explicit MainDialog(QWidget* parent = nullptr);
	~MainDialog() override = default;

	void RunInterpreter(QProcess* process, Kyty::Configuration* info, bool con_emu);

	static void WriteSettings(QSettings* s);
	static void ReadSettings(QSettings* s);

	void resizeEvent(QResizeEvent* event) override;

private:
	MainDialogPrivate* m_p = nullptr;
};

#endif // MAIN_DIALOG_H
