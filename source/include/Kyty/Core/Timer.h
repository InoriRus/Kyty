#ifndef INCLUDE_KYTY_CORE_TIMER_H_
#define INCLUDE_KYTY_CORE_TIMER_H_

#include "Kyty/Core/Common.h"

namespace Kyty::Core {

class Timer final
{
public:
	Timer() noexcept;
	~Timer() = default;

	void Start();

	void Pause();

	void Resume();

	[[nodiscard]] bool IsPaused() const;

	// return time in milliseconds
	[[nodiscard]] double GetTimeMs() const;

	// return time in seconds
	[[nodiscard]] double GetTimeS() const;

	// return time in ticks
	[[nodiscard]] uint64_t GetTicks() const;

	// return ticks frequency
	[[nodiscard]] uint64_t GetFrequency() const;

	KYTY_CLASS_NO_COPY(Timer);

	[[nodiscard]] static uint64_t QueryPerformanceFrequency();
	[[nodiscard]] static uint64_t QueryPerformanceCounter();

private:
	bool     m_is_paused = true;
	uint64_t m_Frequency = 0;
	uint64_t m_StartTime = 0;
	uint64_t m_PauseTime = 0;
};

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_TIMER_H_ */
