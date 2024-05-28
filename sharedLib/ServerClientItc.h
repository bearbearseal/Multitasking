#ifndef _SERVERCLIENTITC_H_
#define _SERVERCLIENTITC_H_
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <list>
#include <chrono>
#include <condition_variable>

template <typename Message2Server, typename Message2Client>
class ServerClientItc
{
    friend class Proxy;

public:
    enum class WaitResult
    {
        Terminated,
        Timeout,
        GotMessage
    };
    class ClientId
    {
        friend class ServerClientItc;

    public:
        size_t get_id() { return id; }

    private:
        ClientId(size_t _id) : id(_id) {}
        size_t id;
    };
    struct ServerMessagePair
    {
        ServerMessagePair(const ClientId newId, const Message2Server &inMessage) : sourceId(newId), message(inMessage) {}
        ServerMessagePair(const ClientId newId, Message2Server &&inMessage) : sourceId(newId)
        {
            message = std::move(inMessage);
        }
        ServerMessagePair(const ServerMessagePair &other) : sourceId(other.sourceId.get_id())
        {
            message = other.message;
        }
        ServerMessagePair(ServerMessagePair &&other) : sourceId(other.sourceId.get_id())
        {
            message = std::move(other.message);
        }
        ServerMessagePair& operator=(const ServerMessagePair &other)
        {
            sourceId = other.sourceId;
            message = other.message;
            return *this;
        }
        ServerMessagePair& operator=(ServerMessagePair&& other)
        {
            sourceId = other.sourceId;
            message = move(other.message);
            return *this;
        }
        ClientId sourceId;
        Message2Server message;
    };

private:
    class Proxy
    {
        friend class ServerSocket;
        friend class ClientSocket;

    public:
        Proxy(ServerClientItc &_master) : master(_master)
        {
            master.clientsData.clientData.push_back(ServerClientItc<Message2Server, Message2Client>::ClientData());
        }
        ~Proxy() {}
        void server_to_client(ClientId clientId, Message2Client &&message)
        {
            master.server_to_client(clientId.id, std::move(message));
        }
        void server_to_client(ClientId clientId, const Message2Client &message)
        {
            master.server_to_client(clientId.id, message);
        }
        void client_to_server(ClientId clientId, Message2Server &&message)
        {
            master.client_to_server(clientId.id, std::move(message));
        }
        void client_to_server(ClientId clientId, const Message2Server &message)
        {
            master.client_to_server(clientId.id, message);
        }
        WaitResult wait_server_message()
        {
            return master.wait_server_message();
        }
        WaitResult wait_server_message(std::chrono::duration<uint64_t, std::micro> timeout)
        {
            return master.wait_server_message(timeout);
        }
        WaitResult wait_client_message(ClientId clientId)
        {
            return master.wait_client_message(clientId.id);
        }
        WaitResult wait_client_message(ClientId clientId, std::chrono::duration<uint64_t, std::micro> timeout)
        {
            return master.wait_client_message(clientId.id, timeout);
        }
        std::optional<ServerMessagePair> get_server_message()
        {
            return master.get_server_message();
        }
        std::optional<Message2Client> get_client_message(ClientId clientId)
        {
            return master.get_client_message(clientId.id);
        }
        void return_client_to_pool(ClientId clientId)
        {
            return master.return_client_socket_to_pool(clientId.id);
        }
        bool has_server_message()
        {
            return master.server_has_message();
        }
        bool has_client_message(ClientId clientId)
        {
            return master.client_has_message(clientId.id);
        }

    private:
        ServerClientItc &master;
    };

public:
    class ServerSocket
    {
        friend class ServerClientItc;

    private:
        ServerSocket(std::weak_ptr<Proxy> _master) : master(_master) {}

