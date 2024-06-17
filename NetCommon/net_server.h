#pragma once

#include <_types/_uint16_t.h>

#include "net_common.h"
#include "net_connection.h"
#include "net_message.h"
#include "net_thread_safe_queue.h"

namespace olc {
namespace net {
template <typename T> class server_interface {
public:
  server_interface(uint16_t port) {}

  virtual ~server_interface() {}

  bool Start() {}

  void Stop() {}

  // ASYNC - Instruct Asio to wait for connection
  void WaitForClientConnection() {}
};
} // namespace net
} // namespace olc
