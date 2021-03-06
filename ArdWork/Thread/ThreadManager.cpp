/*--------------------------------------------------------------------
Thread is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Thread is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

See GNU Lesser General Public License at <http://www.gnu.org/licenses/>.
--------------------------------------------------------------------*/

#include <Arduino.h>

#include "Thread.h"
#include "ThreadManager.h"

#if defined(ARDUINO_ARCH_ESP8266)
extern "C"
{
#include <user_interface.h>
}
#elif defined(__arm__)

#elif defined(ARDUINO_ARCH_AVR)
#include <avr/power.h>
#endif

#define USE_WDT

ThreadManager::ThreadManager() :
        _pFirstThread( NULL ),
        _pLastThread( NULL )
{

#if defined(ARDUINO_ARCH_AVR) && !defined(__arm__)
    // make sure the watch dog is disabled during setup
    // avoid this for Esp8266 due to it will only disable the software watchdog
    // but leave the hardware one to fire, further this would disable all the
    // built in hidden watchdog feed calls that would keep it from firing and
    // thus causing an effect of enabling the watchdog rather than disabling
    wdt_reset();
    wdt_disable();
#endif

    _lastTick = GetThreadTime();
}

void ThreadManager::Setup()
{
    _lastTick = GetThreadTime();
}

void ThreadManager::StartThread(Thread* pThread)
{
    if (pThread->_taskState != ThreadState_Running)
    {
        // check if it has been stopped yet as it may be just stopping
        if (pThread->_taskState == ThreadState_Stopped)
        {
            // append to the list
            if (_pFirstThread == NULL)
            {
                _pFirstThread = pThread;
                _pLastThread = pThread;
            }
            else
            {
                _pLastThread->_pNext = pThread;
                _pLastThread = pThread;
            }
        }
        pThread->Start();
    }
}

void ThreadManager::StopThread(Thread* pThread)
{
    pThread->Stop();
}

ThreadState ThreadManager::StatusThread(Thread* pThread)
{
    return pThread->_taskState;
}

void ThreadManager::Loop(uint16_t watchdogTimeOutFlag)
{
    uint32_t currentTick = GetThreadTime();
    uint32_t deltaTime = currentTick - _lastTick;

    if (deltaTime >= ThreadTimeAccuracy)
    {
        _lastTick = currentTick; // update before calling process
        uint32_t nextWakeTime = ProcessThreads(deltaTime);

        RemoveStoppedThreads();

        // if the next task has more time available than the next
        // millisecond interupt, then sleep
        if (nextWakeTime > ThreadTimePerMs)
        {
            // for idle sleep mode:
            // due to Millis() using timer interupt at 1 ms,
            // the cpu will be woke up by that every millisecond

#if defined(ARDUINO_ARCH_ESP8266)
            // the esp8266 really doesn't have an idle mode
#if defined(USE_WDT)
            // use watchdog timer for failsafe mode,
            // total task update time should be less than watchdogTimeOutFlag
            wdt_disable();
            wdt_enable(watchdogTimeOutFlag);
#endif

#elif defined(__arm__)
                // Arm support for sleep/idle not implemented yet

#elif defined(ARDUINO_ARCH_AVR)

#if defined(USE_WDT)
            // use watchdog timer for failsafe mode,
            // total task update time should be less than watchdogTimeOutFlag
            wdt_reset();
            wdt_enable(watchdogTimeOutFlag);
#endif

            // just sleep
            set_sleep_mode(SLEEP_MODE_IDLE);
            cli();
            sleep_enable();
#if defined(BODSE)
            // lower power trick
            // sleep_bod_disable() - i have seen this method called, but can't find it
            MCUCR |= _BV(BODS) | _BV(BODSE);  // turn on brown-out enable select
            MCUCR &= ~_BV(BODSE);        // this must be done within 4 clock cycles of above
#endif
            sei();
            sleep_cpu(); // will sleep in this call
            sleep_disable();
#endif // Arduino Normal
        }
#if defined(USE_WDT)
        else
        {
#if !defined(__arm__) // no arm support for watchdog
            wdt_reset(); // keep the dog happy
#endif
        }
#endif

    }
}

#if defined(ARDUINO_ARCH_ESP8266)
#define RTC_MEM_SLEEP_ADDR 65 // 64 is being overwritten right now

