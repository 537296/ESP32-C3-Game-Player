// jump_game.h
#ifndef JUMP_GAME_H
#define JUMP_GAME_H

#define MAX_PLATFORMS 8   

#define JUMP_PLATFORM_WIDTH  24
#define JUMP_PLATFORM_HEIGHT 4
#define JUMP_PLAYER_SIZE     8
#define JUMP_PLAYER_Y        110
#define JUMP_MIN_DISTANCE    20
#define JUMP_MAX_DISTANCE    60
#define JUMP_MIN_POWER       3
#define JUMP_MAX_POWER       12
#define JUMP_CHARGE_TIME     600

struct JumpPlatform {
  int x;
  int y;
  int width;
  int height;
  bool active;  
};

struct JumpPlayer {
  int x, y;
  int vx, vy;
  int size;
  bool onGround;
  bool isJumping;
  int currentPlatformX;
  int currentPlatformY;
};

struct JumpGameState {
  JumpPlatform platforms[MAX_PLATFORMS];
  JumpPlayer player;
  int platformCount;     
  int nextPlatformId;     
  int score;
  int highScore;
  bool gameOver;
  bool isCharging;
  unsigned long chargeStartTime;
  int currentChargePower;
  int nextDistance;
  int cameraX;
  int lastPlatformX;     
};

extern JumpGameState jumpGame;

void initJumpGame();
void updateJumpGame();
void drawJumpGame();
void drawJumpPlatform(int x, int y, int width, int height);
void drawJumpPlayer(int x, int y, int size);
void drawJumpScore();
void startJumpCharge();
void releaseJumpCharge();
void generateNextPlatform();
void recycleOldPlatforms();  

#endif