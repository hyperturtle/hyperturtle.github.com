#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <functional>
#include <unordered_map>
#include <utility>
#include <boost/functional/hash/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <algorithm>

using namespace std;

#define MOVE_SPEED 4.0f

#define TEX_SPR 0.03125f
#define TEX_OFF 0.00390625f

#define TILE_SIZEX 50.0f
#define TILE_SIZEY 25.0f

#define REG_TILES 12
#define REG_SIZEX 600 //(REG_TILES*TILE_SIZEX)
#define REG_SIZEY 300 //(REG_TILES*TILE_SIZEY)

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

#define PI 3.14159265f

enum soundname {
  WALK1,
  WALK2,
  HIT1,
  HIT2,
  PICKUP1,
  PICKUP2
};

struct sprite{
  int tiletype;
  int variation;
};

enum thing_t {
  THING_PLAYER,
  THING_ITEM,
  THING_FOOT,
  THING_MOB
};

struct thing{
  int x;
  int y;
  int w;
  int h;
  int life;
  int direction;
  int variation;
  int animation;
  bool kill;
  int invulnerable;
  int hit;
  int speed;
  thing_t type;
};

enum gamestate{
  STATE_MENU,
  STATE_GAME,
  STATE_GAMEOVER
};

class compare_1 { // simple comparison function
   public:
      bool operator()(const pair<int,int>& x,const pair<int,int>& y) const {
        if (x.second == y.second) {
          return x.first < y.first;
        }
        return x.second > y.second;
      } // returns x>y
};

//spatial indexing!
struct mapregion{
  sprite tiles[REG_TILES][REG_TILES];
  list<thing *> things;
  int x;
  int y;
};

typedef pair<int,int> regionXY;
typedef map<regionXY, mapregion *, compare_1> regionsType;
typedef vector<mapregion*> mapRangeType;


array <sf::Sound, 10> sounds;
array <sf::SoundBuffer, 10> soundbuffers;
int bufferCount = 0;
unsigned int clockCounter;
gamestate GameState;
int StateCooldown;

thing player;
int score;
int sprint_power;
list<thing*> things;
regionsType regions;

thing footspawner[50];

int thingtimer;

int shaking;

void SwitchState(gamestate gs){
  if (StateCooldown <= 0){
    GameState = gs;
    StateCooldown = 10;
  }
}

void genMap(int x, int y){
  mapregion *newmap;
  newmap = new mapregion;
  for (int i=0;i<REG_TILES;i++){
    for (int j=0;j<REG_TILES;j++){
      newmap->tiles[i][j].variation = rand()%12;
      newmap->tiles[i][j].tiletype = 0;
    }
  }
  newmap->x = x;
  newmap->y = y;
  regions.insert(regionsType::value_type(make_pair(x,y), newmap));
}


void loadSound(soundname name, const char * file) {
  soundbuffers[bufferCount].LoadFromFile(file);
  sounds[name].SetBuffer(soundbuffers[bufferCount]);
  bufferCount += 1;
}

void loadSounds() {
  loadSound(WALK1, "w1.wav");
  loadSound(WALK2, "w2.wav");
  loadSound(HIT1, "h1.wav");
  loadSound(HIT2, "h2.wav");
  loadSound(PICKUP1, "p1.wav");
  loadSound(PICKUP2, "p2.wav");
}

void playSound(soundname name, float volume = 50.0f){
  sounds[name].SetVolume(volume);
  sounds[name].Play();
}


void shake(int size){
  if (size > 30)
    playSound(HIT1);
  else if (size > 10)
    playSound(HIT2, 20.0f);
  shaking = max(shaking, size);
}


mapRangeType getMapRange(float x, float y, int w, int h){
  vector<mapregion*> ret;
  for(int ix= floor((float)x/REG_SIZEX); ix <= floor((float)(x+w)/REG_SIZEX); ix += 1.0f){
    for(int iy= floor((float)y/REG_SIZEY); iy <= floor((float)(y+h)/REG_SIZEY); iy += 1.0f){
      //cout << "searching " << x << " " << ix << endl;
      if (regions.count(make_pair(ix,iy)) == 0){
        genMap(ix, iy);
      }
      //cout << "(" << ix << ", " << iy << ") ";
      ret.push_back(regions.at(make_pair(ix,iy)));
    }
  }
  //cout << endl;
  return ret;
}


