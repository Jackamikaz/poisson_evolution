#include "Particles.h"
#include "helpers.h"
#include "Globals.h"

using namespace Globals;

Array<Particles::Part,64> Particles::parts;

void Particles::SpawnAt(const Vec2D& p, uint8_t amount) {
  for(unsigned char i=0; i<amount; ++i) {
    Vec2D dir(random(-32,32),random(-32,32));
    dir *= FP(0.05);
    parts.push_back({p,dir});
  }
}

void Particles::Update() {
  unsigned char i=0;
  while(i<parts.size()) {
    Part& part = parts[i];
    part.vel(1) += 0.1;
    part.pos += part.vel;
    if (!pointInScreen(part.pos)) {
      parts.remove(i);
    }
    else {
      ++i;
    }
  }
}

void Particles::Draw() {
  for(auto&& part : parts) {
    ard.drawPixel((int16_t)part.pos(0),(int16_t)part.pos(1));
  }
}

void Particles::Clear() {
  parts.clear();
}