    public:
        ~ServerSocket() {}
        bool send_message(ClientId clientId, Message2Client &&message)
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return false;
            shared->server_to_client(clientId, std::move(message));
            return true;
        }
        bool send_message(ClientId clientId, const Message2Client &message)
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return false;
            shared->server_to_client(clientId, message);
            return true;
        }
        WaitResult wait_message()
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return WaitResult::Terminated;
            return shared->wait_server_message();
        }
        WaitResult wait_message(std::chrono::duration<uint64_t, std::micro> timeout)
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return WaitResult::Terminated;
            return shared->wait_server_message(timeout);
        }
        std::optional<ServerMessagePair> get_message()
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return {};
            return shared->get_server_message();
        }
        bool has_message()
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return false;
            return shared->has_server_message();
        }

    private:
        std::weak_ptr<Proxy> master;
    };
    class ClientSocket
    {
        friend class ServerClientItc;

    private:
        ClientSocket(std::weak_ptr<Proxy> _master, ClientId _myId) : master(_master), myId(_myId) {}

    public:
        ~ClientSocket()
        {
            // return to pool
            auto shared = master.lock();
            if (shared == nullptr)
                return;
            shared->return_client_to_pool(myId.id);
        }
        bool send_message(Message2Server &&message)
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return false;
            shared->client_to_server(myId.id, std::move(message));
            return true;
        }
        bool send_message(const Message2Server &message)
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return false;
            shared->client_to_server(myId.id, message);
            return true;
        }
        WaitResult wait_message()
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return WaitResult::Terminated;
            return shared->wait_client_message(myId.id);
        }
        WaitResult wait_message(std::chrono::duration<uint64_t, std::micro> timeout)
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return WaitResult::Terminated;
            return shared->wait_client_message(myId.id, timeout);
        }
        std::optional<Message2Client> get_message()
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return {};
            return shared->get_client_message(myId.id);
        }
        bool has_message()
        {
            auto shared = master.lock();
            if (shared == nullptr)
                return false;
            return shared->has_client_message(myId.id);
        }

    private:
        std::weak_ptr<Proxy> master;
        ClientId myId;
    };

public:
    ServerClientItc() { myProxy = std::make_shared<Proxy>(*this); }
    ~ServerClientItc() {}
    std::unique_ptr<ServerSocket> get_server_socket()
    {
        std::unique_ptr<ServerSocket> retVal(new ServerSocket(myProxy));
        return retVal;
    }
    std::unique_ptr<ClientSocket> get_client_socket()
    {
        size_t theId = 0;
        {
            std::lock_guard<std::mutex> lock(clientsData.idMutex);
            if (!clientsData.availableId.empty())
            {
                theId = clientsData.availableId[clientsData.availableId.size() - 1];
                clientsData.availableId.pop_back();
            }
            else
            {
                theId = clientsData.nextId;
                ++clientsData.nextId;
                ClientData dummy;
                clientsData.clientData.push_back(dummy);
            }
        }
        std::unique_ptr<ClientSocket> retVal(new ClientSocket(myProxy, theId));
        return retVal;
    }

