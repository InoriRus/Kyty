#include "Kyty/Core/Timer.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Sys/SysTimer.h"

namespace Kyty::Core {

Timer::Timer() noexcept
{
	sys_query_performance_frequency(&m_Frequency);
}

void Timer::Start()
{
	sys_query_performance_counter(&m_StartTime);
	m_is_paused = false;
}

void Timer::Pause()
{
	EXIT_IF(m_is_paused);

	sys_query_performance_counter(&m_PauseTime);
	m_is_paused = true;
}

void Timer::Resume()
{
	EXIT_IF(!m_is_paused);

	uint64_t current_time = 0;
	sys_query_performance_counter(&current_time);

	m_StartTime += current_time - m_PauseTime;

	m_is_paused = false;
}

bool Timer::IsPaused() const
{
	return m_is_paused;
}

// return time in milliseconds
double Timer::GetTimeMs() const
{
	if (m_is_paused)
	{
		return 1000.0 * (static_cast<double>(m_PauseTime - m_StartTime)) / static_cast<double>(m_Frequency);
	}

	uint64_t current_time = 0;
	sys_query_performance_counter(&current_time);

	return 1000.0 * (static_cast<double>(current_time - m_StartTime)) / static_cast<double>(m_Frequency);
}

// return time in seconds
double Timer::GetTimeS() const
{
	if (m_is_paused)
	{
		return (static_cast<double>(m_PauseTime - m_StartTime)) / static_cast<double>(m_Frequency);
	}

	uint64_t current_time = 0;
	sys_query_performance_counter(&current_time);

	return (static_cast<double>(current_time - m_StartTime)) / static_cast<double>(m_Frequency);
}

// return time in ticks
uint64_t Timer::GetTicks() const
{
	if (m_is_paused)
	{
		return (m_PauseTime - m_StartTime);
	}

	uint64_t current_time = 0;
	sys_query_performance_counter(&current_time);

	return (current_time - m_StartTime);
}

// return ticks frequency
uint64_t Timer::GetFrequency() const
{
	return m_Frequency;
}

uint64_t Timer::QueryPerformanceFrequency()
{
	uint64_t ret = 0;
	sys_query_performance_frequency(&ret);
	return ret;
}

uint64_t Timer::QueryPerformanceCounter()
{
	uint64_t ret = 0;
	sys_query_performance_counter(&ret);
	return ret;
}

} // namespace Kyty::Core
