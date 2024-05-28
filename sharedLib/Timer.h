/**************************************************
 * TimedEvent, based on steady_clock
 **************************************************/
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <map>
#include <thread>
#include <vector>

class Timer
{
public:
    class EventId
    {
        friend class Timer;
    public:
        uint32_t get_int_value() { return myId; }
    private:
        EventId(uint32_t newId) : myId(newId) {}
        uint32_t myId;
    };
    class EventCatcher
    {
    public:
        virtual void catch_timed_event(uint32_t eventId, std::chrono::time_point<std::chrono::steady_clock> eventTime) {}
    };
    Timer();
    ~Timer();
    EventId add_event(
        std::weak_ptr<EventCatcher> listener,
        std::chrono::time_point<std::chrono::steady_clock> eventTime,
        std::chrono::milliseconds repeatInterval = std::chrono::milliseconds(0));
    bool remove_event(EventId eventId);
    void start();
    void stop();

private:
    static void process(Timer *theTimer);

    struct EventData
    {
        EventData(std::weak_ptr<EventCatcher> inCatcher, std::chrono::milliseconds inInterval, uint32_t inId)
            : catcher(inCatcher), interval(inInterval), myId(inId) {}
        std::weak_ptr<EventCatcher> catcher;
        std::chrono::milliseconds interval;
        uint32_t myId;
    };
    bool mTerminate = false;
    std::mutex mCVMutex;
    std::condition_variable mConditionVariable;
    std::mutex mDataMutex;
    std::multimap<std::chrono::time_point<std::chrono::steady_clock>, EventData> mEventDataMap;
    uint32_t nextId = 1;
    std::vector<uint32_t> freeId;
    std::thread *threadProcess;
};