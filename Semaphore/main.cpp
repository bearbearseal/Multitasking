#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

using namespace std;

vector<size_t> sharedVector;
// Mutex is needed to synchronize the wait and notify process

mutex readyMutex, movedMutex;
condition_variable readyCv, movedCv;

constexpr size_t ONE_HUNDRED_MILLION = 100000000ull;
constexpr size_t LOOP_COUNT = 3;

// Thread A (main) need to wait till Thread B is done,
// then loop through the vector "produced" by thread B
// Thread B (spawn) need to push till 10 mil number to vector,
// then wait for the numbers to disappear then push again

void loop_through_vector(vector<size_t> &content)
{
    char buffer[64];
    for (size_t i = 0; i < content.size(); ++i)
    {
        sprintf(buffer, "%zu\n", content[i]);
    }
}

void push_100Million(void *threadData)
{
    for (size_t i = 0; i < LOOP_COUNT; ++i)
    {
        {
            // Make sure this thread would not start before the other thread starts waiting
            lock_guard<mutex> lock(readyMutex);
        }
        for (size_t i = 0; i < ONE_HUNDRED_MILLION; ++i)
        {
            sharedVector.push_back(i);
        }
        {
            unique_lock<mutex> lock(movedMutex);
            readyCv.notify_one();
            // Atomic operation of unlock readyMutex and start waiting
            movedCv.wait(lock);
            // At this point, mutex is again locked by wait
        }
    }
}

int main(int argc, char *argv[])
{
    vector<size_t> copy;
    // Lock the mutex, dont let the other thread start
    unique_lock<mutex> lock(readyMutex);
    thread threadA(push_100Million, nullptr);
    size_t loopCount = 0;
    while (loopCount < LOOP_COUNT)
    {
        // Atomic operation of unlock readyMutex and start waiting
        readyCv.wait(lock);
        // At this point, mutex is again locked by wait
        copy = move(sharedVector);
        {
            // Make sure the other side had started waiting b4 notifying it
            lock_guard<mutex> lock(movedMutex);
        }
        movedCv.notify_one();
        loop_through_vector(copy);
        ++loopCount;
    }
    threadA.join();
    return 0;
}
