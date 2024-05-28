/*
 * TcpSocket.hpp
 *
 *  Created on: Jun 27, 2019
 *      Author: lian
 */

#ifndef TCPSOCKET_HPP_
#define TCPSOCKET_HPP_

#include <stdint.h>
#include <string>
#include <arpa/inet.h> //inet_addr
#include <tuple>

class TcpSocket {
public:
	enum class Status{
		SOCKET_CLOSED = -1,
		NO_ERROR = 0,
		
	};
	TcpSocket(bool blocking = true);
	virtual ~TcpSocket();

	void set_host(const std::string& host, uint16_t portNumber);
	bool open();
	bool open(const std::string& host, uint16_t portNumber);
	bool connection_established() const;

	bool listen(uint16_t portNumber);
	bool accept();
	bool accept(TcpSocket& newSocket);

	bool write(const std::string& data) const;
	bool write(const char* format, ...) const;
	std::pair<bool, std::string> read(bool dontWait = false);

	bool is_valid();
	bool connected();

	void close();

	std::string get_his_ip();
	uint16_t get_his_port();

private:
    int socketFd;
    bool block;
    struct sockaddr_in6 hisAddress;
	struct addrinfo* serverAddress;
};




#endif /* TCPSOCKET_HPP_ */
