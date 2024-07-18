#include <_types/_uint16_t.h>
#include <_types/_uint32_t.h>

#include "../NetCommon/olc_net.h"

enum class CustomMessageTypes : uint32_t {
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage,
};

class CustomServer : public olc::net::server_interface<CustomMessageTypes> {
public:
  CustomServer(uint16_t nPort)
      : olc::net::server_interface<CustomMessageTypes>(nPort){};

protected:
  virtual bool OnClientConnect(
      std::shared_ptr<olc::net::connection<CustomMessageTypes>> client) {
    return true;
  }

  virtual void OnClientDisconnect(
      std::shared_ptr<olc::net::connection<CustomMessageTypes>> client) {}

  virtual void
  OnMessage(std::shared_ptr<olc::net::connection<CustomMessageTypes>> client,
            olc::net::message<CustomMessageTypes> msg) {
    switch (msg.header.id) {
    case CustomMessageTypes::ServerPing:
      std::cout << "[" << client->GetID() << "]: Server Ping\n";
      client->Send(msg);
    }
  }
};

int main(int argc, char *argv[]) {
  CustomServer server(60000);

  server.Start();

  while (1) {
    server.Update();
  }

  return 0;
}
