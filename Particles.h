#pragma once

#include "types.h"

struct Particles{
private:
  struct Part {
    Vec2D pos;
    Vec2D vel;

    Part() {}
    Part(const Vec2D& p, const Vec2D& v) : pos{p}, vel{v} {}
  };

  static Array<Part,64> parts;

public:
  static void SpawnAt(const Vec2D& p, uint8_t amount);
  static void Update();
  static void Draw();
  static void Clear();
};