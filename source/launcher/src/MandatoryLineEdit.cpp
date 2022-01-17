#include "MandatoryLineEdit.h"

#include <QColor>
#include <QLine>
#include <QPainter>
#include <QPen>
#include <QVariant>
#include <QtCore>

class QPaintEvent;
class QWidget;

MandatoryLineEdit::MandatoryLineEdit(QWidget* parent): QLineEdit(parent)
{
	this->setProperty(MANDATORY_AND_EMPTY, true);

	connect(this, &QLineEdit::textChanged, this, &MandatoryLineEdit::changed);
}

void MandatoryLineEdit::paintEvent(QPaintEvent* event)
{
	QLineEdit::paintEvent(event);

	if (this->property(MANDATORY_AND_EMPTY).toBool() && this->isEnabled())
	{
		QPainter painter(this);
		QLine    line(4, this->height() - 4, this->width() - 4, this->height() - 4);
		painter.setPen(QPen(QColor(255, 0, 0), 1.0, Qt::DotLine));
		painter.drawLine(line);
	}
}

bool MandatoryLineEdit::FindEmpty(QObject* p)
{
	const QList<QLineEdit*>& ch_list = p->findChildren<QLineEdit*>();

	foreach (QLineEdit* ch, ch_list)
	{
		if (ch->isVisible() && ch->property(MANDATORY_AND_EMPTY).toBool() && ch->isEnabled())
		{
			return true;
		}
	}

	return false;
}

void MandatoryLineEdit::changed(const QString& text)
{
	this->setProperty(MANDATORY_AND_EMPTY, text.isEmpty());
}
