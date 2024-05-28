/*
 * TcpSocket.cpp
 *
 *  Created on: Jun 27, 2019
 *      Author: lian
 */

#include "TcpSocket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>

using namespace std;

TcpSocket::TcpSocket(bool blocking)
{
	socketFd = -1;
	serverAddress = NULL;
	block = blocking;
}

TcpSocket::~TcpSocket()
{
	if (socketFd != -1)
	{
		::close(socketFd);
	}
	if (serverAddress != NULL)
	{
		freeaddrinfo(serverAddress);
	}
}

void TcpSocket::set_host(const std::string &host, uint16_t portNumber)
{
	if (serverAddress != NULL)
	{
		freeaddrinfo(serverAddress);
	}
	struct addrinfo hints;
	memset(&hints, 0x00, sizeof(hints));
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int rc;
	struct in6_addr serverAddr;
	rc = inet_pton(AF_INET, host.c_str(), &serverAddr);
	if (rc == 1)
	{
		hints.ai_family = AF_INET;
		hints.ai_flags |= AI_NUMERICHOST;
	}
	else
	{
		rc = inet_pton(AF_INET6, host.c_str(), &serverAddr);
		if (rc == 1)
		{
			hints.ai_family = AF_INET6;
			hints.ai_flags |= AI_NUMERICHOST;
		}
	}
	rc = getaddrinfo(host.c_str(), std::to_string(portNumber).c_str(), &hints, &serverAddress);
	hisAddress.sin6_family = serverAddress->ai_family;
	if (hisAddress.sin6_family == AF_INET)
	{
		struct sockaddr_in *addr;
		addr = (struct sockaddr_in *)serverAddress->ai_addr;
		memcpy(&hisAddress.sin6_addr, &addr->sin_addr, sizeof(hisAddress.sin6_addr) > sizeof(addr->sin_addr) ? sizeof(addr->sin_addr) : sizeof(hisAddress.sin6_addr));
		hisAddress.sin6_port = addr->sin_port;
	}
	else if (hisAddress.sin6_family == AF_INET6)
	{
		struct sockaddr_in6 *addr;
		addr = (struct sockaddr_in6 *)serverAddress->ai_addr;
		hisAddress.sin6_addr = addr->sin6_addr;
		hisAddress.sin6_port = addr->sin6_port;
	}
}

bool TcpSocket::open()
{
	if (serverAddress == NULL)
	{
		printf("Server Address not known.\n");
		return false;
	}
	if (socketFd != -1)
	{
		::close(socketFd);
		socketFd = -1;
	}
	socketFd = socket(serverAddress->ai_family, serverAddress->ai_socktype, serverAddress->ai_protocol);
	if (socketFd == -1)
	{
		printf("Could not create socket");
		return false;
	}
	int optionValue = 1;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue)))
	{
		printf("Warning, set socket reuse address failed.\n");
	}
	if (!block)
	{
		fcntl(socketFd, F_SETFL, O_NONBLOCK);
	}
	if (connect(socketFd, serverAddress->ai_addr, serverAddress->ai_addrlen) < 0)
	{
		switch (errno)
		{
		case EINPROGRESS:
			return true;
		case EISCONN:
			return true;
		default:
			return false;
		}
	}
	return true;
}

bool TcpSocket::open(const std::string &host, uint16_t portNumber)
{
	set_host(host, portNumber);
	return open();
}

bool TcpSocket::connection_established() const
{
	if (connect(socketFd, serverAddress->ai_addr, serverAddress->ai_addrlen) < 0)
	{
		if (errno == EISCONN)
		{
			return true;
		}
		return false;
	}
	return true;
}

