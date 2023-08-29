#include "Globals.h"
#include "sprites.h"
#include "Font.h"
#include "helpers.h"
#include "Particles.h"
#include "Constants.h"

using namespace Globals;

uint8_t titre = 1;
uint8_t restarting = 0;
uint8_t spawntimer = 1;
int score=0;
Array<Vec2D,10> tirs;

struct Ennemi{
  Vec2D pos;
  Vec2D size;
  //FP yvel{0};
  FP xvel{-0.75};
  uint8_t ytarget{0};
  int8_t life{5};
  uint8_t clignote{1};

  Ennemi() {}
  Ennemi(int16_t x, int16_t y) :
    pos{x,random(-20,HEIGHT-pgm_read_byte(bonhomme+1)+20)},
    size{pgm_read_byte(bonhomme),pgm_read_byte(bonhomme+1)},
    ytarget(random(10,HEIGHT-pgm_read_byte(bonhomme+1)-10))
  {}

  // return true if it needs to be deleted
  bool UpdateAndDraw() {
    // side scrolling
    xvel = crawl(xvel,FP(-0.75),FP(0.1));
    pos(0) += xvel;
    if (pos(0)+size(0) < 0) return true;

    // vertical hopping
    // yvel += 0.1;
    // pos(1) += yvel;
    // if (pos(1)+size(1) > 64 && yvel > 0) yvel *= -1;
    pos(1) = lerp(pos(1),FP(ytarget),FP(0.03));

    // blink and draw
    if (clignote>1) --clignote;
    draw(pos, bonhomme, clignote%2);
    return false;
  }
  void Die() {
    Particles::SpawnAt(pos+size*FP(0.5),32);
    beep.tone(beep.freq(500),10);
  }
};

Array<Ennemi,3> ennemis;

struct Player {
  Vec2D offsets[3] = {{4,2},{16,5},{23,7}};
  const uint8_t* moyenframes[4] = {moyenpoisson1,moyenpoisson2,moyenpoisson3,moyenpoisson4}; 

  Vec2D pos;
  Vec2D vel;
  uint8_t evo;
  uint8_t life;
  uint8_t hit;
  uint8_t tailflip;

  void Reset() {
    pos = {10,30};
    vel = {0,0};
    evo = 0;
    life = 6;
    hit = 0;
    tailflip = 0;
  }

  void Update() {
    if (!life) {
      Particles::SpawnAt(Vec2D(random(0,128),random(0,64)), 1);
      if (hit) {
        vel(1) += 0.2;
        pos += vel;
        if (pos(1) > 0 && vel(1) > 0) {
          vel(1) *= FP(-0.8);
          if (vel(1) > -1.0) hit=0;
          beep.tone(beep.freq(100),3);
      }
      }
      return;
    }
    // move
    Vec2D target(
      (FP)ard.pressed(RIGHT_BUTTON) - (FP)ard.pressed(LEFT_BUTTON),
      (FP)ard.pressed(DOWN_BUTTON) - (FP)ard.pressed(UP_BUTTON));

    if (target(0)!=0 && abs(target(0))==abs(target(1))) {
      target *= Constants::invsqrt2;
    }
    target *= FP(2);

    for(uint8_t i=0; i<2; ++i) {
      vel(i) = crawl(vel(i),target(i),FP(0.2));
    }

    pos += vel;

    // limit to screen borders
    Vec2D ul=pos-offsets[evo];
    unsigned char* const curspr=evo==0 ? poisson : grospoisson;
    Vec2D br=ul+Vec2D(pgm_read_byte(curspr),pgm_read_byte(curspr+1));
    if (ul(0) < 0) pos(0)-=ul(0);
    if (ul(1) < 0) pos(1)-=ul(1);
    if (br(0) > 127) pos(0)+=127-br(0);
    if (br(1) > 63) pos(1)+=63-br(1);

    // shoot
    if (!hit && ard.justPressed(A_BUTTON)) {
      tirs.push_back(pos);
      beep2.tone(beep2.freq(5000),2);
    }

    // tail flip hit
    if (!tailflip && evo==1 && ard.justPressed(B_BUTTON)) {
      tailflip = 1;
    }
    else if (tailflip) {
      if (++tailflip >= 20) tailflip=0;
    }
    bool checktailflip = tailflip >= 12;
    // ennemies collision
    bool checkeat = evo==2 && ard.justReleased(B_BUTTON);
    for(uint8_t i=0; i<ennemis.size();++i) {
      Ennemi& en=ennemis[i];
      // eat when evolved
      if ((checkeat || checktailflip) && pointInRect(pos,en.pos,en.size)) {
        if (checkeat) {
          score += 5;
          en.life -= 10;
        }
        else if (checktailflip && en.xvel< 0) {
          score += 1;
          en.life -= 2;
          en.xvel = 2;
        }

        if (en.life<=0) {
          en.Die();
          ennemis.remove(i);
        }
        else {
          beep.tone(beep.freq(750),2);
          Particles::SpawnAt(pos, 5);
        }
        break;
      }
      // hit
      else if (hit==0 && pointInRect(pos-Vec2D(10,0),en.pos,en.size)) {
        vel(0) = -4;
        --life;
        if (!life) {
          pos = {0,-100};
          vel = {0,0};
        }
        hit = 61;
        Particles::SpawnAt(Vec2D(2,2+(life/2)*6), 10);
        beep2.tone(beep2.freq(100),hit);
        break;
      }
    }
    if (hit>0) --hit;

    // check evo
    if (evo == 0 && score >= 10) {
      evo = 1;
      life += 4;
    }
    if (evo==1 && score >=50) {
      tailflip=0;
      evo = 2;
      life += 8;
    }
  }