void drawmap(mapRangeType mapRange, int x, int y){
  mapRangeType::iterator mapRangeIterator;
  for(mapRangeIterator = mapRange.begin(); mapRangeIterator != mapRange.end(); mapRangeIterator ++){
    float region_offset_x = x - (*mapRangeIterator)->x * REG_SIZEX;
    float region_offset_y = y - (*mapRangeIterator)->y * REG_SIZEY;

    //cout << (*mapRangeIterator)->x << "," << (*mapRangeIterator)->y << endl;

    for (int i=0;i<REG_TILES;i++){
      for (int j=0;j<REG_TILES;j++){
        float tx = (14+(*mapRangeIterator)->tiles[i][j].tiletype * 2)*TEX_SPR;
        float ty = (2+(*mapRangeIterator)->tiles[i][j].variation)*TEX_SPR;
        float tw = 2*TEX_SPR - TEX_OFF;
        float th = 1*TEX_SPR - TEX_OFF;
        float offset_x = i*TILE_SIZEX - region_offset_x;
        float offset_y = j*TILE_SIZEY - region_offset_y;
        glTexCoord2f(tx     , ty     ); glVertex3f(offset_x  , offset_y  , 0);
        glTexCoord2f(tx + tw, ty     ); glVertex3f(offset_x+TILE_SIZEX, offset_y  , 0);
        glTexCoord2f(tx + tw, ty + th); glVertex3f(offset_x+TILE_SIZEX, offset_y+TILE_SIZEY, 0);
        glTexCoord2f(tx     , ty + th); glVertex3f(offset_x  , offset_y+TILE_SIZEY, 0);
      }
    }
  }
}

set<thing*> getThings(mapRangeType mapRange, int x, int y, int w, int h){
  set<thing*> ret;

  mapRangeType::iterator mapRangeIterator;
  for(mapRangeIterator = mapRange.begin(); mapRangeIterator != mapRange.end(); mapRangeIterator ++){
    for(list<thing*>::iterator it = (*mapRangeIterator)->things.begin(); it != (*mapRangeIterator)->things.end(); it ++){
      if ( (((*it)->x - x) < w       ) &&
           ((x - (*it)->x) < (*it)->w) &&
           (((*it)->y - y) < h       ) &&
           ((y - (*it)->y) < (*it)->h)){
        ret.insert(*it);
      }
    }
  } 
  return ret; 
}

void drawShadows(set<thing*> *setOfThings, const int x, const int y){
  
  

  for(set<thing*>::iterator it=(*setOfThings).begin(); it != (*setOfThings).end(); it ++){
    int offset_x, offset_y, w, h;
    offset_x = (*it)->x - x;
    offset_y = (*it)->y - y;
    w = (*it)->w;
    h = (*it)->h;

    

    if ((*it)->type == THING_FOOT){      
      glColor4f(0,0,0, 1.0f - abs((*it)->life) * 0.001f);
    } else {
      glColor4f(0,0,0,0.5f);
    }

    if ((*it)->type == THING_FOOT){
      glTexCoord2f(20*TEX_SPR                    , 8*TEX_SPR                    ); glVertex3f(offset_x  , offset_y  , 0);
      glTexCoord2f(20*TEX_SPR + TEX_SPR - TEX_OFF, 8*TEX_SPR                    ); glVertex3f(offset_x+w, offset_y  , 0);
      glTexCoord2f(20*TEX_SPR + TEX_SPR - TEX_OFF, 8*TEX_SPR - TEX_OFF + TEX_SPR); glVertex3f(offset_x+w, offset_y+h, 0);
      glTexCoord2f(20*TEX_SPR                    , 8*TEX_SPR - TEX_OFF + TEX_SPR); glVertex3f(offset_x  , offset_y+h, 0);
    }
    //cout << offset_x << " - " << offset_x + w << " x " << offset_y << " - " << offset_y + h << " ";
    //cout << offset_x << " - " << offset_x + w << "   ";
    
  }
}

