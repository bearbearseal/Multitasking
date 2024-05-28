#ifndef _TCPLISTENER_H_
#define _TCPLISTENER_H_
#include <stdint.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <mutex>
#include "TcpSocket.h"

class TcpListener {
    friend class Shadow;
private:
	class Shadow {
	public:
		Shadow(TcpListener& _master) : master(_master) {}
		virtual ~Shadow() {}

		void catch_message(const std::string& data, size_t handle) { master.catch_message(data, handle); }
		void catch_connect_event(size_t handle) { master.catch_connect_event(handle); }
		void catch_disconnect_event(size_t handle) { master.catch_disconnect_event(handle); }
		void write_message(size_t handle, const std::string& message) { master.write_message(handle, message); }
        void close_socket(size_t handle) { master.close_socket(handle);}
		void free_connection(size_t handle) { master.free_connection(handle); }

	private:
		TcpListener& master;
	};
public:
	TcpListener(uint16_t portNumber);
	virtual ~TcpListener();

	void start();
	void stop();

protected:
	virtual void catch_message(const std::string& data, size_t handle);
	virtual void catch_connect_event(size_t handle);
	virtual void catch_disconnect_event(size_t handle);
	void broadcast(const std::string& data);
	bool write_message(size_t handle, const std::string& message);
    void close_socket(size_t handle);

private:
	void free_connection(size_t handle);

	uint16_t portNumber;
	TcpSocket listenSocket;
	std::unique_ptr<std::thread> theProcess;
	//std::atomic<bool> terminate;
	bool terminate;
	std::shared_ptr<Shadow> myShadow;

	class SingleConnection {
	public:
		SingleConnection(std::shared_ptr<Shadow> _master, std::unique_ptr<TcpSocket> socket);
        SingleConnection(SingleConnection&& theOther){ socket = std::move(theOther.socket); theThread = std::move(theOther.theThread); master = theOther.master; theOther.socket.reset();}
		virtual ~SingleConnection();
		void start();
		void stop();
		bool write(const std::string& message);

	private:
		static void thread_process(SingleConnection* me);
		std::unique_ptr<TcpSocket> socket;
		std::unique_ptr<std::thread> theThread;
		std::weak_ptr<Shadow> master;
	};

	std::mutex connectionMutex;
	std::unordered_map<size_t, std::unique_ptr<SingleConnection>> socketAddress2Connection;
    std::mutex markedMutex;
    std::unordered_set<size_t> markedRemove;
	static void thread_process(TcpListener* me);
};
#endif