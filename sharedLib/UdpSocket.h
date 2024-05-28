/*
 * UdpSocket.h
 *
 *  Created on: Mar 14, 2018
 *      Author: tzeyang
 */

#ifndef UDPSOCKET_H_
#define UDPSOCKET_H_
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <utility>

#define SOCKET int
#define INVALID_SOCKET -1

class UdpSocket
{
public:
	class Address
	{
		friend class UdpSocket;

	public:
		Address() {}
		Address(const sockaddr_in6 &inAddress) : hisAddress(inAddress) {}
		std::pair<std::string, uint16_t> to_address_and_port() const
		{
			return to_ip_and_port(*this);
		}
		const sockaddr_in6 &get_raw() const { return hisAddress; }
		bool is_ipv4() const;
		bool is_ipv6() const;
		// bool operator==(const Address& theOther);
	private:
		struct sockaddr_in6 hisAddress;
	};

	UdpSocket(bool blocking = true);
	~UdpSocket();
	static Address to_address(const std::string &hostAddr, uint16_t portNumber);
	static std::pair<std::string, uint16_t> to_ip_and_port(const Address &address);
	bool listen(uint16_t portNumber);
	void close();
	void set_destination(const Address &address);
	// bool set_destination(const std::string& hostAddr, uint16_t portNumber);
	std::string read();
	// std::string read(std::string& sourceIp, uint16_t& sourcePortNumber);
	std::string read(Address &hisAddr);
	int16_t write(const std::string &message);
	int16_t write(const std::string &message, const Address &address);
	// int16_t write(const std::string& message, const std::string& ipAddress, uint16_t portNumber);

private:
	SOCKET socketFd;
	// Address hisAddress;
	struct sockaddr_in6 hisAddress;
	// sockaddr_in destinationAddress;
};

#endif /* UDPSOCKET_H_ */
