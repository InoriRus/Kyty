#ifndef CONFIGURATION_ITEM_H
#define CONFIGURATION_ITEM_H

#include "Common.h"

#include <QListWidgetItem>

namespace Kyty {
class Configuration;
} // namespace Kyty

class ConfigurationItem: public QListWidgetItem
{
public:
	explicit ConfigurationItem(Kyty::Configuration* info, QListWidget* parent);
	~ConfigurationItem() override;

	void Update();

	KYTY_QT_CLASS_NO_COPY(ConfigurationItem);

	Kyty::Configuration*                     GetInfo() { return m_info; }
	[[nodiscard]] const Kyty::Configuration* GetInfo() const { return m_info; }
	// void                                     SetInfo(Kyty::Configuration* info);
	void               SetRunning(bool state);
	[[nodiscard]] bool IsRunning() const { return m_running; }

private:
	Kyty::Configuration* m_info    = nullptr;
	bool                 m_running = false;
};

#endif // CONFIGURATION_ITEM_H
