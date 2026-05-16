// jump_game.h
#ifndef JUMP_GAME_H
#define JUMP_GAME_H

// 游戏常量定义
#define JUMP_PLATFORM_WIDTH  25   // 平台宽度
#define JUMP_PLATFORM_HEIGHT 4    // 平台高度
#define JUMP_PLAYER_SIZE     8    // 玩家大小
#define JUMP_PLAYER_Y        110  // 玩家Y坐标（地面高度）
#define JUMP_MIN_DISTANCE    20   // 最小距离
#define JUMP_MAX_DISTANCE    60   // 最大距离
#define JUMP_MIN_POWER       1    // 最小蓄力力度
#define JUMP_MAX_POWER       8   // 最大蓄力力度
#define JUMP_CHARGE_TIME     800  // 最大蓄力时间(ms)

// 平台结构体
struct JumpPlatform {
  int x;           // X坐标
  int y;           // Y坐标
  int width;       // 宽度
  int height;      // 高度
};

// 玩家结构体
struct JumpPlayer {
  int x, y;        // 位置
  int vx, vy;      // 速度
  int size;        // 大小
  bool onGround;   // 是否在地面/平台上
  bool isJumping;  // 是否正在跳跃
  int currentPlatformX; // 当前所在平台的X坐标
  int currentPlatformY; // 当前所在平台的Y坐标
};

// 游戏状态
struct JumpGameState {
  JumpPlatform platforms[15];
  JumpPlayer player;
  int platformCount;
  int score;
  int highScore;
  bool gameOver;
  bool isCharging;        // 是否正在蓄力
  unsigned long chargeStartTime;
  int currentChargePower;
  int nextDistance;
  int cameraX;
};

extern JumpGameState jumpGame;

// 函数声明
void initJumpGame();
void updateJumpGame();
void drawJumpGame();
void drawJumpPlatform(int x, int y, int width, int height);
void drawJumpPlayer(int x, int y, int size);
void drawJumpScore();
void startJumpCharge();
void releaseJumpCharge();
void generateNextPlatform();

#endif