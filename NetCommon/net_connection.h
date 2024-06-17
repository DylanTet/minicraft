#pragma once
#include <memory>

#include "net_common.h"
#include "net_message.h"
#include "net_thread_safe_queue.h"

namespace olc {
namespace net {

template <typename T>
class connection : public std::enable_shared_from_this<connection<T>> {
public:
  connection() {}
  virtual ~connection() {}

public:
  bool ConnectToServer();
  bool Disconnect();
  bool IsConnected() const;

public:
  bool Send(const message<T> &msg);

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
};
} // namespace net
} // namespace olc