void drawThings(set<thing*> *setOfThings, const int x, const int y) {
  float tx, ty, tw, th, w, h, offset_x, offset_y;

  for(set<thing*>::iterator it=(*setOfThings).begin(); it != (*setOfThings).end(); it ++){
    offset_x = (*it)->x - x;
    offset_y = (*it)->y - y;

    if ((*it)->type == THING_FOOT){
      tx = 21 * TEX_SPR;
      ty = 2 * TEX_SPR;
      tw = 2 * TEX_SPR - TEX_OFF;
      th = 4 * TEX_SPR - TEX_OFF;
      w = (*it)->w;
      h = (*it)->h;
      ty = (2+4*(*it)->variation) * TEX_SPR;
      h = 970;
      offset_y = offset_y - h + (*it)->h - abs((*it)->life * (*it)->speed);

      if ((*it)->life == 0) {
        shake(10);
      }

      if ((*it)->direction == 1) {
        tx = tx + tw;
        tw = -tw;
      } else if ((*it)->direction == 2) {
        tx = 23 * TEX_SPR;
        tw = TEX_SPR - TEX_OFF;
      } else if ((*it)->direction == 3) {
        tx = 24 * TEX_SPR;
        tw = TEX_SPR - TEX_OFF;
      }

      glColor4f(1,1,1,1);
      glTexCoord2f(tx     , ty     ); glVertex3f(offset_x  , offset_y  , 0);
      glTexCoord2f(tx + tw, ty     ); glVertex3f(offset_x+w, offset_y  , 0);
      glTexCoord2f(tx + tw, ty + th); glVertex3f(offset_x+w, offset_y+h, 0);
      glTexCoord2f(tx     , ty + th); glVertex3f(offset_x  , offset_y+h, 0);

    } else if ((*it)->type == THING_PLAYER){
      int anim = (*it)->animation / 3;

      tw = 0.0625f - TEX_OFF;
      th = 0.09375f - TEX_OFF;
      ty = (3*anim + 2)*TEX_SPR;

      //offset_x += 2;
      offset_y -= 50;

      if ((*it)->direction == 0) {
        tx = 0;
      }
      else if ((*it)->direction == 1) {
        tx = 0+tw;
        tw = -tw;
        offset_x -= 2;
      } else if ((*it)->direction == 2) {
        tx = 2*TEX_SPR;
      } else if ((*it)->direction == 3) {
        tx = 4*TEX_SPR;
      }

      
      
      w = 56;
      h = 92;

      float bounceh = (sin((float)(anim-1)*2*PI/6.0f)+1.0f)*3.0f;

      if (((*it)->hit > 0) && (clockCounter%3==0)) {
        glColor4f(1,1,1,clockCounter%2);
      } else if ((*it)->invulnerable > 0){
        glColor4f(1,1,1, 1.0f - 0.01f*(*it)->invulnerable );
      } else {
        glColor4f(1,1,1,1);
      }
      glTexCoord2f(tx     , ty     ); glVertex3f(offset_x  , offset_y -  bounceh  , 0);
      glTexCoord2f(tx + tw, ty     ); glVertex3f(offset_x+w, offset_y -  bounceh  , 0);
      glTexCoord2f(tx + tw, ty + th); glVertex3f(offset_x+w, offset_y+h - bounceh, 0);
      glTexCoord2f(tx     , ty + th); glVertex3f(offset_x  , offset_y+h - bounceh, 0);
    } else if ((*it)->type == THING_ITEM){

      tx = (19 + (*it)->variation / 5) * TEX_SPR;
      ty = (2 + (*it)->variation % 5) * TEX_SPR;
      tw = 1 * TEX_SPR - TEX_OFF;
      th = 1 * TEX_SPR - TEX_OFF;
      w = (*it)->w;
      h = (*it)->h;
      glColor4f(1,1,1,1);
      glTexCoord2f(tx     , ty     ); glVertex3f(offset_x  , offset_y  , 0);
      glTexCoord2f(tx + tw, ty     ); glVertex3f(offset_x+w, offset_y  , 0);
      glTexCoord2f(tx + tw, ty + th); glVertex3f(offset_x+w, offset_y+h, 0);
      glTexCoord2f(tx     , ty + th); glVertex3f(offset_x  , offset_y+h, 0);
    }
  }
}

