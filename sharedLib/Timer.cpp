#include "Timer.h"

using namespace std;

Timer::Timer()
{
}

Timer::~Timer()
{
    stop();
}

Timer::EventId Timer::add_event(
    weak_ptr<EventCatcher> listener, chrono::time_point<chrono::steady_clock> eventTime, chrono::milliseconds repeatInterval)
{
    uint32_t eventId;
    {
        lock_guard<mutex> lock(mDataMutex);
        if (!freeId.empty())
        {
            eventId = *freeId.rbegin();
            freeId.pop_back();
        }
        else
        {
            eventId = nextId;
            ++nextId;
        }
        if (mEventDataMap.emplace(eventTime, EventData(listener, repeatInterval, eventId)) == mEventDataMap.begin())
        {
            lock_guard<mutex> lock(mCVMutex);
            mConditionVariable.notify_one();
        }
    }
    return EventId(eventId);
}

bool Timer::remove_event(EventId eventId)
{
    lock_guard<mutex> lock(mDataMutex);
    for (auto it = mEventDataMap.begin(); it != mEventDataMap.end(); ++it)
    {
        if (it->second.myId == eventId.myId)
        {
            mEventDataMap.erase(it);
            return true;
        }
    }
    return false;
}

void Timer::start()
{
    threadProcess = new thread(process, this);
}

void Timer::stop()
{
    if (threadProcess != nullptr)
    {
        mTerminate = true;
        threadProcess->join();
        delete threadProcess;
        threadProcess = nullptr;
    }
}

void Timer::process(Timer *me)
{
    while (!me->mTerminate)
    {
        {
            unique_lock<mutex> lock(me->mCVMutex);
            me->mConditionVariable.wait_until(lock, me->mEventDataMap.begin()->first);
        }
        // Lock event vector
        {
            auto now = chrono::steady_clock::now();
            lock_guard<mutex> lock(me->mDataMutex);
            if (!me->mEventDataMap.empty())
            {
                auto theOne = me->mEventDataMap.begin();
                while (theOne->first <= now)
                {
                    auto sharedP = theOne->second.catcher.lock();
                    if (sharedP != nullptr)
                    {
                        // Notify listener
                        sharedP->catch_timed_event(theOne->second.myId, theOne->first);
                        if (theOne->second.interval != 0ms)
                        {
                            // Reinsert the repeating
                            auto theNode = me->mEventDataMap.extract(theOne);
                            me->mEventDataMap.insert({theNode.key() + theNode.mapped().interval, theNode.mapped()});
                        }
                        else
                        {
                            // Remove if not repeating
                            me->mEventDataMap.erase(theOne);
                        }
                    }
                    else
                    {
                        // The catcher is gone
                        me->mEventDataMap.erase(theOne);
                    }
                    if (!me->mEventDataMap.empty())
                        theOne = me->mEventDataMap.begin();
                    else
                        break;
                }
            }
        }
    }
}