bool TcpSocket::listen(uint16_t portNumber)
{
	if (socketFd == -1)
	{
		socketFd = socket(AF_INET6, SOCK_STREAM, 0);
		if (socketFd == -1)
		{
			printf("Could not create socket");
			return false;
		}
		int optionValue = 1;
		if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue)))
		{
			printf("Warning, set socket reuse address failed.\n");
		}
	}
	memset(&hisAddress, 0, sizeof(hisAddress));
	hisAddress.sin6_family = AF_INET6;
	hisAddress.sin6_addr = in6addr_any;
	hisAddress.sin6_port = htons(portNumber);
	if (!block)
	{
		fcntl(socketFd, F_SETFL, O_NONBLOCK);
	}
	if (bind(socketFd, (struct sockaddr *)&hisAddress, sizeof(hisAddress)) < 0)
	{
		printf("bind failed. Error.\n");
		return false;
	}
	::listen(socketFd, 1024);
	return true;
}

bool TcpSocket::accept()
{
	unsigned addressSize = sizeof(hisAddress);
	int newSocketFd = ::accept(socketFd, (sockaddr *)&hisAddress, &addressSize);
	if (newSocketFd < 0)
	{
		return false;
	}
	::close(socketFd);
	socketFd = newSocketFd;
	if (!block)
	{
		fcntl(socketFd, F_SETFL, O_NONBLOCK);
	}
	return true;
}

bool TcpSocket::accept(TcpSocket &newSocket)
{
	newSocket.socketFd = -1;
	unsigned addressSize = sizeof(newSocket.hisAddress);
	newSocket.socketFd = ::accept(socketFd, (sockaddr *)&newSocket.hisAddress, &addressSize);
	if (!block)
	{
		fcntl(newSocket.socketFd, F_SETFL, O_NONBLOCK);
	}
	if (newSocket.socketFd != -1)
	{
		return true;
	}
	return false;
}

bool TcpSocket::write(const std::string &data) const
{
	ssize_t reply = send(socketFd, data.c_str(), data.size(), MSG_NOSIGNAL);
	if (reply < ssize_t(data.size()))
		return false;
	return true;
}

bool TcpSocket::write(const char* format, ...) const
{
	char buffer[8192];
	ssize_t length = 0;
	va_list vl;
	va_start(vl, format);
	length = vsnprintf(buffer, sizeof(buffer), format, vl);
	va_end(vl);
	if(length < 0)
		return false;
	ssize_t reply = send(socketFd, buffer, length, MSG_NOSIGNAL);
	if (reply < length)
		return false;
	return true;
}

pair<bool, string> TcpSocket::read(bool dontWait)
{
	pair<bool, string> retVal;
	char receiver[1024];
	int length;
	int flags;
	if (dontWait)
	{
		flags = MSG_DONTWAIT;
	}
	else
	{
		flags = 0;
	}
	do
	{
		length = recv(socketFd, receiver, sizeof(receiver), flags);
		if (length < 0)
		{
			//if also non-blocking
			switch (errno)
			{
			case EWOULDBLOCK:
				retVal.first = true;
				return retVal;
			}
			retVal.first = false;
			return retVal;
		}
		else if (length > 0)
		{
			retVal.second.append(receiver, length);
		}
		else //peer closed socket
		{
			retVal.first= false;
			printf("Peer closed socket.\n");
			return retVal;
		}
	} while (length == sizeof(receiver));
	retVal.first = true;
	return retVal;
}

bool TcpSocket::is_valid()
{
	if (socketFd == -1)
	{
		return false;
	}
	return true;
}

bool TcpSocket::connected()
{
	char buffer;
	int err = recv(socketFd, &buffer, sizeof(buffer), MSG_PEEK);
	if (err < 0)
	{
		//printf("Error number: %d %d.\n", err, errno);
		switch (errno)
		{
		case EWOULDBLOCK:
			return true;
		}
	}
	return false;
}

void TcpSocket::close()
{
	::close(socketFd);
	socketFd = -1;
}

std::string TcpSocket::get_his_ip()
{
	char retVal[64];
	memset(retVal, 0x00, sizeof(retVal));
	inet_ntop(((sockaddr *)&hisAddress)->sa_family, &hisAddress.sin6_addr, retVal, sizeof(retVal));
	return retVal;
}

uint16_t TcpSocket::get_his_port()
{
	return ntohs(hisAddress.sin6_port);
}