bool thingYComp(thing* a, thing* b){
  return (a->y+a->h) < (b->y+b->h);
}

void indexThing(thing * thingToIndex) {
  regionXY xy;
  for(int ix= floor((float)thingToIndex->x/REG_SIZEX); ix <= floor((float)(thingToIndex->x+thingToIndex->w)/REG_SIZEX); ix += 1.0f){
    for(int iy= floor((float)thingToIndex->y/REG_SIZEY); iy <= floor((float)(thingToIndex->y+thingToIndex->h)/REG_SIZEY); iy += 1.0f){
      xy = make_pair(ix,iy);
      if (regions.count(xy) > 0){
        regions[xy]->things.push_back(thingToIndex);
        regions[xy]->things.sort(thingYComp);
        //sort(regions[xy]->things.begin(), regions[xy]->things.end(), thingYComp);
      }
    }
  }
}

void addThing(thing * thingToAdd) {
  thingToAdd->kill = false;
  things.push_back(thingToAdd);
  //things.sort(thingYComp);
  indexThing(thingToAdd);
}

void removeFromRegions(thing* thingToRemove) {
  for(int ix= floor((float)thingToRemove->x/REG_SIZEX); ix <= floor((float)(thingToRemove->x+thingToRemove->w)/REG_SIZEX); ix += 1.0f){
    for(int iy= floor((float)thingToRemove->y/REG_SIZEY); iy <= floor((float)(thingToRemove->y+thingToRemove->h)/REG_SIZEY); iy += 1.0f){
      //cout << ix << ", " << iy << endl;
      if (regions.count(make_pair(ix,iy)) > 0){
        for(auto listiter=regions[make_pair(ix,iy)]->things.begin(); listiter != regions[make_pair(ix,iy)]->things.end(); listiter++ ){
          if(*listiter == thingToRemove){
            regions[make_pair(ix,iy)]->things.erase(listiter);
            break;
          }
        }
      }
    }
  }
}

//need to use this to make sure everything is in the right spatial index
void moveThing(thing* thingToMove, int x, int y){
  removeFromRegions(thingToMove);
  thingToMove->x += x;
  thingToMove->y += y;
  indexThing(thingToMove);
}

void updateThings(){
  vector<list<thing*>::iterator> toDelete;

  

  if(player.hit > 0)
    player.hit --;

  if(player.invulnerable > 0) {
    player.invulnerable --;
  } else {
    mapRangeType mapRange = getMapRange(player.x, player.y, player.w, player.h);
    set<thing*> setOfThings = getThings(mapRange, player.x, player.y, player.w, player.h);
    for(set<thing*>::iterator it=setOfThings.begin(); it != setOfThings.end(); it ++){
      if ((*it)->type == THING_FOOT && ((*it)->life < 100) && ((*it)->life >= 0)) {
        shake(50);
        player.life -= 1;
        player.hit = 20;
        player.invulnerable = 20;
      } else if ((*it)->type == THING_ITEM){          
        score += 1;
        if ((*it)->variation == 5) {
          player.life = min(10, player.life + 1);
          playSound(PICKUP2, 15.0f);
        } else {
          playSound(PICKUP1, 15.0f);
        }
        (*it)->kill = true;
      }
    }
  }

  for (list<thing*>::iterator it = things.begin(); it != things.end(); it ++) {
    if ((*it)->kill) {
      toDelete.push_back(it);
      continue;
    }
    if ((*it)->type == THING_FOOT){
      (*it)->life = max(-1000, (*it)->life - 10);

      if ((*it)->life == -1000) {
        (*it)->kill = true;
      }
    } else if ((*it)->type == THING_ITEM){
      if ((abs((*it)->x - player.x) > 2000) || (abs((*it)->y - player.y) > 2000)){
        toDelete.push_back(it);
      }
    }
  }
  

  

  for(vector<list<thing*>::iterator>::iterator it = toDelete.begin(); it != toDelete.end(); it ++){
    removeFromRegions(**it);
    things.erase(*it);
    delete **it;
  }
}



