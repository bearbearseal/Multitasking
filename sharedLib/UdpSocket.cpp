/*
 * UdpSocket.cpp
 *
 *  Created on: Mar 14, 2018
 *      Author: tzeyang
 */

#include "UdpSocket.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

using namespace std;

UdpSocket::UdpSocket(bool blocking) {
	socketFd=socket(AF_INET6, SOCK_DGRAM, 0);
	if(socketFd==INVALID_SOCKET){
		printf("Could not create UdpSocket.\n");
	}
	if(!blocking) {
		fcntl(socketFd, F_SETFL, O_NONBLOCK);
	}
}

UdpSocket::~UdpSocket() {
	::close(socketFd);
}

UdpSocket::Address UdpSocket::to_address(const std::string& hostAddr, uint16_t portNumber) {
	Address retVal;
	struct addrinfo hints;
	memset(&hints, 0x00, sizeof(hints));
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	int rc;
	struct in6_addr serverAddr;
	rc=inet_pton(AF_INET6, hostAddr.c_str(), &serverAddr);
	if(rc==1) {
		hints.ai_family = AF_INET6;
		hints.ai_flags |= AI_NUMERICHOST;
	}
	else {
		rc = inet_pton(AF_INET, hostAddr.c_str(), &serverAddr);
		if(rc==1) {
			hints.ai_family = AF_INET;
			hints.ai_flags |= AI_NUMERICHOST;
		}
		else {
			printf("Failed!\n");
			return retVal;
		}
	}
	struct addrinfo* destAddress;
	rc = getaddrinfo(hostAddr.c_str(), std::to_string(portNumber).c_str(), &hints, &destAddress);
	retVal.hisAddress.sin6_family = destAddress->ai_family;
	if(retVal.hisAddress.sin6_family == AF_INET) {
		struct sockaddr_in *addr;
		addr = (struct sockaddr_in *)destAddress->ai_addr;
		memcpy(&retVal.hisAddress.sin6_addr, &addr->sin_addr, sizeof(retVal.hisAddress.sin6_addr) > sizeof(addr->sin_addr) ? sizeof(addr->sin_addr) : sizeof(retVal.hisAddress.sin6_addr));
		retVal.hisAddress.sin6_port = addr->sin_port;
		retVal.hisAddress.sin6_family = AF_INET;
		retVal.hisAddress.sin6_flowinfo = 0;
		retVal.hisAddress.sin6_scope_id = 0;
	}
	else if(retVal.hisAddress.sin6_family == AF_INET6) {
		struct sockaddr_in6 *addr;
		addr = (struct sockaddr_in6 *)destAddress->ai_addr;
		retVal.hisAddress.sin6_addr = addr->sin6_addr;
		retVal.hisAddress.sin6_port = addr->sin6_port;
		retVal.hisAddress.sin6_family = AF_INET6;
		retVal.hisAddress.sin6_flowinfo = addr->sin6_flowinfo;
		retVal.hisAddress.sin6_scope_id = addr->sin6_scope_id;
	}
	else {
		freeaddrinfo(destAddress);
		return retVal;
	}
	freeaddrinfo(destAddress);

	return retVal;
}

bool UdpSocket::Address::is_ipv4() const {
	return (hisAddress.sin6_family == AF_INET);
}

bool UdpSocket::Address::is_ipv6() const {
	return (hisAddress.sin6_family == AF_INET6);
}

/*
bool UdpSocket::Address::operator==(const Address& theOther) {
	if(hisAddress.sin6_family == AF_INET6) {
		if(theOther.hisAddress.sin6_family == AF_INET6) {
			if(!memcmp(&hisAddress.sin6_addr, &theOther.hisAddress.sin6_addr, sizeof(hisAddress.sin6_addr))) {
				if(!memcmp(&hisAddress.sin6_port, &theOther.hisAddress.sin6_port, sizeof(hisAddress.sin6_port))) {
					return true;
				}
			}
		}
	}
	else if(hisAddress.sin6_family == AF_INET) {
		if(theOther.hisAddress.sin6_family == AF_INET) {
			if(!memcmp(&hisAddress.sin6_addr, &theOther.hisAddress.sin6_addr, sizeof(hisAddress.sin6_addr))) {
				if(!memcmp(&hisAddress.sin6_port, &theOther.hisAddress.sin6_port, sizeof(hisAddress.sin6_port))) {
					return true;
				}
			}
		}
	}
	return false;
}
* */
/*
size_t UdpSocket::Address::hash() {
	std::hash<string> hasher;
	string hashSubject(&hisAddress.sin6_addr, sizeof(hisAddress.sin6_addr));
	return hasher(hashSubject);
}
*/
std::pair<std::string, uint16_t> UdpSocket::to_ip_and_port(const Address& address) {
	string sourceIp;
	uint16_t sourcePortNumber;
	if(address.hisAddress.sin6_family == AF_INET6) {
		char ipAddress[64];
		inet_ntop(AF_INET6, &address.hisAddress.sin6_addr, ipAddress, sizeof(ipAddress));
		sourceIp = ipAddress;
		sourcePortNumber = ntohs(address.hisAddress.sin6_port);
	}
	else if(address.hisAddress.sin6_family == AF_INET) {
		char ipAddress[32];
		inet_ntop(AF_INET, &address.hisAddress.sin6_addr, ipAddress, sizeof(ipAddress));
		sourceIp = ipAddress;
		sourcePortNumber = ntohs(address.hisAddress.sin6_port);
	}
	return {sourceIp, sourcePortNumber};
}

