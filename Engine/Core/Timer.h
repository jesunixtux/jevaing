#pragma once

#include <chrono>

namespace Jevaing::Internal
{
    class Timer
    {
    public:
        Timer();

        void Reset();
        double Tick();

        double GetDeltaTime() const;
        double GetElapsedTime() const;

    private:
        using Clock = std::chrono::steady_clock;

        Clock::time_point m_start;
        Clock::time_point m_last;
        double m_deltaTime = 0.0;
    };
}
