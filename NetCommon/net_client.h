#pragma once
#include <_types/_uint16_t.h>

#include <any>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "net_common.h"
#include "net_connection.h"
#include "net_message.h"
#include "net_thread_safe_queue.h"

namespace olc {
namespace net {
template <typename T> class client_interface {
  client_interface() : m_socket(m_context) {}

  virtual ~client_interface() { Disconnect(); }

public:
  bool Connect(const std::string &host, const uint16_t port) {
    try {
      // Make connection
      m_connection = std::make_unique<connection<T>>(); // TODO

      // REsolve hostname/ip-address into tangiable physical ip-address
      asio::ip::tcp::resolver resolver(m_context);
      m_endpoint = resolver.resolve(host, std::to_string(port));

      // Tell the connection object to connect to server
      m_connection->ConnectToServer(m_endpoint);

      // Start context thread
      thrContext = std::thread([this]() { m_context.run(); });

    } catch (std::exception &e) {
      std::cerr << "Client Exception: " << e.what() << "\n";
      return false;
    }
    return true;
  }

  // Disconnect from the server
  void Disconnect() {
    if (isConnected())
      m_connection->Disconnect();

    m_context.stop();

    if (thrContext.joinable())
      thrContext.join();

    m_connection.release();
  }

  bool isConnected() { return false; }

  threadSafeQueue<owned_message<T>> &Incoming() { return m_qMessagesIn; }

protected:
  asio::ip::tcp::endpoint m_endpoint;
  // Context handles the data transfer...
  asio::io_context m_context;
  // ... but needs a thread of its own to execute its work commands.
  std::thread thrContext;
  // This is the hardware socket that is connected to the server.
  asio::ip::tcp::socket m_socket;
  // The client has a single instance of a "connection" object, which handles
  // data transfer.
  std::unique_ptr<connection<T>> m_connection;

private:
  // This is the thread safe queue of incoming messages from server.
  threadSafeQueue<owned_message<T>> m_qMessagesIn;
};
} // namespace net
} // namespace olc
