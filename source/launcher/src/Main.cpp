#include "MainDialog.h"

#include <QApplication>
#include <QArgument>
#include <QObject>
#include <QStyleFactory>

class QStyle;

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	MainDialog   w;

	QStyle* s = QStyleFactory::create("fusion");
	QApplication::setStyle(s);

	QObject::connect(&a, &QApplication::aboutToQuit, &w, &MainDialog::Quit);

	w.emit Start();

	w.show();

	return QApplication::exec();
}
