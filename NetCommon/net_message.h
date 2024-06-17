#pragma once
#include <_types/_uint32_t.h>
#include <_types/_uint8_t.h>
#include <sys/_types/_size_t.h>

#include <cstring>
#include <vector>

#include "net_common.h"

namespace olc {

namespace net {

// Message Header is sent at start of all messages. The templates allows us to
// use "enum class to ensure that the messages are valid at compile time.
template <typename T> struct message_header {
  T id{};
  uint32_t size;
};

template <typename T> struct message {
  message_header<T> header{};
  std::vector<uint8_t> body;

  size_t size() const { return sizeof(message_header<T>) + body.size(); }

  // Override for std::cout compatability - produces friendly description of
  // message.
  friend std::ostream &operator<<(std::ostream &os, const message<T> &msg) {
    os << "ID:" << int(msg.header.id) << "Size:" << msg.header.size;
    return os;
  }

  // Pushes any POD-like data into the message buffer
  template <typename DataType>
  friend message<T> &operator<<(message<T> &msg, const DataType &data) {
    // Check that the type of data being pushed is trivially copyable
    static_assert(std::is_standard_layout<DataType>::value,
                  "Data is too complex to be pushed");

    // Cache current size of vector, as this will be the point we insert the
    // data
    size_t i = msg.body.size();

    // Resize the vector by the size of the data being pushed.
    msg.body.resize(msg.body.size() + sizeof(DataType));

    // Physically copy the data into the newly allocated vector space.
    std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

    // Recalculate the message size
    msg.header.size = msg.size();

    // Return the target message so it can be chained
    return msg;
  }

  template <typename DataType>
  friend message<T> &operator>>(message<T> &msg, DataType &data) {
    // Check that the type of the data being pushed is trivially copyable.
    static_assert(std::is_standard_layout<DataType>::value,
                  "Data is too complex to be pushed");

    // Cache the location towards the end of the vector where the pulled data
    // starts.
    size_t i = msg.body.size() - sizeof(DataType);
  }
};

} // namespace net
} // namespace olc
