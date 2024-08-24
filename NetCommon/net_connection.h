#pragma once
#include <_types/_uint32_t.h>
#include <_types/_uint64_t.h>
#include <sys/types.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <memory>
#include <system_error>

#include "net_common.h"
#include "net_message.h"
#include "net_server.h"
#include "net_thread_safe_queue.h"

namespace olc {
namespace net {

template <typename T>
class connection : public std::enable_shared_from_this<connection<T>> {
public:
  enum owner { server, client };

  connection(owner parent, asio::io_context &asioContext,
             asio::ip::tcp::socket socket,
             threadSafeQueue<owned_message<T>> &qIn)
      : m_asioContext(asioContext), m_socket(std::move(socket)),
        m_qMessagesIn(qIn) {
    m_nOwnerType = parent;

    // Construct validation check data.
    if (m_nOwnerType == owner::server) {
      // Connection is server -> Client, construct data for the client
      // to transform and send back.
      m_handshakeOut =
          uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

      m_handshakeCheck = scramble(m_handshakeOut);
    } else {
    }
  }
  virtual ~connection() {}

  uint32_t GetID() const { return id; }

public:
  void ConnectToServer(const asio::ip::tcp::resolver::results_type &endpoints) {
    // Only relevant to clients
    if (m_nOwnerType == owner::client) {
      asio::async_connect(
          m_socket, endpoints,
          [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
            if (!ec) {
              readValidation();
            }
          });
    }
  }

  void Disconnect() {
    if (IsConnected())
      asio::post(m_asioContext, [this]() { m_socket.close(); });
  }

  bool IsConnected() const { return m_socket.is_open(); }

  void ConnectToClient(server_interface<T> *server, uint32_t uid = 0) {
    if (m_nOwnerType == owner::server) {
      if (m_socket.is_open()) {
        id = uid;

        // A client attempted to connect to our server, but we wish
        // the client to first validate itself, so first we write out
        // the handshake data to be validated.
        writeValidation();

        // Issue a task to sit and wait async for precisely
        // the validation data sent back from the client.
        readValidation(server);
      }
    }
  }

public:
  bool Send(const message<T> &msg) {
    asio::post(m_asioContext, [this, msg]() {
      bool bWritingMessage = !m_qMessagesOut.empty();
      m_qMessagesOut.push_back(msg);
      if (!bWritingMessage) {
        WriteHeader();
      }
    });
  }

private:
  // ASYNC - Prime context ready to read a message header.
  void ReadHeader() {
    asio::async_read(
        m_socket,
        asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
        [this](std::error_code ec, std::size_t length) {
          if (!ec) {
            if (m_msgTemporaryIn.header.size > 0) {
              m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
              ReadBody();
            } else {
              AddToIncomingMessageQueue();
            }
          } else {
            std::cout << "[" << id << "] Read header Fail.\n";
            m_socket.close();
          }
        });
  }

  // ASYNC - Prime context ready to read a message body.
  void ReadBody() {
    asio::async_read(m_socket,
                     asio::buffer(m_msgTemporaryIn.body.data(),
                                  m_msgTemporaryIn.body.size()),
                     [this](std::error_code ec, std::size_t length) {
                       if (!ec) {
                         AddToIncomingMessageQueue();
                       } else {
                         std::cout << "[" << id << "] Read Body Fail.\n";
                         m_socket.close();
                       }
                     });
  }

  void AddToIncomingMessageQueue() {
    if (m_nOwnerType == owner::server)
      m_qMessagesIn.push_back({this->shared_from_this(), m_msgTemporaryIn});
    else
      m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn});

    ReadHeader();
  }

  void WriteHeader() {
    asio::async_write(
        m_socket,
        asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
        [this](std::error_code ec, std::size_t length) {
          if (!ec) {
            if (m_qMessagesOut.front().body.size() > 0) {
              WriteBody();
            } else {
              m_qMessagesOut.pop_front();

              if (!m_qMessagesOut.empty()) {
                WriteHeader();
              }
            }
          } else {
            std::cout << "[" << id << "] Write header fail.\n";
            m_socket.close();
          }
        });
  }

  void WriteBody() {
    asio::async_write(m_socket,
                      asio::buffer(m_qMessagesOut.front().body.data(),
                                   m_qMessagesOut.front().body.size()),
                      [this](std::error_code ec, std::size_t length) {
                        if (!ec) {
                          m_qMessagesOut.pop_front();
                          if (!m_qMessagesOut.empty()) {
                            WriteHeader();
                          }
                        } else {
                          std::cout << "[" << id << "] Write body fail.\n";
                          m_socket.close();
                        }
                      });
  }

  uint64_t scramble(uint64_t input) {
    uint64_t out = input ^ 0xDEADBEEFC0DECAFE;
    out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0xF0F0F0F0F0F0F0) << 4;
    return out ^ 0xC0DEFACE12345678;
  }

  void writeValidation() {
    asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
                      [this](std::error_code ec, std::size_t length) {
                        if (!ec) {
                          if (m_nOwnerType == client) {
                            ReadHeader();
                          }
                        } else {
                          m_socket.close();
                        }
                      });
  }

  void readValidation(olc::net::server_interface<T> *server = nullptr) {
    asio::async_read(
        m_socket,
        asio::buffer(&m_handshakeIn, sizeof(uint64_t),
                     [this, server](std::error_code ec, std::size_t length) {
                       if (!ec) {
                         if (m_nOwnerType == owner::server) {
                           if (m_handshakeIn == m_handshakeCheck) {
                             std::cout << "Client validated.\n";
                             server->onClientValidated(
                                 this->shared_from_this());

                             // Sit again waiting to receive the header.
                             ReadHeader();
                           } else {
                             // Client failed validation here and we can do
                             // extra here like blacklisting ip address, etc.
                             std::cout << "Client failed validation.\n";
                             m_socket.close();
                           }
                         } else {
                           m_handshakeOut = scramble(m_handshakeIn);
                           writeValidation();
                         }
                       } else {
                         std::cout << "Client disconnected (ReadValidation)\n";
                         m_socket.close();
                       }
                     }));
  }

protected:
  // Each connection has a unique socket to a remote
  asio::ip::tcp::socket m_socket;

  // This context is shared with the whole asio instance
  asio::io_context &m_asioContext;

  // This queue holds all messages to be sent to the remote side of this
  // connection
  threadSafeQueue<message<T>> m_qMessagesOut;

  // This queue holds all messages that have been received from the remote side
  // of this connection. Note it is a reference as the "owner" of this
  // connection is expected to provide a queue.
  threadSafeQueue<owned_message<T>> &m_qMessagesIn;
  message<T> m_msgTemporaryIn;

  // The owner decides how some of the connection behaves.
  owner m_nOwnerType = owner::server;
  uint32_t id = 0;

  // Handshake Validation
  uint64_t m_handshakeOut = 0;
  uint64_t m_handshakeIn = 0;
  uint64_t m_handshakeCheck = 0;
};
} // namespace net
} // namespace olc