void ThreadManager::EnterSleep(uint32_t microSeconds,
    void* state,
    uint16_t sizeofState,
    WakeMode mode)
{
    if (state != NULL && sizeofState > 0)
    {
        system_rtc_mem_write(RTC_MEM_SLEEP_ADDR, state, sizeofState);
    }
    ESP.deepSleep(microSeconds, mode);
}

bool ThreadManager::RestartedFromSleep(void* state, uint16_t sizeofState)
{
    rst_info* resetInfo = ESP.getResetInfoPtr();
    bool wasSleeping = (resetInfo && REASON_DEEP_SLEEP_AWAKE == resetInfo->reason);
    if (wasSleeping)
    {
        if (state != NULL && sizeofState > 0)
        {
            system_rtc_mem_read(RTC_MEM_SLEEP_ADDR, state, sizeofState);
        }
    }
    return wasSleeping;
}

#elif defined(__arm__)
// Arm support for sleep not implemented yet


#elif defined(ARDUINO_ARCH_AVR)

void ThreadManager::EnterSleep(uint8_t sleepMode)
{
#if defined(USE_WDT)
    // disable watchdog so it doesn't wake us up
    wdt_reset();
    wdt_disable();
#endif
    // prepare sleep
    set_sleep_mode(sleepMode);
    cli();
    sleep_enable();

#if defined(BODSE)
    // lower power trick
    // sleep_bod_disable() - i have seen this method called, but can't find it
    MCUCR |= _BV(BODS) | _BV(BODSE);  // turn on brown-out enable select
    MCUCR &= ~_BV(BODSE);        // this must be done within 4 clock cycles of above
#endif

    sei();
    sleep_cpu(); // will sleep in this call
    sleep_disable();

#if defined(USE_WDT)
    // enable watch dog after wake up
    wdt_reset();
    wdt_enable(WDTO_500MS);
#endif
}
#endif

uint32_t ThreadManager::ProcessThreads(uint32_t deltaTime)
{
    uint32_t nextWakeTime = ((uint32_t)-1); // MAX_UINT32

    // Update Threads
    //
    Thread* pIterate = _pFirstThread;
    while (pIterate != NULL)
    {
        // skip any non running tasks
        if (pIterate->_taskState == ThreadState_Running)
        {
            if (pIterate->_remainingTime <= deltaTime)
            {
                // calc per task delta time
                uint32_t taskDeltaTime = pIterate->_timeInterval - pIterate->_remainingTime;
                taskDeltaTime += deltaTime;

                // add the initial time so we don't loose any remainders
                pIterate->_remainingTime += pIterate->_timeInterval;
                // if we are still less than delta time, things are running slow
                // so push to the next update frame
                if (pIterate->_remainingTime <= deltaTime)
                {
                    pIterate->_remainingTime = deltaTime + ThreadTimeAccuracy;
                }

                pIterate->OnUpdate(taskDeltaTime);
            }

            uint32_t newRemainingTime = pIterate->_remainingTime - deltaTime;
            pIterate->_remainingTime = newRemainingTime;

            if (newRemainingTime < nextWakeTime)
            {
                nextWakeTime = newRemainingTime;
            }
        }

        pIterate = pIterate->_pNext;
    }
    return nextWakeTime;
}

void ThreadManager::RemoveStoppedThreads()
{
    // walk task list and remove stopped tasks
    //
    Thread* pIterate = _pFirstThread;
    Thread* pIteratePrev = NULL;
    while (pIterate != NULL)
    {
        Thread* pNext = pIterate->_pNext;
        if (pIterate->_taskState == ThreadState_Stopping)
        {
            // Remove it
            pIterate->_taskState = ThreadState_Stopped;
            pIterate->_pNext = NULL;

            if (pIterate == _pFirstThread)
            {
                // first one, correct our first pointer
                _pFirstThread = pNext;
                if (pIterate == _pLastThread)
                {
                    // last one, correct our last pointer
                    _pLastThread = _pFirstThread;
                }
            }
            else
            {
                // all others correct the previous to remove it
                pIteratePrev->_pNext = pNext;
                if (pIterate == _pLastThread)
                {
                    // last one, correct our last pointer
                    _pLastThread = pIteratePrev;
                }
            }
        }
        else
        {
            // didn't remove, advance the previous pointer
            pIteratePrev = pIterate;
        }
        pIterate = pNext; // iterate to the next
    }
}
