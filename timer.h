/*
 * Copyright 2014 Milian Wolff <mail@milianw.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <cassert>

#include <limits>

class Timer
{
public:
    Timer()
        : m_timesElapsed(0)
        , m_timerId(0)
    {
        /* Establish handler for timer signal */

        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = &Timer::handler;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGRTMIN, &sa, nullptr) == -1) {
            fprintf(stderr, "Failed to call sigaction in %s:%d: %s",
                    __FILE__, __LINE__, strerror(errno));
            return;
        }

        /* Create the timer */

        sigevent sev;
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGRTMIN;
        sev.sigev_value.sival_ptr = this;
        if (timer_create(CLOCK_REALTIME, &sev, &m_timerId) == -1) {
            fprintf(stderr, "Failed to call timer_create in %s:%d: %s",
                    __FILE__, __LINE__, strerror(errno));
            return;
        }
    }

    ~Timer()
    {
        timer_delete(m_timerId);
    }

    size_t timesElapsed() const
    {
        return m_timesElapsed;
    }

    void setInterval(time_t seconds, long long nanoseconds)
    {
        /* Start/Stop the timer */
        itimerspec its;
        its.it_value.tv_sec = seconds;
        its.it_value.tv_nsec = nanoseconds;
        its.it_interval.tv_sec = seconds;
        its.it_interval.tv_nsec = nanoseconds;

        if (timer_settime(m_timerId, 0, &its, nullptr) == -1) {
            fprintf(stderr, "Failed to call timer_settime in %s:%d: %s",
                    __FILE__, __LINE__, strerror(errno));
            return;
        }
    }

private:
    static void handler(int /*sig*/, siginfo_t *si, void */*uc*/)
    {
        auto overrun = si->si_overrun;
        if (overrun < 0) {
            return;
        }

        Timer* timer = static_cast<Timer*>(si->si_value.sival_ptr);
        timer->m_timesElapsed += overrun + 1;
    }

    size_t m_timesElapsed;
    timer_t m_timerId;
};

#endif