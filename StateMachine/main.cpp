#include "../sharedLib/TcpSocket.h"
#include "../sharedLib/UdpSocket.h"
#include "unistd.h"

using namespace std;

enum class TcpState
{
    OPEN,
    CONNECT,
    WRITE,
    WRITE_DELAY
};

struct TcpProcessData
{
    TcpProcessData() : state(TcpState::OPEN), tcpSocket(false), i(0) {}
    TcpState state;
    TcpSocket tcpSocket;
    size_t i;
    size_t delayCounter;
};

void tcp_process(TcpProcessData& data, uint16_t portNum)
{
    switch (data.state)
    {
    case TcpState::OPEN:
        data.tcpSocket.open("127.0.0.1", portNum);
        data.state = TcpState::CONNECT;
        break;
    case TcpState::CONNECT:
        if (data.tcpSocket.connection_established())
        {
            data.state = TcpState::WRITE;
        }
        break;
    case TcpState::WRITE:
        if (data.tcpSocket.write("Hello %zu\n", data.i))
        {
            ++data.i;
            data.state = TcpState::WRITE_DELAY;
            data.delayCounter = 400;
        }
        else
        {
            data.tcpSocket.close();
            data.state = TcpState::OPEN;
        }
        break;
    case TcpState::WRITE_DELAY:
        --data.delayCounter;
        if (data.delayCounter == 0)
        {
            data.state = TcpState::WRITE;
        }
        break;
    default:
        data.state = TcpState::OPEN;
        break;
    }
}

enum class UdpState
{
    LISTEN,
    READ,
    DELAY
};

struct UdpProcessData
{
    UdpProcessData() : state(UdpState::LISTEN), udpSocket(false), i(0) {}
    UdpState state;
    UdpSocket udpSocket;
    size_t i;
    size_t delayCounter;
};

void udp_process(UdpProcessData& data, uint16_t portNum)
{
    switch (data.state)
    {
    case UdpState::LISTEN:
        data.udpSocket.listen(portNum);
        data.state = UdpState::READ;
        break;
    case UdpState::READ:
    {
        UdpSocket::Address hisAddress;
        auto input = data.udpSocket.read(hisAddress);
        if (!input.empty())
        {
            data.udpSocket.write("Hello " + to_string(data.i) + "\n", hisAddress);
            ++data.i;
        }
        else
        {
            data.state = UdpState::DELAY;
            data.delayCounter = 80;
        }
    }
    break;
    case UdpState::DELAY:
        --data.delayCounter;
        if (data.delayCounter == 0)
        {
            data.state = UdpState::READ;
        }
        break;
    default:
        data.state = UdpState::LISTEN;
        break;
    }
}

int main(int argc, char *argv[])
{
    TcpProcessData tcpProcessData1;
    TcpProcessData tcpProcessData2;
    TcpProcessData tcpProcessData3;
    UdpProcessData udpProcessData;
    while(1)
    {
        tcp_process(tcpProcessData1, uint16_t(8887));
        tcp_process(tcpProcessData2, uint16_t(8888));
        tcp_process(tcpProcessData3, uint16_t(8889));
        udp_process(udpProcessData, uint16_t(8888));
        usleep(10000);
    }
    return 0;
}