void spawnfoot(int x, int y, int variation, int speed, int direction) {
  thing *newthing = new thing;
  newthing->type = THING_FOOT;
  newthing->x = x;
  newthing->y = y;
  if ((direction == 0) || (direction == 1)) {
    newthing->w = 483;
    newthing->h = 100;
  } else {
    newthing->w = 100;
    newthing->h = 483;
  }
  newthing->life = 1000;
  newthing->variation = variation;
  newthing->speed = speed;
  newthing->direction = direction;
  addThing(newthing);
}

void spawnitem(int x, int y){
  thing *newthing = new thing;
  newthing->type = THING_ITEM;
  newthing->x = x;
  newthing->y = y;
  newthing->w = 50;
  newthing->h = 50;
  newthing->variation = rand()%10;
  addThing(newthing);
}

void updatefootspawner(thing* footspawner){
  if (footspawner->life == 0) {

    

    if ( (abs( footspawner->x - player.x ) > 2000 + rand()%1000) || (abs( footspawner->y - player.y ) > 2000+ rand()%1000) ) {
      footspawner->direction = rand()%4;
      if (footspawner->direction == 0) {
        footspawner->x = player.x+2000;
        footspawner->y = player.y;
      } else if (footspawner->direction == 1) {
        footspawner->x = player.x-2000;
        footspawner->y = player.y;
      } else if (footspawner->direction == 2) {
        footspawner->x = player.x;
        footspawner->y = player.y+2000;
      } else if (footspawner->direction == 3) {
        footspawner->x = player.x;
        footspawner->y = player.y-2000;
      }

      footspawner->x +=  rand()%2000 - 1000;
      footspawner->y +=  rand()%2000 - 1000;
      footspawner->speed = rand()%3 + 1;
      footspawner->variation = rand()%4;
      footspawner->animation = 1;
    }

    spawnfoot( footspawner->x, footspawner->y, footspawner->variation, footspawner->speed, footspawner->direction);

    if (footspawner->direction == 0) {
      footspawner->x -= 500;
      footspawner->y -= footspawner->animation*100;
    } else if (footspawner->direction == 1) {
      footspawner->x += 500;
      footspawner->y -= footspawner->animation*100;
    } else if (footspawner->direction == 2) {
      footspawner->y -= 500;
      footspawner->x -= footspawner->animation*100;
    } else if (footspawner->direction == 3) {
      footspawner->y += 500;
      footspawner->x -= footspawner->animation*100;
    }
  
    footspawner->life = 50 - footspawner->speed*10;
    footspawner->animation = - footspawner->animation;

  } else {
    footspawner->life = max(0, footspawner->life - footspawner->speed);
  }
}

void drawstring(const char *word, float x, float y, float s){
  float letter_x;
  float letter_y;
  float offset_x;
  float offset_y;
  for(unsigned int i = 0; i < strlen(word); i ++){
    letter_x = (((int)word[i] - 32)%32) * TEX_SPR;
    letter_y = (((int)word[i] - 32)/32) * TEX_SPR;
    offset_x = x + i*s*1.142857143f;
    offset_y = y;
    glTexCoord2f(letter_x, letter_y);                                         glVertex3f(offset_x, offset_y, 0);
    glTexCoord2f(letter_x + TEX_SPR - TEX_OFF, letter_y);                     glVertex3f(offset_x+s, offset_y, 0);
    glTexCoord2f(letter_x + TEX_SPR - TEX_OFF, letter_y + TEX_SPR - TEX_OFF); glVertex3f(offset_x+s, offset_y+s, 0);
    glTexCoord2f(letter_x, letter_y + TEX_SPR - TEX_OFF);                     glVertex3f(offset_x, offset_y+s, 0);
  }
}