  void Draw() {
    if(!life) {
      ard.drawCompressed((int16_t)pos(0), (int16_t)pos(1), gameover);
      return;
    }
    if((hit>>2)%2==0) {
      const uint8_t* spr = poisson;
      if (evo==1) spr = moyenframes[min(tailflip/4,3)];
      if (evo==2) spr = ard.pressed(B_BUTTON) ? grospoissonmange : grospoisson;
      draw(pos-offsets[evo], spr);
    }
  }

  void DrawHealth() {
    if (life>20) return;
    Vec2D disp;
    for(uint8_t i=0; i<life; i+=2) {
      draw(disp,life-i==1 ? coeurdemi : coeurplein);
      disp(1) += 6;
    }
  }
};

Player player;

void setup() {
  ard.boot();
  ard.audio.on();
  ard.setFrameRate(60);
  ard.initRandomSeed();
  beep.begin();
  beep2.begin();
}

void loop() {
  if (!ard.nextFrame()) return;
  beep.timer();
  beep2.timer();
  ard.pollButtons();

  if (titre) {
    if (ard.justPressed(LEFT_BUTTON) || ard.justPressed(RIGHT_BUTTON)) {
      titre = titre%2+1;
      beep2.tone(beep2.freq(5000),2);
    }
    if (ard.justPressed(A_BUTTON) || ard.justPressed(B_BUTTON)) {
      if (titre==1) ard.audio.toggle();
      if (titre==2) {
        titre=0;
        restarting=0;
        score=0;
        tirs.clear();
        ennemis.clear();
        player.Reset();
        Particles::Clear();
      }
    }

    ard.clear();
    ard.drawCompressed(0,0,ecrantitre);
    Font::PrintString(PSTR(">"), titre==1 ? 24 : 71, 7);
    Font::PrintString(ard.audio.enabled() ? PSTR("Sound on") : PSTR("Sound off"), 28, 7);
    Font::PrintString(PSTR("Start"),75,7);
    ard.display();
    return;
  }

  player.Update();

  if (player.life<=0) {
    if (ard.anyPressed(0xFF)) {
      if (++restarting==100) titre=1; }
    else
      restarting = 0;
  }

  if (--spawntimer==0) {
    spawntimer=Constants::SpawnDelay;
    ennemis.push_back(Ennemi(128,random(0,50-pgm_read_byte(bonhomme+1))));
  }

  uint8_t i=0;
  while(i<tirs.size()) {
    Vec2D& tir = tirs[i];
    tir(0) += 2.5;
    bool rem=tir(0) > 128;

    uint8_t j=0;
    while(j<ennemis.size()) {
      Ennemi& en = ennemis[j];
      bool enrem = false;
      
      if (pointInRect(tir,en.pos,en.size)) {
        rem = true;
        if (--en.life <= 0) {
          enrem = true;
          ++score;
        }
        else {
          beep.tone(beep.freq(750),2);
        }
        en.clignote = 30;
      }
      if (enrem) {
        en.Die();
        ennemis.remove(j);
      }
      else {
        ++j;
      }
    }

    if (rem) tirs.remove(i); else ++i;
  }

  ard.clear();
  player.Draw();

  for(auto&& tir: tirs) {
    ard.drawLine((int)tir(0),(int)tir(1),(int)tir(0)+3,(int)tir(1));
  }

  for(uint8_t i=0; i<ennemis.size();) {
    if (ennemis[i++].UpdateAndDraw()) ennemis.remove(--i);
  }

  Particles::Update();
  Particles::Draw();

  player.DrawHealth();
  if (restarting) {
    Font::PrintString(PSTR("Restarting"), 0, 0);
    ard.drawLine(0,7,uint16_t(FP(restarting)*FP(0.4)),7);
  }
  Font::PrintString(PSTR("SCORE:"), 88, 7);
  Font::PrintInt(score, 112, 7);
  ard.display();
}
