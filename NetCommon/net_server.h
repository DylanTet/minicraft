#pragma once

#include <__algorithm/remove.h>
#include <_types/_uint16_t.h>
#include <_types/_uint32_t.h>
#include <sys/_types/_size_t.h>

#include <algorithm>
#include <deque>
#include <exception>
#include <memory>
#include <system_error>

#include "net_common.h"
#include "net_connection.h"
#include "net_message.h"
#include "net_thread_safe_queue.h"

namespace olc {
namespace net {
template <typename T>
class server_interface {
 public:
  server_interface(uint16_t port)
      : m_asioAcceptor(m_asioContext,
                       asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}

  virtual ~server_interface() { Stop(); }

  bool Start() {
    try {
      WaitForClientConnection();
      m_threadContext = std::thread([this]() { m_asioContext.run(); });

    } catch (std::exception &e) {
      std::cerr << "[SERVER] Exception: " << e.what() << "\n";
      return false;
    }

    std::cout << "[SERVER] Started\n";
    return true;
  }

  void Stop() {
    m_asioContext.stop();
    if (m_threadContext.joinable()) m_threadContext.join();

    std::cout << "[SERVER] Stopped\n";
  }

  // ASYNC - Instruct Asio to wait for connection
  void WaitForClientConnection() {
    m_asioAcceptor.async_accept([this](std::error_code ec,
                                       asio::ip::tcp::socket socket) {
      if (!ec) {
        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint()
                  << "\n";
        std::shared_ptr<connection<T>> newConnection =
            std::make_shared<connection<T>>(connection<T>::owner::server,
                                            m_asioContext, std::move(socket),
                                            m_qMessagesIn);

        // Give the user server a chance to deny connection
        if (OnClientConnect(newConnection)) {
          // Connection allowed, so add to container of new connections.
          m_deqConnections.push_back(std::move(newConnection));
          m_deqConnections.back()->ConnectToClient(this, nIDCounter++);
          std::cout << "[" << m_deqConnections.back()->GetID()
                    << "] Connection Approved\n";
        } else {
          std::cout << "[-----] Connection Denied\n";
        }
      } else {
        std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
      }

      // Prime the asio context with more work - again simply await for another
      // connection.
      WaitForClientConnection();
    });
  }

  // Send a message to a specific client
  void MessageClient(std::shared_ptr<connection<T>> client,
                     const message<T> &msg) {
    if (client && client->IsConnected()) {
      client->Send();
    } else {
      OnClientDisconnect(client);
      client.reset();
      m_deqConnections.erase(std::remove(m_deqConnections.begin(),
                                         m_deqConnections.end(), client));
    }
  }

  void MessageAllClients(
      const message<T> &msg,
      std::shared_ptr<connection<T>> pIgnoreClient = nullptr) {
    bool bInvalidClientsExist = false;

    for (auto &client : m_deqConnections) {
      if (client && client->IsConnected()) {
        if (client != pIgnoreClient) {
          client->Send();
        }
      } else {
        OnClientDisconnect(client);
        client.reset();
        bInvalidClientsExist = true;
      }
    }
    if (bInvalidClientsExist) {
      m_deqConnections.erase(std::remove(m_deqConnections.begin(),
                                         m_deqConnections.end(), nullptr),
                             m_deqConnections.end());
    }
  }

  void Update(size_t nMaxMessages = -1, bool wait = false) {
    if (wait) m_qMessagesIn.wait();

    size_t nMessageCount = 0;
    while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {
      auto msg = m_qMessagesIn.pop_front();
      OnMessage(msg.remote, msg.msg);
      nMessageCount++;
    }
  }

 protected:
  // Called when a client connects to our server, you can veto the connection
  // here by returning false
  virtual bool OnClientConnect(std::shared_ptr<connection<T>> client) {
    return false;
  }

  // Called when a client appears to have disconnected
  virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client) {}

  virtual void OnMessage(std::shared_ptr<connection<T>> client,
                         message<T> &msg) {}

 public:
  virtual void onClientValidated(std::shared_ptr<connection<T>> client) {}

 protected:
  // Thread safe queue for incoming message packets
  threadSafeQueue<owned_message<T>> m_qMessagesIn;

  // Container of active validated connections
  std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

  // ORder of declaration is important!!! Its also the order of initialization.
  asio::io_context m_asioContext;
  std::thread m_threadContext;

  // These things need an asio context
  asio::ip::tcp::acceptor m_asioAcceptor;

  // Clients will be identified in the "wider system" via an ID
  uint32_t nIDCounter = 10000;
};
}  // namespace net
}  // namespace olc