void drawhearts(){
  float tx = 18 * TEX_SPR;
  float ty = 2 * TEX_SPR;
  float tw = 1 * TEX_SPR - TEX_OFF;
  float th = 1 * TEX_SPR - TEX_OFF;
  float w = 21;
  float h = 21;
  float offset_x, offset_y;
  offset_y = 84;

  for(int i = 0; i < player.life; i++){
    offset_x = i*(w+14)+14;
    glColor4f(1,1,1,1);
    glTexCoord2f(tx     , ty     ); glVertex3f(offset_x  , offset_y  , 0);
    glTexCoord2f(tx + tw, ty     ); glVertex3f(offset_x+w, offset_y  , 0);
    glTexCoord2f(tx + tw, ty + th); glVertex3f(offset_x+w, offset_y+h, 0);
    glTexCoord2f(tx     , ty + th); glVertex3f(offset_x  , offset_y+h, 0);
  }
}

void drawsprintbar(){
  float offset_x, offset_y, w, h;
  offset_x = 0;
  offset_y = 600-14;
  
  h = 14;
  

  w = 800;

  glColor4f(0,0,0,1);
  glTexCoord2f(20*TEX_SPR                    , 8*TEX_SPR                    ); glVertex3f(offset_x  , offset_y  , 0);
  glTexCoord2f(20*TEX_SPR + TEX_SPR - TEX_OFF, 8*TEX_SPR                    ); glVertex3f(offset_x+w, offset_y  , 0);
  glTexCoord2f(20*TEX_SPR + TEX_SPR - TEX_OFF, 8*TEX_SPR - TEX_OFF + TEX_SPR); glVertex3f(offset_x+w, offset_y+h, 0);
  glTexCoord2f(20*TEX_SPR                    , 8*TEX_SPR - TEX_OFF + TEX_SPR); glVertex3f(offset_x  , offset_y+h, 0);

  w = sprint_power*8;

  glColor4f(0.3f,0.6f,1,1);
  glTexCoord2f(20*TEX_SPR                    , 8*TEX_SPR                    ); glVertex3f(offset_x  , offset_y  , 0);
  glTexCoord2f(20*TEX_SPR + TEX_SPR - TEX_OFF, 8*TEX_SPR                    ); glVertex3f(offset_x+w, offset_y  , 0);
  glTexCoord2f(20*TEX_SPR + TEX_SPR - TEX_OFF, 8*TEX_SPR - TEX_OFF + TEX_SPR); glVertex3f(offset_x+w, offset_y+h, 0);
  glTexCoord2f(20*TEX_SPR                    , 8*TEX_SPR - TEX_OFF + TEX_SPR); glVertex3f(offset_x  , offset_y+h, 0);
}

string words[] = {"OF","MICE","AND","GIANTS", "ARROW KEYS TO MOVE", "HOLD X TO SPRINT", "PRESS X TO START"};
string words2[] = {"GAMEOVER","YOU GOT","THINGS!", "PRESS X TO CONTINUE"};

void gameoverloop(const sf::Input* Input){
  if (Input->IsKeyDown(sf::Key::X)){
    score = 0;
    SwitchState(STATE_MENU);
  }
  string status;
  status = boost::lexical_cast<string>(score);

  glBegin(GL_QUADS);
  
  glColor4f(1,0,0,1);
  drawstring(words2[0].c_str(), 70, 70+0, 49);
  glColor4f(0.1f,0.1f,0.1f,1);
  drawstring(words2[1].c_str(), 70, 70+93, 35);
  drawstring(status.c_str(), 70, 70+186, 35);
  drawstring(words2[2].c_str(), 70, 70+279, 35);
  glColor4f(0.3f,0.6f,0.9f,1);
  drawstring(words2[3].c_str(), 70, 70+372, 21);
  glEnd();
}

