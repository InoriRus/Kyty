#ifndef MANDATORY_LINEEDIT_H
#define MANDATORY_LINEEDIT_H

#include "Common.h"

#include <QLineEdit>
#include <QString>

class QPaintEvent;
class QWidget;

constexpr char MANDATORY_AND_EMPTY[] = "mandatory_and_empty";

class MandatoryLineEdit: public QLineEdit
{
	Q_OBJECT

public:
	explicit MandatoryLineEdit(QWidget* parent = nullptr);

	void paintEvent(QPaintEvent* event) override;

	static bool FindEmpty(QObject* p);

protected slots:

	void changed(const QString& text);
};

#endif // MANDATORY_LINEEDIT_H
