#include <thread>
#include <vector>
#include <mutex>

using namespace std;

// Thread A (main) need to wait till Thread B is done,
// then loop through the vector "produced" by thread B
// Thread B (spawn) need to push till 10 mil number to vector,
// then wait for the numbers to disappear then push again

vector<size_t> sharedVector;
mutex vectorMutex;
constexpr size_t LOOP_COUNT = 3;
constexpr size_t ONE_HUNDRED_MILLION = 100000000ull;

void loop_through_vector(vector<size_t> &content)
{
    char buffer[64];
    for (size_t i(0), theSize(content.size()); i < theSize; ++i)
        sprintf(buffer, "%zu\n", content[i]);
}

void push_numbers(void *threadData)
{
    for (size_t i = 0; i < LOOP_COUNT; ++i)
    {
        // Produce data
        for (size_t i = 0; i < ONE_HUNDRED_MILLION; ++i)
        {
            lock_guard<mutex> lock(vectorMutex);
            sharedVector.push_back(i);
        }
        // Wait for data to be consumed
        while (1)
        {
            {
                lock_guard<mutex> lock(vectorMutex);
                if (sharedVector.empty())
                {
                    break;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    vector<size_t> copy;
    thread threadA(push_numbers, nullptr);
    size_t loopCount = 0;
    while (loopCount < LOOP_COUNT)
    {
        // Wait for data to be produced
        {
            lock_guard<mutex> lock(vectorMutex);
            if (sharedVector.size() >= ONE_HUNDRED_MILLION)
            {
                copy = move(sharedVector);
            }
        }
        if (!copy.empty())
        {
            // Consume data
            loop_through_vector(copy);
            copy.clear();
            ++loopCount;
        }
    }
    threadA.join();
    return 0;
}
