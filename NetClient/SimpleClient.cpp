#include <X11/Xlib.h>
#include <_types/_uint32_t.h>
#include <curses.h>
#include <ncurses.h>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <system_error>

#include "../NetCommon/olc_net.h"
#include "X11/X.h"

enum class CustomMsgTypes : uint32_t {
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage,
};

class CustomClient : public olc::net::client_interface<CustomMsgTypes> {
public:
  void PingServer() {
    olc::net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::ServerPing;

    // Sketchy way to time the time it takes for the message to round robin.
    std::chrono::system_clock::time_point timeNow =
        std::chrono::system_clock::now();

    msg << timeNow;
  }
};

bool kbhit() {
  int ch = getch();

  if (ch != ERR) {
    ungetch(ch);
    return false;
  }

  else
    return true;
}

int main(int argc, char *argv[]) {
  CustomClient c;
  c.Connect("127.0.0.1", 60000);

  bool bQuit = false;
  while (!bQuit) {
    int revert;
    Window winFocus;
    Display *display = XOpenDisplay(NULL);
    if (XDefaultRootWindow(display) ==
        XGetInputFocus(display, &winFocus, &revert)) {
      if (kbhit()) {
        if (getch() == 0)
          c.PingServer();

        if (getch() == 2)
          bQuit = true;
      }
    };

    if (c.isConnected()) {
      if (!c.Incoming().empty()) {
        auto msg = c.Incoming().pop_front().msg;

        switch (msg.header.id) {
        case CustomMsgTypes::ServerPing: {
          std::chrono::system_clock::time_point timeNow =
              std::chrono::system_clock::now();
          std::chrono::system_clock::time_point timeThen;
          msg >> timeThen;
          std::cout << "Ping: "
                    << std::chrono::duration<double>(timeNow - timeThen).count()
                    << "\n";
        }
        }
      }
    } else {
      std::cout << "Server down.\n";
      bQuit = true;
    }
  }
  return 0;
}