private:
    void server_to_client(size_t clientId, Message2Client &&message)
    {
        ClientData &theData = clientsData.clientData[clientId];
        std::lock_guard<std::mutex> lock(theData.messageMutex);
        theData.messages.push_back(std::move(message));
        theData.waitCondition.notify_one();
    }
    void server_to_client(size_t clientId, const Message2Client &message)
    {
        ClientData &theData = clientsData.clientData[clientId];
        std::lock_guard<std::mutex> lock(theData.messageMutex);
        theData.messages.push_back(message);
        theData.waitCondition.notify_one();
    }
    void client_to_server(size_t clientId, Message2Server &&message)
    {
        std::lock_guard<std::mutex> lock(serverData.messageMutex);
        serverData.messages.push_back({clientId, std::move(message)});
        serverData.waitCondition.notify_one();
    }
    void client_to_server(size_t clientId, const Message2Server &message)
    {
        std::lock_guard<std::mutex> lock(serverData.messageMutex);
        serverData.messages.push_back({clientId, message});
        serverData.waitCondition.notify_one();
    }
    void return_client_socket_to_pool(size_t clientId)
    {
        {
            ClientData &theData = clientsData.clientData[clientId];
            std::lock_guard<std::mutex> lock(theData.messageMutex);
            theData.messages.clear();
        }
        {
            std::lock_guard<std::mutex> lock(clientsData.idMutex);
            clientsData.availableId.push_back(clientId);
        }
    }
    WaitResult wait_server_message(std::chrono::duration<uint64_t, std::micro> timeout)
    {
        // Lock the wait mutex, so that it could be signaled
        std::unique_lock<std::mutex> waitLock(serverData.waitMutex);
        // Check if already got message.
        {
            std::lock_guard<std::mutex> messageLock(serverData.messageMutex);
            if (!serverData.messages.empty())
            {
                return WaitResult::GotMessage;
            }
        }
        auto result = serverData.waitCondition.wait_for(waitLock, timeout);
        // Check Result
        if (result == std::cv_status::timeout)
        {
            return WaitResult::Timeout;
        }
        return WaitResult::GotMessage;
    }
    WaitResult wait_server_message()
    {
        // Lock the wait mutex, so that it could be signaled
        std::unique_lock<std::mutex> waitLock(serverData.waitMutex);
        // Check if already got message.
        {
            std::lock_guard<std::mutex> messageLock(serverData.messageMutex);
            if (!serverData.messages.empty())
            {
                return WaitResult::GotMessage;
            }
        }
        serverData.waitCondition.wait(waitLock);
        return WaitResult::GotMessage;
    }
    WaitResult wait_client_message(size_t clientId, std::chrono::duration<uint64_t, std::micro> timeout)
    {
        ClientData &theData = clientsData.clientData[clientId];
        std::unique_lock<std::mutex> waitLock(theData.waitMutex);
        // Check if already got message
        {
            std::lock_guard<std::mutex> mesageLock(theData.messageMutex);
            if (!theData.messages.empty())
            {
                return WaitResult::GotMessage;
            }
        }
        auto result = theData.waitCondition.wait_for(waitLock, timeout);
        // Check Result
        if (result == std::cv_status::timeout)
        {
            return WaitResult::Timeout;
        }
        return WaitResult::GotMessage;
    }
    WaitResult wait_client_message(size_t clientId)
    {
        ClientData &theData = clientsData.clientData[clientId];
        std::unique_lock<std::mutex> waitLock(theData.waitMutex);
        // Check if already got message.
        {
            std::lock_guard<std::mutex> messageLock(theData.messageMutex);
            if (!theData.messages.empty())
            {
                return WaitResult::GotMessage;
            }
        }
        theData.waitCondition.wait(waitLock);
        return WaitResult::GotMessage;
    }
    std::optional<ServerMessagePair> get_server_message()
    {
        std::lock_guard<std::mutex> lock(serverData.messageMutex);
        if (serverData.messages.empty())
        {
            return {};
        }
        ServerMessagePair retVal = std::move(serverData.messages.front());
        serverData.messages.pop_front();
        return retVal;
    }
    std::optional<Message2Client> get_client_message(size_t clientId)
    {
        ClientData &theData = clientsData.clientData[clientId];
        std::lock_guard<std::mutex> lock(theData.messageMutex);
        if (theData.messages.empty())
        {
            return {};
        }
        Message2Client retVal = std::move(theData.messages.front());
        theData.messages.pop_front();
        return retVal;
    }
    bool server_has_message()
    {
        std::lock_guard<std::mutex> lock(serverData.messageMutex);
        if (serverData.messages.empty())
        {
            return false;
        }
        return true;
    }
    bool client_has_message(size_t clientId)
    {
        ClientData &theData = clientsData.clientData[clientId];
        std::lock_guard<std::mutex> lock(theData.messageMutex);
        if (theData.messages.empty())
        {
            return false;
        }
        return true;
    }

private:
    struct ClientData
    {
        ClientData() {}
        ClientData(const ClientData &theOther) {}
        std::mutex messageMutex;
        std::mutex waitMutex;
        std::list<Message2Client> messages;
        std::condition_variable waitCondition;
    };
    struct
    {
        std::mutex messageMutex;
        std::mutex waitMutex;
        std::list<ServerMessagePair> messages;
        std::condition_variable waitCondition;
    } serverData;
    struct ClientsData
    {
        std::mutex idMutex;
        size_t nextId = 0;
        std::vector<size_t> availableId;
        std::vector<ClientData> clientData;
        // std::list<ClientData> clientData;
    } clientsData;
    std::shared_ptr<Proxy> myProxy;
};

#endif
