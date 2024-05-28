#include "TcpListener.h"

using namespace std;

TcpListener::TcpListener(uint16_t _portNumber)
{
	portNumber = _portNumber;
	theProcess = nullptr;
	myShadow = make_shared<Shadow>(*this);
}

TcpListener::~TcpListener()
{
	stop();
}

void TcpListener::start()
{
	// terminate.store(false);
	lock_guard<mutex> lock(connectionMutex);
	if (theProcess == nullptr)
	{
		terminate = false;
		theProcess = make_unique<std::thread>(thread_process, this);
	}
}

void TcpListener::stop()
{
	terminate = true;
	while (socketAddress2Connection.size())
	{
		socketAddress2Connection.begin()->second->stop();
	}
	theProcess->join();
}

void TcpListener::catch_message(const std::string &data, size_t handle)
{
	string reply = "Got your message: ";
	reply += data;
	write_message(handle, reply);
}

void TcpListener::catch_connect_event(size_t handle)
{
	printf("Caught connect event from Socket %zu\n", handle);
}

void TcpListener::catch_disconnect_event(size_t handle)
{
	printf("Caught disconnect event from socket %zu\n", handle);
}

bool TcpListener::write_message(size_t handle, const std::string &message)
{
	lock_guard<mutex> lock(connectionMutex);
	auto i = socketAddress2Connection.find(handle);
	if (i != socketAddress2Connection.end())
	{
		return (i->second)->write(message);
	}
	return false;
}

void TcpListener::broadcast(const std::string &data)
{
	lock_guard<mutex> lock(connectionMutex);
	for (auto i = socketAddress2Connection.begin(); i != socketAddress2Connection.end(); ++i)
	{
		i->second->write(data);
	}
}

void TcpListener::close_socket(size_t handle)
{
	/*
	connectionMutex.lock();
	auto i = socketAddress2Connection.find(handle);
	if(i == socketAddress2Connection.end()) {
		connectionMutex.unlock();
		return;
	}
	i->second->stop();
		socketAddress2Connection.erase(i);
	}
	*/
}

// Only mark to delete, do it at main thread
void TcpListener::free_connection(size_t handle)
{
	lock_guard<mutex> lock(markedMutex);
	markedRemove.emplace(handle);
}

void TcpListener::thread_process(TcpListener *me)
{
	printf("Listen socket listening.\n");
	me->listenSocket.listen(me->portNumber);
	while (1)
	{
		auto tcpSocket = make_unique<TcpSocket>();
		printf("Waiting for connection, socket: %p.\n", tcpSocket.get());
		me->listenSocket.accept(*(tcpSocket.get()));
		{
			lock_guard<mutex> lock(me->connectionMutex);
			if (me->terminate)
			{
				tcpSocket->close();
				// delete tcpSocket;
				return;
			}
			else
			{
				size_t key = (size_t)tcpSocket.get();
				auto singleConnection = make_unique<SingleConnection>(me->myShadow, move(tcpSocket));
				me->socketAddress2Connection.insert({key, move(singleConnection)});
				me->socketAddress2Connection[key]->start();
				me->catch_connect_event(key);
			}
			{
				lock_guard<mutex> otherLock(me->markedMutex);
				for (size_t key : me->markedRemove)
				{
					me->socketAddress2Connection[key]->stop();
					me->socketAddress2Connection.erase(key);
				}
				me->markedRemove.clear();
			}
		}
	}
}

TcpListener::SingleConnection::SingleConnection(shared_ptr<Shadow> _master, unique_ptr<TcpSocket> _socket)
{
	master = _master;
	socket = move(_socket);
}

TcpListener::SingleConnection::~SingleConnection()
{
	// printf("Single connection destroyed and my socket address is %p\n", this->socket.get());
}

void TcpListener::SingleConnection::start()
{
	theThread = make_unique<thread>(thread_process, this);
}

void TcpListener::SingleConnection::stop()
{
	theThread->join();
}

bool TcpListener::SingleConnection::write(const std::string &message)
{
	return socket->write(message);
}

void TcpListener::SingleConnection::thread_process(SingleConnection *me)
{
	while (1)
	{
		auto input = me->socket->read();
		if (input.first)
		{
			auto shared = me->master.lock();
			if (shared != nullptr)
				shared->catch_message(input.second, (size_t)me->socket.get());
			else
				return;
		}
		else
		{
			auto shared = me->master.lock();
			if (shared != nullptr)
			{
				size_t socketHandle = (size_t)me->socket.get();
				shared->catch_disconnect_event(socketHandle);
				me->socket->close();
				shared->free_connection(socketHandle);
			}
			return;
		}
	}
}
