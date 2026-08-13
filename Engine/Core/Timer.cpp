#include "Timer.h"

namespace Jevaing::Internal
{
    Timer::Timer()
    {
        Reset();
    }

    void Timer::Reset()
    {
        m_start = Clock::now();
        m_last = m_start;
        m_deltaTime = 0.0;
    }

    double Timer::Tick()
    {
        const Clock::time_point now = Clock::now();
        m_deltaTime = std::chrono::duration<double>(now - m_last).count();
        m_last = now;

        return m_deltaTime;
    }

    double Timer::GetDeltaTime() const
    {
        return m_deltaTime;
    }

    double Timer::GetElapsedTime() const
    {
        return std::chrono::duration<double>(Clock::now() - m_start).count();
    }
}
