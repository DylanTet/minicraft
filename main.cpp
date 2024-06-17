#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <iostream>
#include <system_error>
#include <thread>

std::vector<char> vBuffer(1024 * 20);

void grabSomeData(asio::ip::tcp::socket &socket) {
  socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
                         [&](std::error_code ec, std::size_t length) {
                           if (!ec) {
                             std::cout << "\n\nRead " << length << " bytes\n\n";

                             for (int i = 0; i < length; i++) {
                               std::cout << vBuffer[i];
                             }

                             grabSomeData(socket);
                           }
                         });
}

int main(int argc, char *argv[]) {
  asio::error_code ec;

  // Create a "context" - essentially the platform specific context.
  asio::io_context context;

  // Give some fake tasks to asio so the context doesnt finish
  asio::io_context::work idleWork(context);

  // Start the context
  std::thread thrContext = std::thread([&]() { context.run(); });

  // Get the address of somewhere we wish to connect to
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec),
                                   80);

  // Create a socket, the context will deliver the implementation.
  asio::ip::tcp::socket socket(context);
  socket.connect(endpoint, ec);

  if (!ec) {
    std::cout << "Connected" << std::endl;
  } else {
    std::cout << "Failed " << ec.message() << std::endl;
  }

  if (socket.is_open()) {
    grabSomeData(socket);

    std::string sRequest = "GET /index.html HTTP/1.1\r\n"
                           "Host: example.com\r\n"
                           "Connection: close\r\n\r\n";

    socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2000ms);
  }

  return 0;
}
