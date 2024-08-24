#include "../MMOServer/MMOCommon.h"
#include "../NetCommon/olc_net.h"
#include "../include/olcPixelGameEngine.h"

#define OLC_PGEX_TRANSFORMEDVIEW
#include <string>

#include "../include/olcPGEX_TransformedView.h"

class MMOGame : public olc::PixelGameEngine,
                olc::net::client_interface<GameMsg> {
 public:
  MMOGame() { sAppName = "MMO Client"; }

  std::string sWorldMap =
      "################################"
      "#..............................#"
      "#.......#####.#.....#####......#"
      "#.......#...#.#.....#..........#"
      "#.......#...#.#.....#..........#"
      "#.......#####.#####.#####......#"
      "#..............................#"
      "#.....#####.#####.#####..##....#"
      "#.........#.#...#.....#.#.#....#"
      "#.....#####.#...#.#####...#....#"
      "#.....#.....#...#.#.......#....#"
      "#.....#####.#####.#####.#####..#"
      "#..............................#"
      "#..............................#"
      "#..#.#..........#....#.........#"
      "#..#.#..........#....#.........#"
      "#..#.#.......#####.#######.....#"
      "#..#.#..........#....#.........#"
      "#..#.#.............###.#.#.....#"
      "#..#.##########................#"
      "#..#..........#....#.#.#.#.....#"
      "#..#.####.###.#................#"
      "#..#.#......#.#................#"
      "#..#.#.####.#.#....###..###....#"
      "#..#.#......#.#....#......#....#"
      "#..#.########.#....#......#....#"
      "#..#..........#....#......#....#"
      "#..############....#......#....#"
      "#..................########....#"
      "#..............................#"
      "#..............................#"
      "################################";

  olc::vi2d worldSize = {32, 32};

 private:
  olc::TileTransformedView tv;

 public:
  bool OnUserCreate() override {
    tv = olc::TileTransformedView({ScreenWidth(), ScreenHeight()}, {8, 8});
    return true;
  }

  bool OnUserUpdate(float elapsedTime) override {
    // Handle Pan & Zoom
    if (GetMouse(2).bPressed) tv.StartPan(GetMousePos());
    if (GetMouse(2).bHeld) tv.UpdatePan(GetMousePos());
    if (GetMouse(2).bReleased) tv.EndPan(GetMousePos());
    if (GetMouseWheel() > 0) tv.ZoomAtScreenPos(2.0f, GetMousePos());
    if (GetMouseWheel() < 0) tv.ZoomAtScreenPos(0.5f, GetMousePos());

    Clear(olc::BLACK);

    // Draw World
    olc::vi2d vTL = tv.GetTopLeftTile().max({0, 0});
    olc::vi2d vBR = tv.GetBottomRightTile().min(worldSize);
    olc::vi2d vTile;
    for (vTile.y = vTL.y; vTile.y < vBR.y; vTile.y++)
      for (vTile.x = vTL.x; vTile.x < vBR.x; vTile.x++) {
        if (sWorldMap[vTile.y * worldSize.x + vTile.x] == '#') {
          tv.DrawRect(vTile, {1.0f, 1.0f}, olc::WHITE);
          tv.DrawLine(vTile, vTile + olc::vf2d(1.0f, 1.0f), olc::WHITE);
          tv.DrawLine(vTile + olc::vf2d(0.0f, 1.0f),
                      vTile + olc::vf2d(1.0f, 0.0f), olc::WHITE);
        }
      }

    return true;
  }
};

int main(int argc, char *argv[]) { MMOGame demo; }