void menuloop(const sf::Input* Input){
  if (Input->IsKeyDown(sf::Key::X)){

    thingtimer = 100;

    player.life = 3;
    player.x = 0;
    player.y = 0;

    for(int i = 0; i < 10; i ++){
      footspawner[i].life = 0;
      footspawner[i].x = 9999;
      footspawner->animation = 1;
    }

    SwitchState(STATE_GAME);
  }

  

  glBegin(GL_QUADS);
  glColor4f(0.1f,0.1f,0.1f,1);
  drawstring(words[0].c_str(), 70, 70+0, 84);
  drawstring(words[1].c_str(), 70, 70+93, 84);
  drawstring(words[2].c_str(), 70, 70+186, 84);
  drawstring(words[3].c_str(), 70, 70+279, 84);
  drawstring(words[4].c_str(), 70, 70+372, 21);
  drawstring(words[5].c_str(), 70, 70+418, 21);
  glColor4f(0.3f,0.6f,0.9f,1);
  drawstring(words[6].c_str(), 70, 70+465, 21);
  glEnd();
}

void gameloop(const sf::Input* Input){
  bool moving;
  float move_mod;
  moving = false;
  if ((Input->IsKeyDown(sf::Key::Up) || Input->IsKeyDown(sf::Key::Down)) && (Input->IsKeyDown(sf::Key::Right) || Input->IsKeyDown(sf::Key::Left))){
    move_mod = 2.0f * 0.707f;
  } else{
    move_mod = 2.0f * 1.0f;
  }

  if (Input->IsKeyDown(sf::Key::X) && sprint_power > 5){
    move_mod *= 2;
    sprint_power = max(0, sprint_power - 5);
  } else {
    sprint_power = min(100, sprint_power + 1);
  }
  
  if (player.hit == 0) {
    removeFromRegions(&player);
    if (Input->IsKeyDown(sf::Key::Up)){
      player.y -= move_mod * MOVE_SPEED;
      player.direction = 3;
      moving = true;
    }
    if (Input->IsKeyDown(sf::Key::Down)){
      player.y += move_mod * MOVE_SPEED;
      player.direction = 2;
      moving = true;
    }
    if (Input->IsKeyDown(sf::Key::Left)){
      player.x -= move_mod * MOVE_SPEED;
      player.direction = 1;
      moving = true;
    }
    if (Input->IsKeyDown(sf::Key::Right)){
      player.x += move_mod * MOVE_SPEED;
      player.direction = 0;
      moving = true;
    }
    indexThing(&player);
  }

  

  if (Input->IsKeyDown(sf::Key::C)){
    //shake(20);
  }
  
  //do game stuff here
  
  for(int i = 0; i < min(50, score/3); i ++){
    updatefootspawner(&footspawner[i]);
  }

  thingtimer = max(0, thingtimer - 1);
  if (thingtimer == 0){
    int direction = rand()%4;
    if (direction == 0){
      spawnitem(player.x + 500, player.y + rand()%1000 - 500);
    } else if (direction == 1){
      spawnitem(player.x - 500, player.y + rand()%1000 - 500);
    } else if (direction == 2){
      spawnitem(player.x + rand()%1000 - 500, player.y + 500);
    } else if (direction == 3){
      spawnitem(player.x + rand()%1000 - 500, player.y + -500);
    }
    
    thingtimer = 15;
  }
  
  

  if (moving) {
    player.animation = (player.animation+1) % 21;

    if(player.animation == 1) {
      playSound(WALK1,25.0f);
    } else if (player.animation == 11) {
      playSound(WALK2,25.0f);
    }
  } else {
    player.animation = 0;
  }

  if (shaking > 0) {
    glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      float x = floor((float)(shaking)*((rand() % 100) * 0.01f));
      float y = floor((float)(shaking)*((rand() % 100) * 0.01f));
      glOrtho(x, x+WIN_WIDTH, y+WIN_HEIGHT, y, 1, -1);
    shaking -= 1;
  }

  //cout << "updating things" << endl;
  updateThings();

  if (player.life <= 0){
    SwitchState(STATE_GAMEOVER);
  }

  

  glBegin(GL_QUADS);
  

  //cout << "start drawing" << endl;

  glColor4f(1,1,1,1);

  int screenoffsetx = player.x-(WIN_WIDTH-player.w)/2;
  int screenoffsety = player.y-(WIN_HEIGHT)/2;

  mapRangeType mapRange;
  mapRange = getMapRange(screenoffsetx, screenoffsety, WIN_WIDTH, WIN_HEIGHT);
  set<thing*> shownthings = getThings(mapRange, screenoffsetx, screenoffsety, WIN_WIDTH, WIN_HEIGHT);

  drawmap(mapRange, screenoffsetx, screenoffsety);
  drawShadows(&shownthings, screenoffsetx, screenoffsety);
  drawThings(&shownthings, screenoffsetx, screenoffsety);
  glColor4f(1,1,1,1);

  glColor4f(0.1f,0.1f,0.1f,1);
  string status;// = boost::lexical_cast<string>(player.x) + "," + boost::lexical_cast<string>(player.y);
  //drawstring(status.c_str(), 0, 0, 1);

  status = boost::lexical_cast<string>(score) + " THINGS";
  drawstring(status.c_str(), 7, 7, 49);
  

  drawhearts();
  //status = boost::lexical_cast<string>(player.life);
  //drawstring(status.c_str(), 7, 84, 14);

  drawsprintbar();

  glColor4f(0.1f,0.1f,0.1f,1);
  drawstring(words[4].c_str(), 7, 600-56, 14);
  drawstring(words[5].c_str(), 7, 600-35, 14);

  //status = boost::lexical_cast<string>(sprint_power);
  //drawstring(status.c_str(), 7, 104, 14);
  

  glEnd();
}



