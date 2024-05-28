#include "../sharedLib/TcpSocket.h"
#include "../sharedLib/UdpSocket.h"
#include "unistd.h"

using namespace std;

void udp_socket_process(UdpSocket& udpSocket, uint16_t portNum)
{
    size_t i = 0;
    UdpSocket::Address hisAddress;
    udpSocket.listen(portNum);
    while (1)
    {
        auto input = udpSocket.read(hisAddress);
        if (!input.empty())
        {
            udpSocket.write("Hello " + to_string(i) + "\n", hisAddress);
            ++i;
        }
        else
            sleep(1);
    }
}

void tcp_socket_process(TcpSocket& tcpSocket, uint16_t portNum)
{
    while (1)
    {
        tcpSocket.open("127.0.0.1", portNum);
        while (!tcpSocket.connection_established())
        {
            usleep(10000);
        }
        size_t i = 0;
        while (tcpSocket.write("Hello %zu\n", i))
        {
            sleep(5);
            ++i;
        }
        tcpSocket.close();
        sleep(1);
    }
}

int main(int argc, char *argv[])
{
    TcpSocket tcpSocket(false);
    UdpSocket udpSocket(false);
    udp_socket_process(udpSocket, uint16_t(8888));
    tcp_socket_process(tcpSocket, uint16_t(8888));
    return 0;
}