/*
bool UdpSocket::set_destination(const std::string& hostAddr, uint16_t portNumber){
	struct addrinfo hints;
	memset(&hints, 0x00, sizeof(hints));
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	int rc;
	struct in6_addr serverAddr;
	rc=inet_pton(AF_INET, hostAddr.c_str(), &serverAddr);
	if(rc==1) {
		hints.ai_family = AF_INET;
		hints.ai_flags |= AI_NUMERICHOST;
	}
	else {
		rc = inet_pton(AF_INET6, hostAddr.c_str(), &serverAddr);
		if(rc==1) {
			hints.ai_family = AF_INET6;
			hints.ai_flags |= AI_NUMERICHOST;
		}
		else {
			return false;
		}
	}
	struct addrinfo* destAddress;
	rc = getaddrinfo(hostAddr.c_str(), std::to_string(portNumber).c_str(), &hints, &destAddress);
	hisAddress.sin6_family = destAddress->ai_family;
	if(hisAddress.sin6_family == AF_INET) {
		struct sockaddr_in *addr;
		addr = (struct sockaddr_in *)destAddress->ai_addr;
		memcpy(&hisAddress.sin6_addr, &addr->sin_addr, sizeof(hisAddress.sin6_addr) > sizeof(addr->sin_addr) ? sizeof(addr->sin_addr) : sizeof(hisAddress.sin6_addr));
		hisAddress.sin6_port = addr->sin_port;
	}
	else if(hisAddress.sin6_family == AF_INET6) {
		struct sockaddr_in6 *addr;
		addr = (struct sockaddr_in6 *)destAddress->ai_addr;
		hisAddress.sin6_addr = addr->sin6_addr;
		hisAddress.sin6_port = addr->sin6_port;
	}
	else {
		freeaddrinfo(destAddress);
		return false;
	}
	freeaddrinfo(destAddress);
	return true;
}
*/

bool UdpSocket::listen(uint16_t portNumber) {
	memset(&hisAddress, 0, sizeof(hisAddress));
    hisAddress.sin6_family = AF_INET6;
    hisAddress.sin6_addr = in6addr_any;
    hisAddress.sin6_port = htons(portNumber);
    if(bind(socketFd,(struct sockaddr *) &hisAddress , sizeof(hisAddress)) < 0)
    {
        printf("bind failed. Error.\n");
        return false;
    }
	return true;
	/*
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(portNumber);
	if(bind(socketFd, (sockaddr*) &address, sizeof(address))==-1){
		printf("Bind failed with error code.\n");
		return false;
	}
	fcntl(socketFd, F_SETFL, O_NONBLOCK);
	return true;
	*/
}

void UdpSocket::close() {
	::close(socketFd);
}

void UdpSocket::set_destination(const Address& address) {
	hisAddress = address.hisAddress;
}

std::string UdpSocket::read()
{
	static socklen_t addressSize = sizeof(sockaddr_in6);
	std::string retVal;
	sockaddr_in6 sourceAddress;
	int length;
	char recvBuffer[64*1024];
	length = recvfrom(socketFd, recvBuffer, sizeof(recvBuffer)-1, 0, (sockaddr*)&sourceAddress, &addressSize);
	if(length!=-1){
		retVal.assign(recvBuffer, length);
	}
	return retVal;
}
/*
std::string UdpSocket::read(std::string& sourceIp, uint16_t& sourcePortNumber){
	static socklen_t addressSize = sizeof(sockaddr_in6);
	std::string retVal;
	sockaddr_in6 sourceAddress;
	int length;
	char recvBuffer[64*1024];
	length = recvfrom(socketFd, recvBuffer, sizeof(recvBuffer)-1, 0, (sockaddr*)&sourceAddress, &addressSize);
	if(sourceAddress.sin6_family == AF_INET6) {
		retVal.assign(recvBuffer, length);
		char ipAddress[64];
		inet_ntop(AF_INET6, &sourceAddress.sin6_addr, ipAddress, sizeof(ipAddress));
		sourceIp = ipAddress;
		sourcePortNumber = ntohs(sourceAddress.sin6_port);
	}
	else if(sourceAddress.sin6_family == AF_INET) {
		retVal.assign(recvBuffer, length);
		char ipAddress[32];
		inet_ntop(AF_INET, &sourceAddress.sin6_addr, ipAddress, sizeof(ipAddress));
		sourceIp = ipAddress;
		sourcePortNumber = ntohs(sourceAddress.sin6_port);
	}
	if(length!=-1){
		retVal.assign(recvBuffer, length);
	}
	return retVal;
}
*/
std::string UdpSocket::read(UdpSocket::Address& hisAddr) {
	static socklen_t addressSize = sizeof(sockaddr_in6);
	std::string retVal;
	int length;
	char recvBuffer[64*1024];
	length = recvfrom(socketFd, recvBuffer, sizeof(recvBuffer)-1, 0, (sockaddr*)&(hisAddr.hisAddress), &addressSize);
	//printf("Family: %lu, FlowInfo: %lu, Scope: %lu", hisAddr.hisAddress.sin6_family, hisAddr.hisAddress.sin6_flowinfo, hisAddr.hisAddress.sin6_scope_id);
	if(length!=-1){
		retVal.assign(recvBuffer, length);
	}
	return retVal;
}


int16_t UdpSocket::write(const std::string& message){
	return sendto(socketFd, message.c_str(), message.size(), 0, (sockaddr*) &hisAddress, sizeof(hisAddress));
}

int16_t UdpSocket::write(const std::string& message, const Address& address) {
	return sendto(socketFd, message.c_str(), message.size(), 0, (sockaddr*) &(address.hisAddress), sizeof(address.hisAddress));
}
/*
int16_t UdpSocket::write(const std::string& message, const std::string& ipAddress, uint16_t portNumber){
	return write(message, to_address(ipAddress, portNumber));
}
*/