int main(int argc, char* argv[]) {

  
  sf::Clock Clock;

  sf::WindowSettings Settings;
  Settings.DepthBits         = 24; // Request a 24 bits depth buffer
  Settings.StencilBits       = 8;  // Request a 8 bits stencil buffer
  Settings.AntialiasingLevel = 0;  // Request 2 levels of antialiasing
  sf::Window App(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT, 32), "OF MICE AND GIANTS", sf::Style::Close, Settings);

  sf::Image Image;

  if (!Image.LoadFromFile("sprite.png"))
  {
    return 0;
  }

  loadSounds();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, Image.GetWidth(),
      Image.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
      Image.GetPixelsPtr());
  
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


  glClearColor(0.9f, 0.9f, 0.9f, 1);

  glClearDepth(1.0f);
  glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

  glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIN_WIDTH, WIN_HEIGHT, 0, 1, -1);
  
  glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

  glEnable(GL_TEXTURE_2D);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float time_now = Clock.GetElapsedTime();

  srand ( time_now );

  

  const sf::Input& Input = App.GetInput();

  shaking = 0;

  

  
  

  sf::Music Music;
  Music.OpenFromFile("music.ogg");
  Music.SetVolume(25.0f);
  Music.SetLoop(true);
  Music.Play();

  cout << "generating map" << endl;

  
  for (int xx = -100; xx <= 100; xx ++) {
    for (int yy = -100; yy <= 100; yy ++) {
      genMap(xx, yy);
    }
  }


  

  
  player.w = 50;
  player.h = 50;
  player.direction = 0;
  player.animation = 0;
  player.type = THING_PLAYER;
  


  
  
  //addThing(&player);

  cout << "starting loop" << endl;

  GameState = STATE_MENU;
  StateCooldown = 0;

  while (App.IsOpened()) {
    sf::Event Event;
    while (App.GetEvent(Event)) {
      if (Event.Type == sf::Event::Closed)
        App.Close();
    }



    time_now = Clock.GetElapsedTime();

    if(time_now > 1.0f/60.0f) {
      clockCounter += 1;

      StateCooldown = max (0, StateCooldown - 1);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      

      if (GameState == STATE_MENU) {
        menuloop(&Input);
      } else if (GameState == STATE_GAME) {
        gameloop(&Input);
      } else if (GameState == STATE_GAMEOVER) {
        gameoverloop(&Input);
      }

      

      

      App.Display();
      Clock.Reset();
    } else {
      sf::Sleep(1.0f/59.0f - time_now);
    }
  }

  return 0;
}