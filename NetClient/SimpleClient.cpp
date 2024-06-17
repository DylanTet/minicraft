#include <_types/_uint32_t.h>

#include <iostream>

#include "../NetCommon/olc_net.h"

enum class CustomMsgTypes : uint32_t {
  FireBullet,
  MovePlayer,
};

int main(int argc, char *argv[]) {
  olc::net::message<CustomMsgTypes> msg;
  msg.header.id = CustomMsgTypes::FireBullet;

  int a = 1;
  bool b = true;
  float c = 3.1459f;

  struct {
    float x;
    float y;
  } d[5];

  msg << a << b << c << d;

  a = 99;
  b = false;
  c = 99.0f;

  msg >> d >> c >> b >> a;

  return 0;
}
