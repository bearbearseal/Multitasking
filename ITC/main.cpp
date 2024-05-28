#include "../sharedLib/ServerClientItc.h"
#include <thread>

using namespace std;

vector<size_t> sharedVector;

constexpr size_t ONE_HUNDRED_MILLION = 100000000ull;
constexpr size_t LOOP_COUNT = 3;

void loop_through_vector(const vector<size_t> &content)
{
    char buffer[64];
    for (size_t i = 0, max = content.size(); i < content.size(); ++i)
    {
        sprintf(buffer, "%zu\n", content[i]);
    }
}

void push_100Million(void *threadData)
{
    unique_ptr<ServerClientItc<vector<size_t>, bool>::ClientSocket> *ppSocket =
        static_cast<unique_ptr<ServerClientItc<vector<size_t>, bool>::ClientSocket> *>(threadData);

    for (size_t i = 0; i < LOOP_COUNT; ++i)
    {
        vector<size_t> message;
        for (size_t i = 0; i < ONE_HUNDRED_MILLION; ++i)
        {
            message.push_back(i);
        }
        (*ppSocket)->send_message(move(message));
    }
}

int main(int argc, char *argv[])
{
    // Create an ITC (Inter-Thread Communicator)
    // Get "Socket" from newly created ITC
    // The itc socket mimic the idea of socket to pass message around
    // Server would pass vector<size_t> to Client,
    // Client would not pass anything to Server, use anything for 2nd template
    ServerClientItc<vector<size_t>, bool> itc;
    auto serverSocket = itc.get_server_socket();
    auto clientSocket = itc.get_client_socket();

    // threadA would pass message to main thread through client socket
    thread threadA(push_100Million, &clientSocket);

    size_t loopCount = 0;
    while (loopCount < LOOP_COUNT)
    {
        // Main thread wait for message from threadA,
        // It uses condition_variable wait under the hood, no cpu hogging
        serverSocket->wait_message();

        // Message arrived, get the vector which would appear as message.value()
        auto messagePair = serverSocket->get_message();
        loop_through_vector(messagePair.value().message);
        ++loopCount;
    }
    threadA.join();
    return 0;
}
