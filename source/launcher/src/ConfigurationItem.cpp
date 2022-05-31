#include "ConfigurationItem.h"

#include "Configuration.h"

#include <QFont>
#include <QIcon>
#include <QSize>

ConfigurationItem::ConfigurationItem(Kyty::Configuration* info, QListWidget* parent): QListWidgetItem(parent), m_info(info)
{
	setSizeHint(QSize(0, 18));
	if (m_info != nullptr)
	{
		setText(m_info->name);
	}
	SetRunning(false);
}

ConfigurationItem::~ConfigurationItem()
{
	delete m_info;
}

void ConfigurationItem::Update()
{
	if (m_info != nullptr)
	{
		setText(m_info->name);
	}
}

void ConfigurationItem::SetRunning(bool state)
{
	m_running = state;

	QFont f = font();
	f.setBold(state);
	setFont(f);

	if (m_running)
	{
		setIcon(QIcon(":/running"));
	} else
	{
		setIcon(QIcon(":/application"));
	}
}
