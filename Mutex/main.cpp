#include <thread>
#include <vector>
#include <mutex>

using namespace std;

FILE* pFile = nullptr;
mutex vectorMutex;
vector<size_t> sharedVector;

// Push 1 to 100 into sharedVector
void write_1_to_100(void* threadData)
{
    for(size_t i=1; i<=100; ++i)
    {
        {
            lock_guard<mutex> lock(vectorMutex);
            sharedVector.push_back(i);
        }
        this_thread::sleep_for(1ms);
    }
}

// Write everything in sharedVector to file
void commit_write()
{
    char buffer[64];
    pFile = fopen("myFile.txt", "w");
    for(size_t i = 0; i<sharedVector.size(); ++i)
    {
        sprintf(buffer, "%05zu\n", sharedVector[i]);
        fputs(buffer, pFile);
    }
    fclose(pFile);
}

int main(int argc, char* argv[])
{
    // 2 threads to push 1 to 100 into global variable sharedVector at the same time
    thread thread1(write_1_to_100, nullptr);
    thread thread2(write_1_to_100, nullptr);

    // Wait for both thread to finish
    thread1.join();
    thread2.join();

    // Start writing
    commit_write();
    return 0;
}