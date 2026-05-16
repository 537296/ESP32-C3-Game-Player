#include "set1img.h"
#include "set2img.h"
#include "set3img.h"
#include "set4img.h"
#include "img1.h"
#include "img_1.h"
#include "img_2.h"
#include "img_3.h"
#include "img_4.h"
#include "img_5.h"
#include "img_6.h"
#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include "ST7735_HW_SPI.h"
#include <pgmspace.h>
#include <esp32-hal-ledc.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <WiFi.h>
#include <time.h>
#include <esp_pm.h>
#include "jump_game.h"
#define BL_PIN        7
#define BTN_UP        5  
#define BTN_DOWN      9  
#define BTN_LEFT      10  
#define BTN_OK        6   
#define DHT_PIN       21
#define BUZZER_PIN    8  
#include "config.h"
#define MINE_SIZE 10 

struct WiFiNetwork {
  const char* ssid;
  const char* password;
};
JumpGameState jumpGame;

WiFiNetwork wifiNetworks[5] = {
  {WIFI_1_SSID, WIFI_1_PASSWORD},
  {WIFI_2_SSID, WIFI_2_PASSWORD},
  {WIFI_3_SSID, WIFI_3_PASSWORD},
  {WIFI_4_SSID, WIFI_4_PASSWORD},
  {WIFI_5_SSID, WIFI_5_PASSWORD}
};

int wifiNetworkCount = WIFI_NETWORK_COUNT;

int selectedWifiIndex = 0;     // 当前选中的WiFi索引
bool wifiSelectMode = false;   // 是否处于WiFi选择模式



bool wifiConnected = false;
String timeSyncStatus = "Not started";

ST7735_HW_SPI tft;

unsigned long lastPress = 0; 


DHT dht(DHT_PIN, DHT11);


float temperature = 0.0;
float humidity = 0.0;
unsigned long lastDHTReadTime = 0;
const unsigned long DHT_READ_INTERVAL = 2000;
bool dhtSensorError = false;


// ========== UI状态 ==========
enum UIState {
  STATE_SPLASH,
  STATE_MAIN_MENU,
  STATE_GAME_SELECT,
  STATE_GAME_DINO,
  STATE_SETTINGS,
  STATE_SETTING_DETAIL,
  STATE_TETRIS,
  STATE_ABOUT,
  STATE_ALARM_SETTING,
  STATE_TIME_SETTING,
  STATE_BRIGHTNESS_SETTING,
  STATE_NETWORK_TIME,
  STATE_CALCULATOR,
  STATE_MINESWEEPER ,
  STATE_MINE_COUNT_SETTING,
  STATE_JUMP_GAME
};

UIState currentState = STATE_SPLASH;
UIState lastState = STATE_SPLASH;
int menuSelection = 0;
int gameSelection = 1;
int settingsSelection = 0;


int mineCount = 15;  
const int minMineCount = 5;
const int maxMineCount = 30;
int mineBackgroundIndex = 0;  
const uint8_t* getMineBackground() {
  switch(mineBackgroundIndex) {
    case 0: return nullptr;      
    case 1: return gImage_set1img; 
    case 2: return gImage_set2img; 
    case 3: return gImage_set3img; 
    case 4: return gImage_set4img; 
    default: return nullptr;
  }
}
const int maxBackgroundIndex = 4;

enum CellState {
  CELL_HIDDEN = 0,   // 未翻开
  CELL_REVEALED = 1, // 已翻开
  CELL_FLAGGED = 2   // 标记旗子
};

// 游戏数据
int mineField[MINE_SIZE][MINE_SIZE];      
CellState cellState[MINE_SIZE][MINE_SIZE]; // 格子状态
bool mineGameOver = false;
bool mineGameWin = false;
int mineCursorX = 0;  
int mineCursorY = 0;
int revealedCount = 0;  
int flaggedCount = 0;   
#define MINE_SIZE 10  


int mineCellSize = 12;  
int mineGridWidth = MINE_SIZE * mineCellSize;
int mineStartX = (SCREEN_WIDTH - mineGridWidth) / 2; 
int mineStartY = (SCREEN_HEIGHT - mineGridWidth) / 2; 


int lastMineCursorX = 0;
int lastMineCursorY = 0;
#define MINE_BG_COLOR       0x1DFC  
#define MINE_REVEALED_COLOR 0xDEFB  
#define MINE_GRID_COLOR     0x7BEF  

// ========== 俄罗斯方块 ==========
#define TETRIS_COLS 10
#define TETRIS_ROWS 18      
#define TETRIS_CELL_SIZE 5   

const int tetrisShapes[5][4][4] = {
  // I 形
  {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
  // O 形
  {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
  // T 形
  {{1,1,1,0}, {0,1,0,0}, {0,0,0,0}, {0,0,0,0}},
  // L 形
  {{1,1,1,0}, {1,0,0,0}, {0,0,0,0}, {0,0,0,0}},
  // Z 形
  {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}
};

// 方块颜色
const uint16_t tetrisColors[5] = {CYAN, YELLOW, MAGENTA, BLUE, GREEN};

// 游戏数据
int tetrisBoard[TETRIS_ROWS][TETRIS_COLS];
int currentShape[4][4];
int currentShapeType;
uint16_t currentShapeColor;
int currentX, currentY;
bool tetrisGameOver = false;
int tetrisScore = 0;
unsigned long lastFallTime = 0;
int tetrisSpeed = 500;  
const int minTetrisSpeed = 100;   
const int maxTetrisSpeed = 1000;  
int tetrisSpeedSelectIndex = 0;   
int tetrisStartX = 60;
int tetrisStartY = 15;
int nextShape[4][4];
int nextShapeType;
uint16_t nextShapeColor;

// 预览区域位置
int previewX = 2;
int previewY = tetrisStartY + 10;
int previewSize = 4;  // 预览方块大小

String calcExpression = "";  
String calcResult = "";      
bool calcError = false;      
bool needClearOnNextInput = false;  

// ========== 计算器按钮定义 ==========
struct CalcButton {
  char label[5];
  int x, y;
};

CalcButton calcButtons[20] = {

  {"7", 0, 0}, {"8", 0, 0}, {"9", 0, 0}, {"/", 0, 0}, {"C", 0, 0},

  {"4", 0, 0}, {"5", 0, 0}, {"6", 0, 0}, {"*", 0, 0}, {"^", 0, 0},

  {"1", 0, 0}, {"2", 0, 0}, {"3", 0, 0}, {"-", 0, 0}, {"R", 0, 0},

  {".", 0, 0}, {"0", 0, 0}, {"=", 0, 0}, {"+", 0, 0}, {"D", 0, 0}
};
int calcCursorX = 0;
int calcCursorY = 0;

double evaluateExpression(String expr) {
  // 移除空格
  expr.replace(" ", "");
  
  if (expr.length() == 0) return 0;
  
  
  for (int i = 0; i < expr.length(); i++) {
    if (expr[i] == '^') {
   
      int leftStart = i - 1;
      while (leftStart >= 0 && (isDigit(expr[leftStart]) || expr[leftStart] == '.')) {
        leftStart--;
      }
      leftStart++;
      
     
      int rightEnd = i + 1;
      while (rightEnd < expr.length() && (isDigit(expr[rightEnd]) || expr[rightEnd] == '.')) {
        rightEnd++;
      }
      rightEnd--;
      
      if (leftStart < i && rightEnd > i) {
        double base = expr.substring(leftStart, i).toDouble();
        double exponent = expr.substring(i + 1, rightEnd + 1).toDouble();
        double result = pow(base, exponent);
        
        String before = expr.substring(0, leftStart);
        String after = expr.substring(rightEnd + 1);
        expr = before + String(result, 10) + after;  
        i = leftStart - 1;  // 重新扫描
      }
    }
  }
  

  double result = 0;
  double currentNumber = 0;
  char lastOp = '+';
  bool hasDecimal = false;
  int decimalPlaces = 0;
  
  for (int i = 0; i <= expr.length(); i++) {
    char c = (i < expr.length()) ? expr[i] : '\0';
    
    if (isDigit(c)) {
      if (hasDecimal) {
        decimalPlaces++;
        currentNumber = currentNumber + (c - '0') / pow(10, decimalPlaces);
      } else {
        currentNumber = currentNumber * 10 + (c - '0');
      }
    } 
    else if (c == '.') {
      hasDecimal = true;
    }
    else {
 
      if (lastOp == '+') {
        result += currentNumber;
      } else if (lastOp == '-') {
        result -= currentNumber;
      } else if (lastOp == '*') {
        result *= currentNumber;
      } else if (lastOp == '/') {
        if (currentNumber != 0) {
          result /= currentNumber;
        } else {
          calcError = true;
          return 0;
        }
      }
      
      currentNumber = 0;
      hasDecimal = false;
      decimalPlaces = 0;
      lastOp = c;
    }
  }
  
  return result;
}


String formatResult(double value) {
  if (calcError) return "ERROR";
  
  // 检查是否为整数
  if (abs(value - round(value)) < 0.000001) {
    return String((int)round(value));
  }
  

  char buffer[32];
  

  for (int decimals = 5; decimals >= 0; decimals--) {
    double multiplier = pow(10, decimals);
    double rounded = round(value * multiplier) / multiplier;
    
    if (abs(value - rounded) < 0.000001) {
      if (decimals == 0) {
        sprintf(buffer, "%d", (int)round(value));
      } else {
        
        char format[10];
        sprintf(format, "%%.%df", decimals);
        sprintf(buffer, format, value);
        
       
        int len = strlen(buffer);
        while (len > 0 && buffer[len-1] == '0') {
          buffer[len-1] = '\0';
          len--;
        }
        
        if (len > 0 && buffer[len-1] == '.') {
          buffer[len-1] = '\0';
        }
      }
      return String(buffer);
    }
  }
  
  
  sprintf(buffer, "%.5f", value);
  return String(buffer);
}


const char* mainMenuItems[] = {"GAMES", "SETTINGS", "ABOUT"};
const int mainMenuItemCount = 3;
const int gameItemCount = 6;


#define BRIGHTNESS_LEVELS 5
const int brightnessValues[BRIGHTNESS_LEVELS] = {8, 64, 128, 192, 255};
int currentBrightnessLevel = 4;
//bool isScreenOff = false;
unsigned long lastOkPressTime = 0;
bool okPressedLong = false;
bool okPressedState = false;



int alarmHour = 0;          // 闹钟小时
int alarmMinute = 0;        // 闹钟分钟
int alarmSecond = 0;        // 闹钟秒
bool alarmEnabled = false;  // 闹钟是否开启
bool alarmTriggered = false; // 闹钟是否已触发
unsigned long alarmLastCheck = 0; // 上次检查闹钟时间
unsigned long alarmStartTime = 0;  // 闹钟开始响的时间
bool alarmRinging = false; 
int alarmSelectIndex = 0;  


bool isScreenOff = false;  


const char* settingsItems[] = {
  "Set Time",
  "Network Time",
  "Brightness",
  "Alarm",
  "Game Set",    
  "nothing here",
  "nothing here"
};
const int settingsItemCount = 7;  



// ========== 时间
int setHour = 0;
int setMinute = 0;
int setSecond = 0;
int timeSelectIndex = 0;
String currentTime = "00:00:00";
unsigned long lastTimeUpdate = 0;


// ========== 恐龙游戏
#define GROUND_Y 120
#define DINO_Y 87
//#define SCROLL_SPEED 4
#define JUMP_FORCE -10
#define GRAVITY 1

#include "dinosaur.h"
#include "obstacle.h"
#include "cloud.h"
static int lastDinoX = 30, lastDinoY = DINO_Y;
static int lastCactusX = SCREEN_WIDTH;
static int lastCloud1X = 60, lastCloud2X = 100;
static int lastObstacleIndex = 0;
int dinoScore = 0;
int dinoHighScore = 0;
bool dinoGameOver = false;
bool dinoJumping = false;
int dinoJumpVelocity = 0;
int nextObstacleDistance = 0;  // 下一个障碍物的生成距离
const int minDistance = 40;    // 最小间隔距离
const int maxDistance = 100;   // 最大间隔距离

struct GameObject {
  int x, y;
  int w, h;
  const uint16_t* img;
  
  GameObject(int x0, int y0, int w0, int h0, const uint16_t* img0) {
    x = x0; y = y0; w = w0; h = h0; img = img0;
  }
};

GameObject dino(30, DINO_Y, 30, 32, dinosaur_1_img);
GameObject cactus_small(SCREEN_WIDTH, GROUND_Y  - 19, 10, 18, cactus_1_img);
GameObject cactus_big(SCREEN_WIDTH, GROUND_Y - 31, 17, 31, cactus_2_img);
GameObject dinoCloud1(60, 20, 30, 8, cloud_img);
GameObject dinoCloud2(100, 30, 30, 8, cloud_img);

GameObject* dinoObstacles[] = {&cactus_small, &cactus_big};
int dinoObstacleCount = 2;
int dinoCurrentObstacle = 0;

// ========== 函数
void drawCharWithBg(int x, int y, char c, uint16_t color);
void drawStringWithBg(int x, int y, const char* str, uint16_t color);
//void showImageFast(const uint8_t* img);
void updateTimeString();
void drawTempHumOnSplash(bool forceRefresh = false);
void drawTimeOnSplash(bool forceRefresh = false);
void drawTimeOnScreen(int timeX, int timeY);
void drawLargeTimeOnSplash(bool forceRefresh = false);
void drawSplashScreen();
void drawMainMenu();
void drawGameSelect();
void drawSettings();
void drawTimeSetting();
void drawBrightnessSetting();
void drawAlarmSetting();
void drawSettingDetail();
void drawAboutScreen();
void drawDinoObject(GameObject &obj);
void drawDinoScore();
void initDinoGame();
void updateDinoGame();
void readDHT11();
void setBrightness(int level);
//void turnScreenOn();
//void turnScreenOff();
//void handleTimeSetting();
//void handleBrightnessSetting();
//void handleAlarmSetting();
//void handleGameSelect();
//void handleSettings();
//void handleSettingDetail();
//void handleDinoGame();
//void handleAbout();
//void handleSplash();
//void handleRightKey();
void initBuzzer();
void checkAlarm();
void stopAlarm();
void checkButtons();
//void showImage30x30Fast();
void initTetris();
//void drawTetris();
void handleTetrisInput();
void rotateShape();
bool checkCollision(int shape[4][4], int px, int py);
void placeShape();
void spawnShape();


void drawCharWithBg(int x, int y, char c, uint16_t color) {
  int charWidth = 8;
  int charHeight = 8;
  
  for(int dy = 0; dy < charHeight; dy++) {
    for(int dx = 0; dx < charWidth; dx++) {
      int imgX = x + dx;
      int imgY = y + dy;
      
      if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
        int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
        uint8_t high = gImage_img1[pixelIdx];
        uint8_t low = gImage_img1[pixelIdx + 1];
        uint16_t bgColor = (high << 8) | low;
        tft.drawPixel(imgX, imgY, bgColor);
      }
    }
  }
  
  tft.drawChar(c, x, y, color);
}


void drawStringWithBg(int x, int y, const char* str, uint16_t color) {
  int len = strlen(str);
  for(int i = 0; i < len; i++) {
    drawCharWithBg(x + i * 8, y, str[i], color);
  }
}


void showImageFast(const uint8_t* img, int x = 0, int y = 0, int width = SCREEN_WIDTH, int height = SCREEN_HEIGHT) {
  if (!img) return;

  int startCol = max(0, -x);
  int endCol = min(width, SCREEN_WIDTH - x);
  int startRow = max(0, -y);
  int endRow = min(height, SCREEN_HEIGHT - y);
  
  if (startCol >= endCol || startRow >= endRow) return;
  
  int visibleWidth = endCol - startCol;
  uint16_t lineBuffer[visibleWidth];
  
  int idx = 8 + (startRow * width + startCol) * 2;
  
  for (int row = startRow; row < endRow; row++) {
    // 读取一行像素到缓冲区
    for (int col = 0; col < visibleWidth; col++) {
      uint8_t high = pgm_read_byte(&img[idx++]);
      uint8_t low = pgm_read_byte(&img[idx++]);
      lineBuffer[col] = (high << 8) | low;
    }
  
    tft.drawImageMirror(x + startCol, y + row, visibleWidth, 1, lineBuffer);
    if (endCol < width) {
      idx += (width - endCol) * 2;
    }
  }
}
// ========== 时间
void updateTimeString() {
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", setHour, setMinute, setSecond);
  currentTime = String(timeStr);
}

// ========== 开屏界面的温湿度显示
void drawTempHumOnSplash(bool forceRefresh) {
  static String lastTempHumStr = "";
  static bool firstRun = true;
  
  char tempHumBuf[20];
  
  if (dhtSensorError) {
    sprintf(tempHumBuf, "ERR");
  } else {
    sprintf(tempHumBuf, "%.1fC %.0f%%", temperature, humidity);
  }
  
  String currentTempHumStr = String(tempHumBuf);
  int strLen = currentTempHumStr.length();
  
  int tempHumX = 0;
  int tempHumY = 2;
  
  if (forceRefresh) {
    firstRun = true;
    lastTempHumStr = "";
  }
  
  if (firstRun || forceRefresh) {
    for(int y = 0; y < 8; y++) {
      for(int x = 0; x < 80; x++) {
        int imgX = tempHumX + x;
        int imgY = tempHumY + y;
        
        if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
          int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
          uint8_t high = gImage_img1[pixelIdx];
          uint8_t low = gImage_img1[pixelIdx + 1];
          uint16_t color = (high << 8) | low;
          tft.drawPixel(imgX, imgY, color);
        }
      }
    }
    
    uint16_t textColor = dhtSensorError ? RED : CYAN;
    for(int i = 0; i < strLen; i++) {
      tft.drawChar(currentTempHumStr[i], tempHumX + i * 8, tempHumY, textColor);
    }
    
    lastTempHumStr = currentTempHumStr;
    firstRun = false;
    return;
  }
  
  if (lastTempHumStr != currentTempHumStr) {
    lastTempHumStr = currentTempHumStr;
    
    for(int y = 0; y < 8; y++) {
      for(int x = 0; x < 80; x++) {
        int imgX = tempHumX + x;
        int imgY = tempHumY + y;
        
        if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
          int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
          uint8_t high = gImage_img1[pixelIdx];
          uint8_t low = gImage_img1[pixelIdx + 1];
          uint16_t color = (high << 8) | low;
          tft.drawPixel(imgX, imgY, color);
        }
      }
    }
    
    uint16_t textColor = dhtSensorError ? RED : CYAN;
    for(int i = 0; i < strLen; i++) {
      tft.drawChar(currentTempHumStr[i], tempHumX + i * 8, tempHumY, textColor);
    }
  }
}

// ========== 开屏界面的时间显示2
void drawTimeOnSplash(bool forceRefresh) {
  static String lastTimeStr = "";
  static bool firstRun = true;
  
  char timeBuf[9];
  sprintf(timeBuf, "%02d:%02d:%02d", setHour, setMinute, setSecond);
  String currentTimeStr = String(timeBuf);
  
  int timeX = 62;
  int timeY = 2;
  
  if (forceRefresh) {
    firstRun = true;
    lastTimeStr = "";
  }
  
  if (firstRun || forceRefresh) {
    for(int y = 0; y < 8; y++) {
      for(int x = 0; x < 64; x++) {
        int imgX = timeX + x;
        int imgY = timeY + y;
        
        if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
          int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
          uint8_t high = gImage_img1[pixelIdx];
          uint8_t low = gImage_img1[pixelIdx + 1];
          uint16_t color = (high << 8) | low;
          tft.drawPixel(imgX, imgY, color);
        }
      }
    }
    
    for(int i = 0; i < 8; i++) {
      tft.drawChar(currentTimeStr[i], timeX + i * 8, timeY, 0x3333);
    }
    
    lastTimeStr = currentTimeStr;
    firstRun = false;
    return;
  }
  
  if (lastTimeStr != currentTimeStr) {
    for (int i = 0; i < 8; i++) {
      if (lastTimeStr[i] != currentTimeStr[i]) {
        int charX = timeX + i * 8;
        
        for(int dy = 0; dy < 8; dy++) {
          for(int dx = 0; dx < 8; dx++) {
            int imgX = charX + dx;
            int imgY = timeY + dy;
            
            if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
              int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
              uint8_t high = gImage_img1[pixelIdx];
              uint8_t low = gImage_img1[pixelIdx + 1];
              uint16_t color = (high << 8) | low;
              tft.drawPixel(imgX, imgY, color);
            }
          }
        }
        
        tft.drawChar(currentTimeStr[i], charX, timeY, 0x3333);
      }
    }
    
    lastTimeStr = currentTimeStr;
  }
}

// ========== 时间显示函数

void drawTimeOnScreen(int timeX, int timeY) {
  if (currentState == STATE_SETTING_DETAIL) {
    if (settingsSelection == 5 || settingsSelection == 6 ) {
      return;
    }
  }
  if (currentState == STATE_MINESWEEPER) {
      return;
  }
  static String lastTimeStr = "";
  
  char timeBuf[9];
  sprintf(timeBuf, "%02d:%02d:%02d", setHour, setMinute, setSecond);
  String currentTimeStr = String(timeBuf);
  
  if (lastTimeStr != currentTimeStr) {
    lastTimeStr = currentTimeStr;
    
    if (currentState == STATE_MAIN_MENU || currentState == STATE_GAME_SELECT || 
        currentState == STATE_SETTINGS) {
      for(int i = 0; i < 8; i++) {
        int charX = timeX + i * 8;
        for(int dy = 0; dy < 8; dy++) {
          for(int dx = 0; dx < 8; dx++) {
            int imgX = charX + dx;
            int imgY = timeY + dy;
            
            if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
              int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
              uint8_t high = gImage_img1[pixelIdx];
              uint8_t low = gImage_img1[pixelIdx + 1];
              uint16_t bgColor = (high << 8) | low;
              tft.drawPixel(imgX, imgY, bgColor);
            }
          }
        }
        tft.drawChar(currentTimeStr[i], charX, timeY, YELLOW);
      }
    } else {
      tft.fillRect(timeX, timeY, 64, 8, BLACK);
      for(int i = 0; i < 8; i++) {
        tft.drawChar(currentTimeStr[i], timeX + i * 8, timeY, YELLOW);
      }
    }
  }
}

void drawLargeTimeOnSplash(bool forceRefresh) {
  static String lastTimeStr = "";
  static bool firstRun = true;
  
  char timeBuf[9];
  sprintf(timeBuf, "%02d:%02d:%02d", setHour, setMinute, setSecond);
  String currentTimeStr = String(timeBuf);
  
  int digitWidth = 16;
  int digitHeight = 24;
  
  int colonWidth = 8;
  int colonOffset = -1;
  int horizontalShift = 2;
  
  int hourWidth = 2 * digitWidth;
  int minuteWidth = 2 * digitWidth;
  int secondWidth = 2 * digitWidth;
  int totalWidth = hourWidth + colonWidth + minuteWidth + colonWidth + secondWidth;
  
  int digitPositions[6];
  int currentX = (SCREEN_WIDTH - totalWidth) / 2 + horizontalShift;
  
  digitPositions[0] = currentX;
  currentX += digitWidth;
  digitPositions[1] = currentX;
  currentX += digitWidth;
  int colon1X = currentX;
  currentX += colonWidth;
  digitPositions[2] = currentX;
  currentX += digitWidth;
  digitPositions[3] = currentX;
  currentX += digitWidth;
  int colon2X = currentX;
  currentX += colonWidth;
  digitPositions[4] = currentX;
  currentX += digitWidth;
  digitPositions[5] = currentX;
  
  int timeY = 62;
  uint16_t textColor = 0x3333;
  
  if (firstRun || forceRefresh) {
    for(int y = timeY - 1; y < timeY + digitHeight + 1; y++) {
      for(int x = digitPositions[0] - 1; x < digitPositions[5] + digitWidth + 1; x++) {
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
          int pixelIdx = 8 + (y * SCREEN_WIDTH + x) * 2;
          uint8_t high = gImage_img1[pixelIdx];
          uint8_t low = gImage_img1[pixelIdx + 1];
          uint16_t bgColor = (high << 8) | low;
          tft.drawPixel(x, y, bgColor);
        }
      }
    }
    
    for(int i = 0; i < 6; i++) {
      tft.drawLargeDigit(currentTimeStr[i < 2 ? i : (i < 4 ? i+1 : i+2)], 
                        digitPositions[i], timeY, textColor);
    }
    
    tft.drawChar(':', colon1X + colonOffset, timeY + 8, textColor);
    tft.drawChar(':', colon2X + colonOffset, timeY + 8, textColor);
    
    firstRun = false;
    lastTimeStr = currentTimeStr;
    return;
  }
  
  if (lastTimeStr != currentTimeStr) {
    if (lastTimeStr[0] != currentTimeStr[0]) {
      for(int dy = 0; dy < digitHeight; dy++) {
        for(int dx = 0; dx < digitWidth; dx++) {
          int imgX = digitPositions[0] + dx;
          int imgY = timeY + dy;
          if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
            uint8_t high = gImage_img1[pixelIdx];
            uint8_t low = gImage_img1[pixelIdx + 1];
            uint16_t bgColor = (high << 8) | low;
            tft.drawPixel(imgX, imgY, bgColor);
          }
        }
      }
      tft.drawLargeDigit(currentTimeStr[0], digitPositions[0], timeY, textColor);
    }
    
    if (lastTimeStr[1] != currentTimeStr[1]) {
      for(int dy = 0; dy < digitHeight; dy++) {
        for(int dx = 0; dx < digitWidth; dx++) {
          int imgX = digitPositions[1] + dx;
          int imgY = timeY + dy;
          if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
            uint8_t high = gImage_img1[pixelIdx];
            uint8_t low = gImage_img1[pixelIdx + 1];
            uint16_t bgColor = (high << 8) | low;
            tft.drawPixel(imgX, imgY, bgColor);
          }
        }
      }
      tft.drawLargeDigit(currentTimeStr[1], digitPositions[1], timeY, textColor);
    }
    
    if (lastTimeStr[3] != currentTimeStr[3]) {
      for(int dy = 0; dy < digitHeight; dy++) {
        for(int dx = 0; dx < digitWidth; dx++) {
          int imgX = digitPositions[2] + dx;
          int imgY = timeY + dy;
          if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
            uint8_t high = gImage_img1[pixelIdx];
            uint8_t low = gImage_img1[pixelIdx + 1];
            uint16_t bgColor = (high << 8) | low;
            tft.drawPixel(imgX, imgY, bgColor);
          }
        }
      }
      tft.drawLargeDigit(currentTimeStr[3], digitPositions[2], timeY, textColor);
    }
    
    if (lastTimeStr[4] != currentTimeStr[4]) {
      for(int dy = 0; dy < digitHeight; dy++) {
        for(int dx = 0; dx < digitWidth; dx++) {
          int imgX = digitPositions[3] + dx;
          int imgY = timeY + dy;
          if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
            uint8_t high = gImage_img1[pixelIdx];
            uint8_t low = gImage_img1[pixelIdx + 1];
            uint16_t bgColor = (high << 8) | low;
            tft.drawPixel(imgX, imgY, bgColor);
          }
        }
      }
      tft.drawLargeDigit(currentTimeStr[4], digitPositions[3], timeY, textColor);
    }
    
    if (lastTimeStr[6] != currentTimeStr[6]) {
      for(int dy = 0; dy < digitHeight; dy++) {
        for(int dx = 0; dx < digitWidth; dx++) {
          int imgX = digitPositions[4] + dx;
          int imgY = timeY + dy;
          if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
            uint8_t high = gImage_img1[pixelIdx];
            uint8_t low = gImage_img1[pixelIdx + 1];
            uint16_t bgColor = (high << 8) | low;
            tft.drawPixel(imgX, imgY, bgColor);
          }
        }
      }
      tft.drawLargeDigit(currentTimeStr[6], digitPositions[4], timeY, textColor);
    }
    
    if (lastTimeStr[7] != currentTimeStr[7]) {
      for(int dy = 0; dy < digitHeight; dy++) {
        for(int dx = 0; dx < digitWidth; dx++) {
          int imgX = digitPositions[5] + dx;
          int imgY = timeY + dy;
          if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
            uint8_t high = gImage_img1[pixelIdx];
            uint8_t low = gImage_img1[pixelIdx + 1];
            uint16_t bgColor = (high << 8) | low;
            tft.drawPixel(imgX, imgY, bgColor);
          }
        }
      }
      tft.drawLargeDigit(currentTimeStr[7], digitPositions[5], timeY, textColor);
    }
    
    lastTimeStr = currentTimeStr;
  }
}

// ========== 开屏界面
void drawSplashScreen() {
  if (lastState != currentState) {
    showImageFast(gImage_img1, 0, 0, 128, 128); 
    //showImageFast(gImage_img1);
    lastState = currentState;
    
    drawTempHumOnSplash(true);
    drawLargeTimeOnSplash(true);
  } else {
    drawTempHumOnSplash();
    drawLargeTimeOnSplash();
  }
}

// ========== 主菜单
void drawMainMenu() {
  static int lastSelection = -1;
  static bool firstDraw = true;
  
  if (lastState != currentState || firstDraw) {
    //showImageFast(gImage_img1);
    showImageFast(gImage_img1, 0, 0, 128, 128); 
    for(int i = 0; i < mainMenuItemCount; i++) {
      drawStringWithBg(30, 35 + i * 20, mainMenuItems[i], WHITE);
    }
    
    drawCharWithBg(15, 35 + menuSelection * 20, '>', YELLOW);
    drawStringWithBg(30, 35 + menuSelection * 20, mainMenuItems[menuSelection], YELLOW);
    
    lastState = currentState;
    lastSelection = menuSelection;
    firstDraw = false;
    
    drawTimeOnScreen(62 ,2);
    return;
  }
  
  if (lastSelection != menuSelection) {
    int oldY = 35 + lastSelection * 20;
    int newY = 35 + menuSelection * 20;
    
    int clearX = 10;
    int clearW = 100;
    int clearH = 16;
    
    for(int y = 0; y < clearH; y++) {
      for(int x = 0; x < clearW; x++) {
        int imgX = clearX + x;
        int imgY = oldY + y;
        
        if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
          int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
          uint8_t high = gImage_img1[pixelIdx];
          uint8_t low = gImage_img1[pixelIdx + 1];
          uint16_t color = (high << 8) | low;
          tft.drawPixel(imgX, imgY, color);
        }
      }
    }
    
    drawStringWithBg(30, oldY, mainMenuItems[lastSelection], WHITE);
    drawCharWithBg(15, newY, '>', YELLOW);
    drawStringWithBg(30, newY, mainMenuItems[menuSelection], YELLOW);
    
    lastSelection = menuSelection;
  }
}

// ========== 游戏选择界面
void drawGameSelect() {
  static int lastSelection = -1;
  static bool firstDraw = true;
  const int imgWidth = 30;
  const int imgHeight = 30;
  const int cols = 2;
  const int startX = (SCREEN_WIDTH - (cols * imgWidth + (cols - 1) * 10)) / 2; 
  const int startY = 20;
  const int spacingX = 10; 
  const int spacingY = 5;    
  
  if (lastState != currentState || firstDraw) {
   
    showImageFast(gImage_img1, 0, 0, 128, 128); 
    
    for (int i = 0; i < gameItemCount; i++) {
      int row = i / cols;
      int col = i % cols;
      int imgX = startX + col * (imgWidth + spacingX);
      int imgY = startY + row * (imgHeight + spacingY);
      
  
      extern const unsigned char gImage_img_1[1808];
      extern const unsigned char gImage_img_2[1808];
      extern const unsigned char gImage_img_3[1808];
      extern const unsigned char gImage_img_4[1808];
      extern const unsigned char gImage_img_5[1808];
      extern const unsigned char gImage_img_6[1808];
      
      const unsigned char* gameImages[] = {
        gImage_img_1, gImage_img_2, gImage_img_3, gImage_img_4, gImage_img_5, gImage_img_6
      };
      
     
      tft.showImage30x30Fast(gameImages[i], imgX, imgY);
    }
    
    // 绘制选中框
    int selectedRow = gameSelection / cols;
    int selectedCol = gameSelection % cols;
    int selectedX = startX + selectedCol * (imgWidth + spacingX) - 2;
    int selectedY = startY + selectedRow * (imgHeight + spacingY) - 2;
    tft.drawRect(selectedX, selectedY, imgWidth + 4, imgHeight + 4, YELLOW);
    
    lastState = currentState;
    lastSelection = gameSelection;
    firstDraw = false;
  } 
  else if (lastSelection != gameSelection) {
    // 只清除旧的选中框（用背景图片恢复）
    int oldRow = lastSelection / cols;
    int oldCol = lastSelection % cols;
    int oldX = startX + oldCol * (imgWidth + spacingX) - 2;
    int oldY = startY + oldRow * (imgHeight + spacingY) - 2;

    for (int y = oldY; y < oldY + imgHeight + 4; y++) {
      for (int x = oldX; x < oldX + imgWidth + 4; x++) {

        bool isBorder = (y == oldY || y == oldY + imgHeight + 3 || 
                         x == oldX || x == oldX + imgWidth + 3);
        if (isBorder && x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
          int pixelIdx = 8 + (y * SCREEN_WIDTH + x) * 2;
          uint8_t high = gImage_img1[pixelIdx];
          uint8_t low = gImage_img1[pixelIdx + 1];
          uint16_t bgColor = (high << 8) | low;
          tft.drawPixel(x, y, bgColor);
        }
      }
    }
    

    int newRow = gameSelection / cols;
    int newCol = gameSelection % cols;
    int newX = startX + newCol * (imgWidth + spacingX) - 2;
    int newY = startY + newRow * (imgHeight + spacingY) - 2;
    tft.drawRect(newX, newY, imgWidth + 4, imgHeight + 4, YELLOW);
    
    lastSelection = gameSelection;
  }
  
  drawTimeOnScreen(62 ,2);
}
// ========== 设置
void drawSettings() {
  static int lastSelection = -1;
  static bool firstDraw = true;
  
  if (lastState != currentState || firstDraw) {
    //showImageFast(gImage_img1);
    showImageFast(gImage_img1, 0, 0, 128, 128); 
    //drawStringWithBg(30, 10, "SETTINGS", CYAN);
    
    for(int i = 0; i < settingsItemCount; i++) {
      if (i == settingsSelection) {
        drawCharWithBg(10, 20 + i * 15, '>', YELLOW);
        drawStringWithBg(20, 20 + i * 15, settingsItems[i], YELLOW);
      } else {
        drawStringWithBg(20, 20 + i * 15, settingsItems[i], WHITE);
      }
    }
    
    lastState = currentState;
    lastSelection = settingsSelection;
    firstDraw = false;
  } 
  else if (lastSelection != settingsSelection) {
    int oldY = 20 + lastSelection * 15;
    int newY = 20 + settingsSelection * 15;
    
    int clearX = 10;
    int clearW = 100;
    int clearH = 12;
    
    for(int y = 0; y < clearH; y++) {
      for(int x = 0; x < clearW; x++) {
        int imgX = clearX + x;
        int imgY = oldY + y;
        
        if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
          int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
          uint8_t high = gImage_img1[pixelIdx];
          uint8_t low = gImage_img1[pixelIdx + 1];
          uint16_t color = (high << 8) | low;
          tft.drawPixel(imgX, imgY, color);
        }
      }
    }
    
    drawStringWithBg(20, oldY, settingsItems[lastSelection], WHITE);
    drawCharWithBg(10, newY, '>', YELLOW);
    drawStringWithBg(20, newY, settingsItems[settingsSelection], YELLOW);
    
    lastSelection = settingsSelection;
  }
  
  drawTimeOnScreen(62 ,2);
}

void drawTimeSetting() {
  tft.fillScreen(BLACK);
  tft.drawString("SET TIME", 30, 10, CYAN);
  
  char timeBuf[9];
  sprintf(timeBuf, "%02d:%02d:%02d", setHour, setMinute, setSecond);
  
  int timeX = 20;
  int timeY = 50;
  
  if (timeSelectIndex == 0) {
    tft.drawString(timeBuf, timeX, timeY, WHITE);
    tft.fillRect(timeX, timeY, 16, 8, BLACK);
    char hourBuf[3];
    sprintf(hourBuf, "%02d", setHour);
    tft.drawString(hourBuf, timeX, timeY, YELLOW);
  } 
  else if (timeSelectIndex == 1) {
    tft.drawString(timeBuf, timeX, timeY, WHITE);
    tft.fillRect(timeX + 24, timeY, 16, 8, BLACK);
    char minBuf[3];
    sprintf(minBuf, "%02d", setMinute);
    tft.drawString(minBuf, timeX + 24, timeY, YELLOW);
  } 
  else {
    tft.drawString(timeBuf, timeX, timeY, WHITE);
    tft.fillRect(timeX + 48, timeY, 16, 8, BLACK);
    char secBuf[3];
    sprintf(secBuf, "%02d", setSecond);
    tft.drawString(secBuf, timeX + 48, timeY, YELLOW);
  }
  
  drawTimeOnScreen(62, 2);
  

  lastPress = millis() - 250;
}

void drawBrightnessSetting() {
  tft.fillScreen(BLACK);
  tft.drawString("BRIGHTNESS", 20, 10, CYAN);
  
  String levelStr = "Level: " + String(currentBrightnessLevel + 1) + "/5";
  tft.drawString(levelStr.c_str(), 20, 40, WHITE);
  
  int barWidth = map(brightnessValues[currentBrightnessLevel], 8, 255, 10, 100);
  tft.drawRect(14, 60, 100, 10, WHITE);
  tft.fillRect(15, 61, barWidth-2, 8, BLUE);
  
  drawTimeOnScreen(62, 2);

  lastPress = millis() - 250; 
}

// ========== 闹钟设置
void drawAlarmSetting() {
  tft.fillScreen(BLACK);
  //tft.drawString("ALARM SET", 30, 10, CYAN);
  

  char timeBuf[9];
  sprintf(timeBuf, "%02d:%02d:%02d", alarmHour, alarmMinute, alarmSecond);
  

  int timeX = 20;
  int timeY = 40;
  
  if (alarmSelectIndex == 0) {
    // 小时
    tft.drawString(timeBuf, timeX, timeY, WHITE);
    tft.fillRect(timeX, timeY, 16, 8, BLACK);
    char hourBuf[3];
    sprintf(hourBuf, "%02d", alarmHour);
    tft.drawString(hourBuf, timeX, timeY, YELLOW);
  } 
  else if (alarmSelectIndex == 1) {
    // 分钟
    tft.drawString(timeBuf, timeX, timeY, WHITE);
    tft.fillRect(timeX + 24, timeY, 16, 8, BLACK);
    char minBuf[3];
    sprintf(minBuf, "%02d", alarmMinute);
    tft.drawString(minBuf, timeX + 24, timeY, YELLOW);
  } 
  else if (alarmSelectIndex == 2) {
    // 秒
    tft.drawString(timeBuf, timeX, timeY, WHITE);
    tft.fillRect(timeX + 48, timeY, 16, 8, BLACK);
    char secBuf[3];
    sprintf(secBuf, "%02d", alarmSecond);
    tft.drawString(secBuf, timeX + 48, timeY, YELLOW);
  } 
  else {
    // 没有选中时间部分，正常显示
    tft.drawString(timeBuf, timeX, timeY, WHITE);
  }
  
  // 显示闹钟开关状态
  tft.drawString("Status:", 20, 65, WHITE);
  if (alarmEnabled) {
    tft.drawString("ON", 80, 65, GREEN);
  } else {
    tft.drawString("OFF", 80, 65, RED);
  }
  if (alarmSelectIndex == 3) {
    tft.drawRect(78, 62, 30, 13, YELLOW);
  }
  
 // tft.drawString("LEFT/RIGHT: select", 10, 90, GREEN);
 // tft.drawString("UP/DOWN: adjust", 15, 105, GREEN);
 // tft.drawString("OK: save & return", 10, 120, YELLOW);
}


void drawSettingDetail() {
  if (lastState != currentState) {
    tft.fillScreen(BLACK);
    lastState = currentState;
  }

  switch(settingsSelection) {
    case 0: tft.drawString("SET TIME", 30, 10, CYAN); break;
    case 1: tft.drawString("NETWORK TIME", 20, 10, CYAN); break;
    case 2: tft.drawString("BRIGHTNESS", 25, 10, CYAN); break;
    case 3: tft.drawString("ALARM", 35, 10, CYAN); break;
    case 4: tft.drawString("GAME SET", 30, 10, CYAN); break;
    case 5: tft.drawString("INFO", 40, 10, CYAN); break;
    case 6: tft.drawString("INFO", 40, 10, CYAN); break;
    default: tft.drawString(settingsItems[settingsSelection], 15, 10, CYAN); break;
  }

  switch(settingsSelection) {
    case 0:
      drawTimeSetting();
      break;
    case 1:
      drawNetworkTimeSetting();
      break;
    case 2:
      drawBrightnessSetting();
      break;
    case 3:
      drawAlarmSetting();
      break;
    case 4:
      drawMineCountSetting();
      break;
    case 5:

      break;
    case 6:
      break;
  }
  
  drawTimeOnScreen(62, 2);
}
// ========== 关于
void drawAboutScreen() {
  if (lastState != currentState) {
    tft.fillScreen(BLACK);
    lastState = currentState;
  }
  
  tft.drawString("ABOUT", 40, 10, CYAN);
  tft.drawString("ESP32-C3", 20, 30, GREEN);
  
  tft.drawString("Heap:", 10, 45, WHITE);
  tft.drawString("Free:", 10, 55, WHITE);
  tft.drawString("Flash:", 10, 65, WHITE);
  tft.drawString("Freq:", 10, 75, WHITE);
  tft.drawString("Cores:", 10, 85, WHITE);
  tft.drawString("Model:", 10, 95, WHITE);
  
  uint32_t heapSize = ESP.getHeapSize();
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t flashSize = ESP.getFlashChipSize();
  String chipModel = ESP.getChipModel();
  uint32_t chipCores = ESP.getChipCores();
  uint32_t chipFreq = ESP.getCpuFreqMHz();
  
  tft.fillRect(60, 45, 40, 8, BLACK);
  tft.drawNumber(heapSize / 1024, 60, 45, YELLOW);
  tft.fillRect(60, 55, 40, 8, BLACK);
  tft.drawNumber(freeHeap / 1024, 60, 55, YELLOW);
  tft.fillRect(60, 65, 40, 8, BLACK);
  tft.drawNumber(flashSize / 1024 / 1024, 60, 65, YELLOW);
  tft.fillRect(60, 75, 40, 8, BLACK);
  tft.drawNumber(chipFreq, 60, 75, YELLOW);
  tft.fillRect(60, 85, 40, 8, BLACK);
  tft.drawNumber(chipCores, 60, 85, YELLOW);
  tft.fillRect(60, 95, 68, 8, BLACK);
  tft.drawString(chipModel.c_str(), 60, 95, CYAN);
  
  drawTimeOnScreen(62 ,2);
}


void drawDinoObject(GameObject &obj) {
    int x = obj.x;
    int y = obj.y;
    int w = obj.w;
    int h = obj.h;

    if (x >= 0 && y >= 0 && 
        x + w <= SCREEN_WIDTH && 
        y + h <= SCREEN_HEIGHT) {
        tft.drawImage(x, y, w, h, obj.img);
    }
}


void clearDinoAreaExact(int x, int y, int w, int h) {
    // 裁剪到屏幕范围内
    if (x + w <= 0 || x >= SCREEN_WIDTH || y + h <= 0 || y >= SCREEN_HEIGHT) return;
    
    int clearX = max(x, 0);
    int clearY = max(y, 0);
    int clearW = min(w, SCREEN_WIDTH - clearX);
    int clearH = min(h, SCREEN_HEIGHT - clearY);
    
    if (clearW > 0 && clearH > 0) {
        tft.fillRect(clearX, clearY, clearW, clearH, BLACK);
    }
}


void drawDinoScore() {
 
    static int lastScore = -1;
    if (dinoScore != lastScore) {
 
        tft.fillRect(15, 0, 35, 8, BLACK);
        tft.drawNumber(dinoScore, 15, 0, YELLOW);
        lastScore = dinoScore;
    }
}
/*void clearDinoArea(int x, int y, int w, int h) {
  int expand = 2;
  int clearX = x - expand;
  int clearY = y - expand - 1;
  int clearW = w + expand * 2;
  int clearH = h + expand * 2;

  if (clearX >= SCREEN_WIDTH || clearX + clearW <= 0) return;
  if (clearY >= SCREEN_HEIGHT || clearY + clearH <= 0) return;

  clearX = max(clearX, 0);
  clearY = max(clearY, 0);
  clearW = min(clearW, SCREEN_WIDTH - clearX);
  clearH = min(clearH, SCREEN_HEIGHT - clearY);

  tft.fillRect(clearX, clearY, clearW, clearH + 3, BLACK);
}
*/


void initDinoGame() {
    dino.x = 30; dino.y = DINO_Y;
    cactus_small.x = SCREEN_WIDTH; cactus_big.x = SCREEN_WIDTH;
    dinoCloud1.x = 60; dinoCloud2.x = 100;
    
    dinoCurrentObstacle = random(0, dinoObstacleCount);
    dinoObstacles[dinoCurrentObstacle]->x = SCREEN_WIDTH;

    nextObstacleDistance = random(minDistance, maxDistance);
    
    dinoScore = 0;
    dinoGameOver = false;
    dinoJumping = false;
    dinoJumpVelocity = 0;
    
    
    // 保存初始位置
    lastDinoX = dino.x;
    lastDinoY = dino.y;
    lastCactusX = SCREEN_WIDTH;
    lastCloud1X = 60;
    lastCloud2X = 100;
    lastObstacleIndex = dinoCurrentObstacle;
    
    tft.fillScreen(BLACK);
    tft.fillRect(0, GROUND_Y, SCREEN_WIDTH, 2, WHITE);
    
    // 绘制初始物体
    drawDinoObject(dinoCloud1);
    drawDinoObject(dinoCloud2);
    drawDinoObject(*dinoObstacles[dinoCurrentObstacle]);
    drawDinoObject(dino);
    
    // 绘制分数
    tft.fillRect(0, 0, 50, 10, BLACK);
    tft.drawString("S:", 0, 0, WHITE);
    tft.drawNumber(0, 15, 0, YELLOW);
}

void updateDinoGame() {
    if (dinoGameOver) return;

    // 保存移动前的位置
    int oldCactusX = dinoObstacles[dinoCurrentObstacle]->x;
    int oldCloud1X = dinoCloud1.x;
    int oldCloud2X = dinoCloud2.x;
    int oldDinoX = dino.x;
    int oldDinoY = dino.y;
    int oldObstacleIndex = dinoCurrentObstacle;

    // 移动物体
    int scrollSpeed = (dinoScore < 10) ? 4 : 5;
    dinoObstacles[dinoCurrentObstacle]->x -= scrollSpeed;
    dinoCloud1.x -= 2;
    dinoCloud2.x -= 2;

       
    if (dinoObstacles[dinoCurrentObstacle]->x <= -dinoObstacles[dinoCurrentObstacle]->w) {
        clearDinoAreaExact(oldCactusX, dinoObstacles[dinoCurrentObstacle]->y,
                           dinoObstacles[dinoCurrentObstacle]->w, dinoObstacles[dinoCurrentObstacle]->h);

        dinoCurrentObstacle = random(0, dinoObstacleCount);
        
        // 随分数增加，最小间隔逐渐减小
        int dynamicMin = max(20, minDistance - dinoScore * 2);
        int dynamicMax = max(50, maxDistance - dinoScore * 3);
        nextObstacleDistance = random(dynamicMin, dynamicMax);
        
        dinoObstacles[dinoCurrentObstacle]->x = SCREEN_WIDTH + nextObstacleDistance;
        
        dinoScore += 1;
        static int lastScore = -1;
    if (dinoScore != lastScore) {
 
        tft.fillRect(15, 0, 35, 8, BLACK);
        tft.drawNumber(dinoScore, 15, 0, YELLOW);
        lastScore = dinoScore;
    }

        drawDinoObject(*dinoObstacles[dinoCurrentObstacle]);
    }

    // 云朵循环
    if (dinoCloud1.x <= -30) {
        clearDinoAreaExact(oldCloud1X, dinoCloud1.y, dinoCloud1.w, dinoCloud1.h);
        dinoCloud1.x = SCREEN_WIDTH;
        dinoCloud1.y = random(10, 30);
        drawDinoObject(dinoCloud1);
    }

    if (dinoCloud2.x <= -30) {
        clearDinoAreaExact(oldCloud2X, dinoCloud2.y, dinoCloud2.w, dinoCloud2.h);
        dinoCloud2.x = SCREEN_WIDTH;
        dinoCloud2.y = random(10, 30);
        drawDinoObject(dinoCloud2);
    }

    // 跳跃物理
    if (dinoJumping) {
        dino.y += dinoJumpVelocity;
        dinoJumpVelocity += GRAVITY;
        if (dino.y >= DINO_Y) {
            dino.y = DINO_Y;
            dinoJumping = false;
            dinoJumpVelocity = 0;
        }
    }

    // 恐龙动画
    static int anim = 0;
    anim++;
    if (anim % 8 == 0) {
        dino.img = (dino.img == dinosaur_1_img) ? dinosaur_2_img : dinosaur_1_img;
    }

    // 碰撞检测
    if (dino.x < dinoObstacles[dinoCurrentObstacle]->x + dinoObstacles[dinoCurrentObstacle]->w &&
        dino.x + dino.w > dinoObstacles[dinoCurrentObstacle]->x &&
        dino.y < dinoObstacles[dinoCurrentObstacle]->y + dinoObstacles[dinoCurrentObstacle]->h &&
        dino.y + dino.h > dinoObstacles[dinoCurrentObstacle]->y) {
        dinoGameOver = true;
        if (dinoScore > dinoHighScore) dinoHighScore = dinoScore;

        tft.fillScreen(BLACK);
        tft.drawString("GAME OVER", 20, 40, RED);
        tft.drawString("SCORE", 25, 60, WHITE);
        tft.drawNumber(dinoScore, 70, 60, YELLOW);
        tft.drawString("BEST", 25, 75, WHITE);
        tft.drawNumber(dinoHighScore, 70, 75, CYAN);
        tft.drawString("PRESS OK", 25, 100, GREEN);
        tft.drawString("TO EXIT", 30, 110, GREEN);
        return;
    }

    
    
    // 1. 处理云朵
    if (oldCloud1X != dinoCloud1.x) {
        clearDinoAreaExact(oldCloud1X, dinoCloud1.y, dinoCloud1.w, dinoCloud1.h);
        drawDinoObject(dinoCloud1);
    }
    
    if (oldCloud2X != dinoCloud2.x) {
        clearDinoAreaExact(oldCloud2X, dinoCloud2.y, dinoCloud2.w, dinoCloud2.h);
        drawDinoObject(dinoCloud2);
    }

    // 2. 处理障碍物
    if (oldObstacleIndex != dinoCurrentObstacle) {
    
        clearDinoAreaExact(oldCactusX, dinoObstacles[oldObstacleIndex]->y,
                           dinoObstacles[oldObstacleIndex]->w, dinoObstacles[oldObstacleIndex]->h);
        drawDinoObject(*dinoObstacles[dinoCurrentObstacle]);
    } else if (oldCactusX != dinoObstacles[dinoCurrentObstacle]->x) {
      
        GameObject* obs = dinoObstacles[dinoCurrentObstacle];
        
        // 绘制新位置的障碍物
        drawDinoObject(*obs);
        
        // 擦除旧位置覆盖的部分
        if (oldCactusX > obs->x) {
            int eraseX = obs->x + obs->w;
            int eraseW = oldCactusX + obs->w - eraseX;
            if (eraseW > 0) {
                clearDinoAreaExact(eraseX, obs->y, eraseW, obs->h);
            }
        }
    }

    // 恐龙
    bool dinoMoved = (oldDinoX != dino.x || oldDinoY != dino.y);
    bool dinoImgChanged = (anim % 8 == 0);
    
    if (dinoMoved || dinoImgChanged) {
       
        clearDinoAreaExact(oldDinoX, oldDinoY, dino.w, dino.h);
        
        drawDinoObject(dino);
    }


    tft.fillRect(0, GROUND_Y, SCREEN_WIDTH, 2, WHITE);
}
// ========== 读取DHT11
void readDHT11() {
  if (millis() - lastDHTReadTime >= DHT_READ_INTERVAL) {
    lastDHTReadTime = millis();
    
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();
    
    if (isnan(newTemp) || isnan(newHum)) {
      dhtSensorError = true;
    } else {
      dhtSensorError = false;
      temperature = newTemp;
      humidity = newHum;
    }
  }
}

// ========== 背光控制
void setBrightness(int level) {
  if (level >= 0 && level < BRIGHTNESS_LEVELS) {
    currentBrightnessLevel = level;
    ledcWrite(BL_PIN, brightnessValues[level]); 
  }
}




void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}


void checkAlarm() {
  if (!alarmEnabled) {
    if (alarmRinging) {
      digitalWrite(BUZZER_PIN, LOW);
      alarmRinging = false;
    }
    return;
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - alarmLastCheck >= 500) {
    alarmLastCheck = currentMillis;
    
    int currentTotalSeconds = setHour * 3600 + setMinute * 60 + setSecond;
    int alarmTotalSeconds = alarmHour * 3600 + alarmMinute * 60 + alarmSecond;
    
    // 放宽到10秒内触发
    int timeDiff = abs(currentTotalSeconds - alarmTotalSeconds);
    bool timeMatch = (timeDiff <= 10);
    
    static int lastTriggerTotalSeconds = -1;
    static unsigned long lastRingTime = 0;
    
    if (timeMatch && !alarmRinging) {
      if (lastTriggerTotalSeconds != alarmTotalSeconds || currentMillis - lastRingTime > 60000) {
        lastTriggerTotalSeconds = alarmTotalSeconds;
        lastRingTime = currentMillis;
        
        alarmRinging = true;
        alarmStartTime = currentMillis;
        alarmTriggered = true;
      }
    }
  }
  
  if (alarmRinging) {
    unsigned long ringDuration = millis() - alarmStartTime;
    
    if (ringDuration < 30000) {
      if ((ringDuration / 250) % 2 == 0) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
      }
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      alarmRinging = false;
    }
  }
}

// ========== 停止闹钟
void stopAlarm() {
  if (alarmRinging) {
    alarmRinging = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
}



void checkButtons() {
  unsigned long currentTime = millis();
  if (currentTime - lastPress < 200) return;
  
  bool upRaw = (digitalRead(BTN_UP) == LOW);
  bool downRaw = (digitalRead(BTN_DOWN) == LOW);
  bool leftRaw = (digitalRead(BTN_LEFT) == LOW);
  bool okRaw = (digitalRead(BTN_OK) == LOW);

  bool rightRaw = (downRaw && leftRaw);

  bool up = upRaw;
  bool down = downRaw && !rightRaw;
  bool left = leftRaw && !rightRaw;
  bool right = rightRaw;
  bool ok = okRaw;

  if (alarmRinging && (up || down || left || right || ok)) {
    stopAlarm();
    lastPress = currentTime;
    return;
  }
  
  switch (currentState) {

    case STATE_SPLASH:
      if (ok) {
        currentState = STATE_MAIN_MENU;
        drawMainMenu();
        lastPress = currentTime;
      }
      break;

    case STATE_MAIN_MENU:
      if (up) {
        menuSelection = (menuSelection - 1 + mainMenuItemCount) % mainMenuItemCount;
        drawMainMenu();
        lastPress = currentTime;
      }
      else if (down) {
        menuSelection = (menuSelection + 1) % mainMenuItemCount;
        drawMainMenu();
        lastPress = currentTime;
      }
      else if (left) {
        currentState = STATE_SPLASH;
        drawSplashScreen();
        lastPress = currentTime;
      }
      else if (right) {
        if (menuSelection == 0) {
          currentState = STATE_GAME_SELECT;
          gameSelection = 0;
          drawGameSelect();
        } else if (menuSelection == 1) {
          currentState = STATE_SETTINGS;
          settingsSelection = 0;
          drawSettings();
        } else if (menuSelection == 2) {
          currentState = STATE_ABOUT;
          drawAboutScreen();
        }
        lastPress = currentTime;
      }
      break;

    case STATE_GAME_SELECT:
      if (right) {
        if (gameSelection % 2 == 0 && gameSelection < 5) {
          gameSelection++;
          drawGameSelect();
        }
        lastPress = currentTime;
      }
      else if (left) {
        if (gameSelection % 2 == 1) {
          gameSelection--;
          drawGameSelect();
        } else {
          currentState = STATE_MAIN_MENU;
          menuSelection = 0;
          drawMainMenu();
        }
        lastPress = currentTime;
      }
      else if (up) {
        int newRow = (gameSelection / 2) - 1;
        if (newRow >= 0) {
          gameSelection = newRow * 2 + (gameSelection % 2);
          drawGameSelect();
        }
        lastPress = currentTime;
      }
      else if (down) {
        int newRow = (gameSelection / 2) + 1;
        if (newRow < 3) {
          gameSelection = newRow * 2 + (gameSelection % 2);
          drawGameSelect();
        }
        lastPress = currentTime;
      }
      else if (ok) {
        if (gameSelection == 0) {
          currentState = STATE_CALCULATOR;
          calcExpression = "";
          calcResult = "";
          calcError = false;
          drawCalculator();
        } else if (gameSelection == 1) {
          currentState = STATE_MINESWEEPER;
          initMinesweeper();
        } else if (gameSelection == 2) {
          currentState = STATE_TETRIS;
          tft.fillScreen(BLACK);
          initTetris();
        } else if (gameSelection == 3) {
          currentState = STATE_JUMP_GAME;
          initJumpGame();
        } else if (gameSelection == 5) {
          currentState = STATE_GAME_DINO;
          initDinoGame();
        }
        lastPress = currentTime;
      }
      break;

    case STATE_SETTINGS:
      if (up) {
        settingsSelection = (settingsSelection - 1 + settingsItemCount) % settingsItemCount;
        drawSettings();
        lastPress = currentTime;
      }
      else if (down) {
        settingsSelection = (settingsSelection + 1) % settingsItemCount;
        drawSettings();
        lastPress = currentTime;
      }
      else if (left) {
        currentState = STATE_MAIN_MENU;
        menuSelection = 1;
        drawMainMenu();
        lastPress = currentTime;
      }
      else if (right) {
        currentState = STATE_SETTING_DETAIL;
        drawSettingDetail();
        lastPress = currentTime;
      }
      break;
    
    case STATE_SETTING_DETAIL:
  if (left) {
    currentState = STATE_SETTINGS;
    drawSettings();
    lastPress = currentTime;
  }
  else if (right) {
    if (settingsSelection == 0) {
      currentState = STATE_TIME_SETTING;
      drawTimeSetting();
      lastPress = currentTime - 300;  
    } else if (settingsSelection == 1) {
      currentState = STATE_NETWORK_TIME;
      drawNetworkTimeSetting();
      lastPress = currentTime - 300;
    } else if (settingsSelection == 2) {
      currentState = STATE_BRIGHTNESS_SETTING;
      drawBrightnessSetting();
      lastPress = currentTime - 300;
    } else if (settingsSelection == 3) {
      currentState = STATE_ALARM_SETTING;
      drawAlarmSetting();
      lastPress = currentTime - 300;
    } else if (settingsSelection == 4) {
      currentState = STATE_MINE_COUNT_SETTING;
      tft.fillScreen(BLACK);
      drawMineCountSetting();
      lastPress = currentTime - 300;
    } else if (settingsSelection == 5) {

    } else if (settingsSelection == 6) {

    }
    lastPress = currentTime;
  }
  break;
    
    case STATE_TIME_SETTING:
  if (left) {
    currentState = STATE_SETTING_DETAIL;
    drawSettingDetail();
    lastPress = currentTime;
  }
  else if (right) {
    timeSelectIndex = (timeSelectIndex + 1) % 3;
    drawTimeSetting();
    lastPress = currentTime;
  }
  else if (up) {
    if (timeSelectIndex == 0) setHour = (setHour + 1) % 24;
    else if (timeSelectIndex == 1) setMinute = (setMinute + 1) % 60;
    else setSecond = (setSecond + 1) % 60;
    drawTimeSetting();
    lastPress = currentTime;
  }
  else if (down) {
    if (timeSelectIndex == 0) setHour = (setHour - 1 + 24) % 24;
    else if (timeSelectIndex == 1) setMinute = (setMinute - 1 + 60) % 60;
    else setSecond = (setSecond - 1 + 60) % 60;
    drawTimeSetting();
    lastPress = currentTime;
  }
  else if (ok) {
    updateTimeString();
    currentState = STATE_SETTING_DETAIL;
    drawSettingDetail();
    lastPress = currentTime;
  }
  break;
    
    case STATE_NETWORK_TIME:
  if (wifiSelectMode) {
    if (left) {
      selectedWifiIndex = (selectedWifiIndex - 1 + wifiNetworkCount) % wifiNetworkCount;
      drawNetworkTimeSetting();
      lastPress = currentTime;
    }
    else if (right) {
      selectedWifiIndex = (selectedWifiIndex + 1) % wifiNetworkCount;
      drawNetworkTimeSetting();
      lastPress = currentTime;
    }
    else if (ok) {
      wifiSelectMode = false;
      drawNetworkTimeSetting();
      lastPress = currentTime;
    }
  } else {
    if (left) {
      currentState = STATE_SETTING_DETAIL;
      drawSettingDetail();
      lastPress = currentTime;
    }
    else if (up) {
      initWiFi();
      drawNetworkTimeSetting();
      lastPress = currentTime;
    }
    else if (down) {
      if (wifiConnected) syncNetworkTime();
      else {
        initWiFi();
        if (wifiConnected) syncNetworkTime();
      }
      drawNetworkTimeSetting();
      lastPress = currentTime;
    }
    else if (ok) {
      wifiSelectMode = true;
      drawNetworkTimeSetting();
      lastPress = currentTime;
    }
  }
  break;
    
    case STATE_BRIGHTNESS_SETTING:
  if (left) {
    currentState = STATE_SETTING_DETAIL;

    lastPress = currentTime;
  }
  else if (right) {
    currentState = STATE_SETTING_DETAIL;
    lastPress = currentTime;
  }
  else if (up) {
    if (currentBrightnessLevel < BRIGHTNESS_LEVELS - 1) {
      currentBrightnessLevel++;
      setBrightness(currentBrightnessLevel);
      drawBrightnessSetting();
    }
    lastPress = currentTime;
  }
  else if (down) {
    if (currentBrightnessLevel > 0) {
      currentBrightnessLevel--;
      setBrightness(currentBrightnessLevel);
      drawBrightnessSetting();
    }
    lastPress = currentTime;
  }
  else if (ok) {
    currentState = STATE_SETTING_DETAIL;
    lastPress = currentTime;
  }
  break;
    
   case STATE_ALARM_SETTING:
  if (left) {
    currentState = STATE_SETTING_DETAIL;
    drawSettingDetail();
    lastPress = currentTime;
  }
  else if (right) {
    alarmSelectIndex = (alarmSelectIndex + 1) % 4;
    drawAlarmSetting();
    lastPress = currentTime;
  }
  else if (up) {
    if (alarmSelectIndex == 0) alarmHour = (alarmHour + 1) % 24;
    else if (alarmSelectIndex == 1) alarmMinute = (alarmMinute + 1) % 60;
    else if (alarmSelectIndex == 2) alarmSecond = (alarmSecond + 1) % 60;
    else if (alarmSelectIndex == 3) alarmEnabled = !alarmEnabled;
    drawAlarmSetting();
    lastPress = currentTime;
  }
  else if (down) {
    if (alarmSelectIndex == 0) alarmHour = (alarmHour - 1 + 24) % 24;
    else if (alarmSelectIndex == 1) alarmMinute = (alarmMinute - 1 + 60) % 60;
    else if (alarmSelectIndex == 2) alarmSecond = (alarmSecond - 1 + 60) % 60;
    else if (alarmSelectIndex == 3) alarmEnabled = !alarmEnabled;
    drawAlarmSetting();
    lastPress = currentTime;
  }
  else if (ok) {
    currentState = STATE_SETTING_DETAIL;
    drawSettingDetail();
    lastPress = currentTime;
  }
  break;
    
    case STATE_MINE_COUNT_SETTING:
  if (left) {
    currentState = STATE_SETTING_DETAIL;
    drawSettingDetail();
    lastPress = currentTime;
  }
  else if (right) {
    tetrisSpeedSelectIndex = (tetrisSpeedSelectIndex + 1) % 3;
    drawMineCountSetting();
    lastPress = currentTime;
  }
  else if (up) {
    if (tetrisSpeedSelectIndex == 0 && mineCount < maxMineCount) mineCount++;
    else if (tetrisSpeedSelectIndex == 1 && tetrisSpeed < maxTetrisSpeed) tetrisSpeed += 50;
    else if (tetrisSpeedSelectIndex == 2) mineBackgroundIndex = (mineBackgroundIndex + 1) % (maxBackgroundIndex + 1);
    drawMineCountSetting();
    lastPress = currentTime;
  }
  else if (down) {
    if (tetrisSpeedSelectIndex == 0 && mineCount > minMineCount) mineCount--;
    else if (tetrisSpeedSelectIndex == 1 && tetrisSpeed > minTetrisSpeed) tetrisSpeed -= 50;
    else if (tetrisSpeedSelectIndex == 2) mineBackgroundIndex = (mineBackgroundIndex - 1 + maxBackgroundIndex + 1) % (maxBackgroundIndex + 1);
    drawMineCountSetting();
    lastPress = currentTime;
  }
  else if (ok) {
    currentState = STATE_SETTING_DETAIL;
    drawSettingDetail();
    lastPress = currentTime;
  }
  break;

    case STATE_ABOUT:
      if (left) {
        currentState = STATE_MAIN_MENU;
        menuSelection = 2;
        drawMainMenu();
        lastPress = currentTime;
      }
      break;

    case STATE_GAME_DINO:
      if (left) {
        currentState = STATE_GAME_SELECT;
        lastState = STATE_JUMP_GAME; 
        gameSelection = 5;
        drawGameSelect();
        lastPress = currentTime;
      }
      else if (ok && !dinoGameOver && !dinoJumping) {
        dinoJumping = true;
        dinoJumpVelocity = JUMP_FORCE;
        lastPress = currentTime;
      }
      else if (dinoGameOver && ok) {
        currentState = STATE_GAME_SELECT;
        gameSelection = 5;
        drawGameSelect();
        lastPress = currentTime;
      }
      break;

    case STATE_MINESWEEPER:
      handleMinesweeperInput();
      break;

    case STATE_TETRIS:
      handleTetrisInput();
      break;

    case STATE_CALCULATOR:
      checkCalculatorInput();
      break;

    case STATE_JUMP_GAME:
      if (left || jumpGame.gameOver && ok ) {
        currentState = STATE_GAME_SELECT;
        lastState = STATE_JUMP_GAME; 
        gameSelection = 3;
        drawGameSelect();
        lastPress = currentTime;
        return;
      }
      else if (!jumpGame.gameOver) {
        static bool lastOkStateJump = false;
        static bool chargingStarted = false;
        bool currentOkJump = ok;
        
        if (currentOkJump && !lastOkStateJump && !jumpGame.player.isJumping && jumpGame.player.onGround) {
          jumpGame.isCharging = true;
          jumpGame.chargeStartTime = millis();
          chargingStarted = true;
          lastPress = currentTime;
        }
        
        if (lastOkStateJump && !currentOkJump && chargingStarted && jumpGame.isCharging) {
          releaseJumpCharge();
          chargingStarted = false;
          lastPress = currentTime;
        }
        
        if (jumpGame.player.isJumping && jumpGame.isCharging) {
          jumpGame.isCharging = false;
          chargingStarted = false;
        }
        
        lastOkStateJump = currentOkJump;
      }
      break;
      
    default:
      break;
  }
}


// ========== 绘制计算器
void drawCalculator() {
  drawCalculatorBackground();
  updateCalcExpression();
  updateCalcResult();
  drawCalcCursor(calcCursorX, calcCursorY);
  drawTimeOnScreen(62, 2);
}

void calculateResult() {
  if (calcExpression.length() == 0) {
    calcResult = "0";
    return;
  }
  
  calcError = false;

  String expr = calcExpression;

  for (int i = 1; i < expr.length(); i++) {
    if ((expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/' || expr[i] == '^' || expr[i] == '.') &&
        (expr[i-1] == '+' || expr[i-1] == '-' || expr[i-1] == '*' || expr[i-1] == '/' || expr[i-1] == '^' || expr[i-1] == '.')) {
      calcError = true;
      calcResult = "ERROR";
      return;
    }
  }

  char lastChar = expr[expr.length() - 1];
  if (lastChar == '+' || lastChar == '-' || lastChar == '*' || lastChar == '/' || lastChar == '^' || lastChar == '.') {
    calcError = true;
    calcResult = "ERROR";
    return;
  }

  char firstChar = expr[0];
  if (firstChar == '*' || firstChar == '/' || firstChar == '^') {
    calcError = true;
    calcResult = "ERROR";
    return;
  }

  double result = evaluateExpression(expr);
  
  if (!calcError) {
    calcResult = formatResult(result);
  } else {
    calcResult = "ERROR";
  }
  
  needClearOnNextInput = true;
}

void handleCalculatorInput(char input) {

  if (calcError && input != 'C' && input != 'D') {
    calcExpression = "";
    calcResult = "";
    calcError = false;
    needClearOnNextInput = false;
  }

  if (needClearOnNextInput && input != '=' && input != 'C' && input != 'D') {
    calcExpression = "";
    calcResult = "";
    needClearOnNextInput = false;
  }
  
  switch (input) {
    case 'C':
      calcExpression = "";
      calcResult = "";
      calcError = false;
      needClearOnNextInput = false;
      break;
      
    case 'D':
      if (calcExpression.length() > 0) {
        calcExpression.remove(calcExpression.length() - 1);
      }
      if (calcExpression.length() == 0) {
        calcResult = "";
        needClearOnNextInput = false;
      }
      break;
      
    case 'R':
      currentState = STATE_GAME_SELECT;
      lastState = STATE_CALCULATOR;
      gameSelection = 0;
      drawGameSelect();
      return;
      
    case '=':
      if (calcExpression.length() > 0) {
        calculateResult();
      }
      break;
      
    case '.':
      {
        bool hasDot = false;
        for (int i = calcExpression.length() - 1; i >= 0; i--) {
          char c = calcExpression[i];
          if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') break;
          if (c == '.') {
            hasDot = true;
            break;
          }
        }
        if (!hasDot) {
          if (calcExpression.length() == 0 || 
              calcExpression[calcExpression.length()-1] == '+' ||
              calcExpression[calcExpression.length()-1] == '-' ||
              calcExpression[calcExpression.length()-1] == '*' ||
              calcExpression[calcExpression.length()-1] == '/' ||
              calcExpression[calcExpression.length()-1] == '^') {
            calcExpression += "0.";
          } else {
            calcExpression += ".";
          }
        }
      }
      break;
      
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
      if (calcExpression.length() > 0) {
        char last = calcExpression[calcExpression.length() - 1];
        if (last == '+' || last == '-' || last == '*' || last == '/' || last == '^') {
          calcExpression[calcExpression.length() - 1] = input;
        } else {
          calcExpression += input;
        }
        needClearOnNextInput = false;
      } else if (input == '-') {
        calcExpression += input;
      }
      break;
      
    default:
      if (calcResult.length() > 0 && calcExpression.length() == 0) {
        calcExpression = input;
        calcResult = "";
      } else {
        calcExpression += input;
      }
      needClearOnNextInput = false;
      break;
  }

  updateCalcExpression();
  updateCalcResult();
}
void checkCalculatorInput() {
  static unsigned long lastMoveTime = 0;
  static unsigned long lastKeyTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastMoveTime < 150) return;
  
  bool upPressed = (digitalRead(BTN_UP) == LOW);
  bool downPressed = (digitalRead(BTN_DOWN) == LOW);
  bool leftPressed = (digitalRead(BTN_LEFT) == LOW);
  bool okPressedNow = (digitalRead(BTN_OK) == LOW);
  
  bool right = (downPressed && leftPressed);
  bool up = upPressed;
  bool down = downPressed && !right;
  bool left = leftPressed && !right;
  
  int oldCursorX = calcCursorX;
  int oldCursorY = calcCursorY;
  
  // 光标移动
  if (right) {
    if (calcCursorX < 4) {
      calcCursorX++;
      lastMoveTime = currentTime;
    }
  }
  else if (left) {
    if (calcCursorX > 0) {
      calcCursorX--;
      lastMoveTime = currentTime;
    }
    else {
      currentState = STATE_GAME_SELECT;
      lastState = STATE_CALCULATOR;
      gameSelection = 0;
      drawGameSelect();
      return;
    }
  }
  else if (up) {
    if (calcCursorY > 0) {
      calcCursorY--;
      lastMoveTime = currentTime;
    }
  }
  else if (down) {
    if (calcCursorY < 3) {
      calcCursorY++;
      lastMoveTime = currentTime;
    }
  }
  else if (okPressedNow && (currentTime - lastKeyTime > 200)) {
    int index = calcCursorY * 5 + calcCursorX;
    if (index < 20) {
      char label = calcButtons[index].label[0];
      
      if (label == 'R') {
        currentState = STATE_GAME_SELECT;
        lastState = STATE_CALCULATOR;
        gameSelection = 0;
        drawGameSelect();
        return;
      } else {
        handleCalculatorInput(label);
        lastKeyTime = currentTime;
      }
    }
  }
  
  // 局部刷新光标
  if (oldCursorX != calcCursorX || oldCursorY != calcCursorY) {
    clearCalcCursor(oldCursorX, oldCursorY);
    drawCalcCursor(calcCursorX, calcCursorY);
  }
}



void drawCalculatorBackground() {
  tft.fillScreen(BLACK);

  tft.fillRect(5, 18, 118, 15, 0x2104);
  tft.drawRect(5, 18, 118, 15, WHITE);

  tft.fillRect(5, 36, 118, 15, 0x2104);
  tft.drawRect(5, 36, 118, 15, WHITE);
  
  const int btnW = 20;
  const int btnH = 18;
  const int startX = 3;
  const int actualStartY = 50;
  const int gapX = 2;
  const int gapY = 2;
  

  for (int i = 0; i < 20; i++) {
    int row = i / 5;
    int col = i % 5;
    int x = startX + col * (btnW + gapX);
    int y = actualStartY + row * (btnH + gapY);
    
    calcButtons[i].x = x;
    calcButtons[i].y = y;
    
    uint16_t color;
    char label = calcButtons[i].label[0];
    if (label == 'C' || label == 'D') {
      color = RED;
    } else if (label == '=') {
      color = GREEN;
    } else if (label == '+' || label == '-' || label == '*' || label == '/' || label == '^') {
      color = 0xFD20;
    } else if (label == 'R') {
      color = 0x07FF;
    } else {
      color = 0x39E7;
    }
    
    tft.fillRect(x, y, btnW, btnH, color);
    tft.drawRect(x, y, btnW, btnH, WHITE);
    tft.drawSmallString(calcButtons[i].label, x + 4, y + 4, WHITE);
  }
}


void updateCalcExpression() {
  tft.fillRect(6, 19, 116, 13, 0x2104);
  
  String displayExpr = calcExpression;
  if (displayExpr.length() > 14) {
    displayExpr = "..." + displayExpr.substring(displayExpr.length() - 11);
  }
  tft.drawSmallString(displayExpr.c_str(), 8, 20, WHITE);
}


void updateCalcResult() {
  tft.fillRect(6, 37, 116, 13, 0x2104);
  
  if (calcError) {
    tft.drawSmallString("ERROR", 8, 38, RED);
  } else if (calcResult.length() > 0) {
    String displayResult = calcResult;
    if (displayResult.length() > 14) {
      displayResult = displayResult.substring(0, 13) + "...";
    }
    tft.drawSmallString(displayResult.c_str(), 8, 38, YELLOW);
  }
}


void clearCalcCursor(int oldX, int oldY) {
  int oldIdx = oldY * 5 + oldX;
  int btnX = calcButtons[oldIdx].x;
  int btnY = calcButtons[oldIdx].y;
  

  tft.fillRect(btnX - 2, btnY - 2, 24, 22, BLACK);
  tft.fillRect(btnX - 3, btnY - 3, 26, 26, BLACK);
  
  // 重绘按钮
  uint16_t color;
  char label = calcButtons[oldIdx].label[0];
  if (label == 'C' || label == 'D') {
    color = GREEN;
  } else if (label == '=') {
    color = GREEN;
  } else if (label == '+' || label == '-' || label == '*' || label == '/' || label == '^') {
    color = 0xFD20;
  } else if (label == 'R') {
    color = 0x07FF;
  } else {
    color = 0x39E7;
  }
  
  tft.fillRect(btnX, btnY, 20, 18, color);
  tft.drawRect(btnX, btnY, 20, 18, WHITE);
  tft.drawSmallString(calcButtons[oldIdx].label, btnX + 4, btnY + 4, WHITE);
  

  if (oldY > 0) {
    int upIdx = (oldY - 1) * 5 + oldX;
    int upX = calcButtons[upIdx].x;
    int upY = calcButtons[upIdx].y;

    tft.fillRect(upX, upY + 16, 20, 4, BLACK);
    // 重绘相邻按钮的底部
    uint16_t upColor;
    char upLabel = calcButtons[upIdx].label[0];
    if (upLabel == 'C' || upLabel == 'D') upColor = RED;
    else if (upLabel == '=') upColor = GREEN;
    else if (upLabel == '+' || upLabel == '-' || upLabel == '*' || upLabel == '/' || upLabel == '^') upColor = 0xFD20;
    else if (upLabel == 'R') upColor = 0x07FF;
    else upColor = 0x39E7;
    tft.fillRect(upX, upY, 20, 18, upColor);
    tft.drawRect(upX, upY, 20, 18, WHITE);
    tft.drawSmallString(calcButtons[upIdx].label, upX + 4, upY + 4, WHITE);
  }
  
  // 下
  if (oldY < 3) {
    int downIdx = (oldY + 1) * 5 + oldX;
    int downX = calcButtons[downIdx].x;
    int downY = calcButtons[downIdx].y;
    uint16_t downColor;
    char downLabel = calcButtons[downIdx].label[0];
    if (downLabel == 'C' || downLabel == 'D') downColor = RED;
    else if (downLabel == '=') downColor = GREEN;
    else if (downLabel == '+' || downLabel == '-' || downLabel == '*' || downLabel == '/' || downLabel == '^') downColor = 0xFD20;
    else if (downLabel == 'R') downColor = 0x07FF;
    else downColor = 0x39E7;
    tft.fillRect(downX, downY, 20, 18, downColor);
    tft.drawRect(downX, downY, 20, 18, WHITE);
    tft.drawSmallString(calcButtons[downIdx].label, downX + 4, downY + 4, WHITE);
  }
  
  // 左
  if (oldX > 0) {
    int leftIdx = oldY * 5 + (oldX - 1);
    int leftX = calcButtons[leftIdx].x;
    int leftY = calcButtons[leftIdx].y;
    uint16_t leftColor;
    char leftLabel = calcButtons[leftIdx].label[0];
    if (leftLabel == 'C' || leftLabel == 'D') leftColor = RED;
    else if (leftLabel == '=') leftColor = GREEN;
    else if (leftLabel == '+' || leftLabel == '-' || leftLabel == '*' || leftLabel == '/' || leftLabel == '^') leftColor = 0xFD20;
    else if (leftLabel == 'R') leftColor = 0x07FF;
    else leftColor = 0x39E7;
    tft.fillRect(leftX, leftY, 20, 18, leftColor);
    tft.drawRect(leftX, leftY, 20, 18, WHITE);
    tft.drawSmallString(calcButtons[leftIdx].label, leftX + 4, leftY + 4, WHITE);
  }
  
  // 右
  if (oldX < 4) {
    int rightIdx = oldY * 5 + (oldX + 1);
    int rightX = calcButtons[rightIdx].x;
    int rightY = calcButtons[rightIdx].y;
    uint16_t rightColor;
    char rightLabel = calcButtons[rightIdx].label[0];
    if (rightLabel == 'C' || rightLabel == 'D') rightColor = RED;
    else if (rightLabel == '=') rightColor = GREEN;
    else if (rightLabel == '+' || rightLabel == '-' || rightLabel == '*' || rightLabel == '/' || rightLabel == '^') rightColor = 0xFD20;
    else if (rightLabel == 'R') rightColor = 0x07FF;
    else rightColor = 0x39E7;
    tft.fillRect(rightX, rightY, 20, 18, rightColor);
    tft.drawRect(rightX, rightY, 20, 18, WHITE);
    tft.drawSmallString(calcButtons[rightIdx].label, rightX + 4, rightY + 4, WHITE);
  }
}

// 绘制新光标
void drawCalcCursor(int x, int y) {
  int idx = y * 5 + x;
  int cursorX = calcButtons[idx].x - 2;
  int cursorY = calcButtons[idx].y - 2;
  tft.drawRect(cursorX, cursorY, 24, 22, YELLOW);
  tft.drawRect(cursorX - 1, cursorY - 1, 26, 24, YELLOW);
}




void initWiFi() {
  timeSyncStatus = "Connecting...";
  
  // 获取当前选中的WiFi
  const char* ssid = wifiNetworks[selectedWifiIndex].ssid;
  const char* password = wifiNetworks[selectedWifiIndex].password;
  
  // 显示连接状态
  tft.fillScreen(BLACK);
  tft.drawString("WiFi Setup", 25, 20, CYAN);
  tft.drawString("Connecting to:", 10, 40, WHITE);
  tft.drawString(ssid, 10, 55, YELLOW);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  int dotX = 10;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(1000);
    tft.drawString(".", dotX + attempts * 8, 70, WHITE);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    timeSyncStatus = "WiFi Connected";
    
    tft.fillRect(0, 65, 128, 30, BLACK);
    tft.drawString("Connected", 5, 75, GREEN);
    tft.drawString("IP:", 5, 90, WHITE);
    tft.drawString(WiFi.localIP().toString().c_str(), 30, 90, CYAN);
    
    delay(2000);
  } else {
    wifiConnected = false;
    timeSyncStatus = "WiFi Failed";
    
    tft.fillRect(0, 65, 128, 30, BLACK);
    tft.drawString("Failed!", 10, 75, RED);
    tft.drawString("Check password", 10, 90, YELLOW);
    
    delay(2000);
  }
}

bool syncNetworkTime() {
  if (!wifiConnected) {
    initWiFi();
  }
  
  if (wifiConnected) {
    timeSyncStatus = "Syncing time...";
    
    // 绘制状态
    tft.fillScreen(BLACK);
    tft.drawString("Time Sync", 30, 10, CYAN);
    tft.drawString("Getting time...", 5, 40, WHITE);
    
    // 配置NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov");
    
    // 等待时间同步
    int retry = 0;
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo) && retry < 10) {
      delay(1000);
      tft.drawString(".", 10 + retry * 5, 55, YELLOW);
      retry++;
    }
    
    if (getLocalTime(&timeinfo)) {
      // 更新时间
      setHour = timeinfo.tm_hour;
      setMinute = timeinfo.tm_min;
      setSecond = timeinfo.tm_sec;
      updateTimeString();
      
      //timeSynced = true;
      //timeSyncStatus = "Time synced!";
      
    
      tft.fillRect(0, 40, 128, 40, BLACK);
      tft.drawString("Time Synced!", 15, 50, GREEN);
      
      char timeBuf[20];
      sprintf(timeBuf, "%02d:%02d:%02d", setHour, setMinute, setSecond);
      tft.drawString(timeBuf, 25, 70, YELLOW);
      
      delay(2000);
      return true;
    } else {
      timeSyncStatus = "Sync failed";
      
      tft.fillRect(0, 40, 128, 40, BLACK);
      tft.drawString("Sync Failed!", 15, 50, RED);
      tft.drawString("Check WiFi/NTP", 10, 70, YELLOW);
      
      delay(2000);
      return false;
    }
  }
  
  return false;
}

void drawNetworkTimeSetting() {
  tft.fillScreen(BLACK);
  tft.drawString("NETWORK TIME", 15, 10, CYAN);
  
  // 显示当前选中的WiFi
  tft.drawString("WiFi:", 3, 28, WHITE);
  
  if (wifiSelectMode) {

    //tft.fillRect(40, 26, 80, 12, 0x39E7); 
    tft.drawRect(1, 40, 122, 12, YELLOW);  
    tft.drawString(wifiNetworks[selectedWifiIndex].ssid, 3, 42, YELLOW);
    

    //tft.drawString("<", 33, 28, YELLOW);
    //tft.drawString(">", 118, 28, YELLOW);
  } else {
    
    tft.drawString(wifiNetworks[selectedWifiIndex].ssid, 3, 42, wifiConnected ? GREEN : WHITE);
  }
  

  tft.drawString("Status:", 3, 55, WHITE);
  if (wifiConnected) {
    tft.drawString("Connected", 3, 66, GREEN);
  } else {
    tft.drawString("Not connected", 3, 66, RED);
  }

  if (wifiConnected) {
    tft.drawString("IP:", 3, 85, WHITE);
    tft.drawString(WiFi.localIP().toString().c_str(), 25, 85, CYAN);
  }

  //char timeBuf[20];
  //sprintf(timeBuf, "%02d:%02d:%02d", setHour, setMinute, setSecond);
  //tft.drawString("Time:", 5, 78, WHITE);
  //tft.drawString(timeBuf, 40, 78, YELLOW);
  
  
  
 
  if (wifiSelectMode) {
   // tft.drawString("L/R: change WiFi", 5, 98, YELLOW);
    //tft.drawString("OK: confirm", 5, 110, GREEN);
    //tft.drawString("BACK: cancel", 5, 122, WHITE);
  } else {
    //tft.drawString("OK: select WiFi", 5, 98, GREEN);
    //tft.drawString("UP: connect", 5, 110, GREEN);
    tft.drawString("DOWN: sync time", 3, 102, GREEN);
  }
}



// ========== 扫雷游戏

void initMinesweeper() {
  mineGameOver = false;
  mineGameWin = false;
  mineCursorX = 0;
  mineCursorY = 0;
  lastMineCursorX = 0;
  lastMineCursorY = 0;
  revealedCount = 0;
  flaggedCount = 0;
  

  for (int y = 0; y < MINE_SIZE; y++) {
    for (int x = 0; x < MINE_SIZE; x++) {
      mineField[y][x] = 0;
      cellState[y][x] = CELL_HIDDEN;
    }
  }
  
  //mineCount
  int minesPlaced = 0;
  while (minesPlaced < mineCount) {
    int x = random(0, MINE_SIZE);
    int y = random(0, MINE_SIZE);
    if (mineField[y][x] != -1) {
      mineField[y][x] = -1;
      minesPlaced++;
    }
  }
  
  // 计算每个格子周围的地雷数
  for (int y = 0; y < MINE_SIZE; y++) {
    for (int x = 0; x < MINE_SIZE; x++) {
      if (mineField[y][x] != -1) {
        int count = 0;
        for (int dy = -1; dy <= 1; dy++) {
          for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < MINE_SIZE && ny >= 0 && ny < MINE_SIZE) {
              if (mineField[ny][nx] == -1) count++;
            }
          }
        }
        mineField[y][x] = count;
      }
    }
  }
  
  drawMinesweeper();
}

void revealCell(int x, int y) {
  if (x < 0 || x >= MINE_SIZE || y < 0 || y >= MINE_SIZE) return;
  if (cellState[y][x] != CELL_HIDDEN) return;
  if (mineGameOver || mineGameWin) return;
  
  cellState[y][x] = CELL_REVEALED;
  revealedCount++;
  drawSingleCell(x, y);
  
  if (mineField[y][x] == -1) {
    mineGameOver = true;
    revealAllMines();
    return;
  }
  
  if (mineField[y][x] == 0) {
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        if (dx != 0 || dy != 0) {
          revealCell(x + dx, y + dy);
        }
      }
    }
  }
  
  if (revealedCount == MINE_SIZE * MINE_SIZE - mineCount) {
    mineGameWin = true;
  }
}

void drawMineCountSetting() {
  tft.fillScreen(BLACK);
  //tft.drawString("GAME SETTINGS", 15, 5, CYAN);
  
  // 选项1：地雷数量
  if (tetrisSpeedSelectIndex == 0) {
    tft.fillRect(5, 20, 7, 8, BLACK);
    tft.drawString(">", 5, 20, YELLOW);
  } else {
    tft.fillRect(5, 20, 7, 8, BLACK);
    tft.drawString(" ", 5, 20, BLACK);
  }
  char countBuf[20];
  sprintf(countBuf, "Mines: %d", mineCount);
  tft.drawString(countBuf, 20, 20, WHITE);

  int barWidth = map(mineCount, minMineCount, maxMineCount, 10, 60);
  tft.drawRect(20, 30, 60, 4, WHITE);
  tft.fillRect(21, 31, barWidth - 2, 2, RED);
  
  // 选项2：俄罗斯方块速度
  if (tetrisSpeedSelectIndex == 1) {
    tft.fillRect(5, 45, 7, 8, BLACK);
    tft.drawString(">", 5, 45, YELLOW);
  } else {
    tft.fillRect(5, 45, 7, 8, BLACK);
    tft.drawString(" ", 5, 45, BLACK);
  }
  char speedBuf[20];
  sprintf(speedBuf, "Speed: %dms", tetrisSpeed);
  tft.drawString(speedBuf, 20, 45, WHITE);
  
  int speedBarWidth = map(tetrisSpeed, minTetrisSpeed, maxTetrisSpeed, 10, 60);
  tft.drawRect(20, 55, 60, 4, WHITE);
  tft.fillRect(21, 56, speedBarWidth - 2, 2, BLUE);
  
  // 选项3：扫雷背景
  if (tetrisSpeedSelectIndex == 2) {
    tft.fillRect(5, 70, 7, 8, BLACK);
    tft.drawString(">", 5, 70, YELLOW);
  } else {
    tft.fillRect(5, 70, 7, 8, BLACK);
    tft.drawString(" ", 5, 70, BLACK);
  }
  tft.drawString("BG:", 20, 70, WHITE);
  
  switch(mineBackgroundIndex) {
    case 0: tft.drawString("Black", 55, 70, 0x8410); break;
    case 1: tft.drawString("set1", 55, 70, WHITE); break;
    case 2: tft.drawString("set2", 55, 70, WHITE); break;
    case 3: tft.drawString("set3", 55, 70, WHITE); break;
    case 4: tft.drawString("set4", 55, 70, WHITE); break;
  }
  
  //tft.drawString("L/R:switch  UP/DOWN:adj", 8, 100, GREEN);
  //tft.drawString("OK:save", 40, 115, YELLOW);
  
  drawTimeOnScreen(62, 2);
}


void restorePixel(int px, int py) {
  // 判断这个像素属于哪个格子
  for (int gy = 0; gy < MINE_SIZE; gy++) {
    for (int gx = 0; gx < MINE_SIZE; gx++) {
      int cellX = mineStartX + gx * mineCellSize;
      int cellY = mineStartY + gy * mineCellSize;

      if (px >= cellX - 2 && px <= cellX + mineCellSize + 2 &&
          py >= cellY - 2 && py <= cellY + mineCellSize + 2) {
        
      
        if (px >= cellX && px < cellX + mineCellSize && 
            py >= cellY && py < cellY + mineCellSize) {
         
          drawSingleCell(gx, gy);
          return;
        }
        
      
        drawSingleCell(gx, gy);
        if (gx > 0) drawSingleCell(gx - 1, gy);
        if (gy > 0) drawSingleCell(gx, gy - 1);
        if (gx > 0 && gy > 0) drawSingleCell(gx - 1, gy - 1);
        return;
      }
    }
  }
}

void restoreCellCorner(int px, int py, int gx, int gy) {
  if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) return;
  drawSingleCell(gx, gy);
}
// 显示所有地雷
void revealAllMines() {
  for (int y = 0; y < MINE_SIZE; y++) {
    for (int x = 0; x < MINE_SIZE; x++) {
      if (mineField[y][x] == -1) {
        cellState[y][x] = CELL_REVEALED;
        drawSingleCell(x, y);  
      }
    }
  }
  
  drawCursor(mineCursorX, mineCursorY, YELLOW);
}

void drawMinesweeper() {
  const uint8_t* bg = getMineBackground();
  if (bg != nullptr) {
    showImageFast(bg);
  } else {
    tft.fillScreen(BLACK);
  }

  drawFullGrid();
  
  lastMineCursorX = mineCursorX;
  lastMineCursorY = mineCursorY;
  drawCursor(mineCursorX, mineCursorY, YELLOW);
}

void drawFullGrid() {
  for (int y = 0; y < MINE_SIZE; y++) {
    for (int x = 0; x < MINE_SIZE; x++) {
      drawSingleCell(x, y);
    }
  }
}

void drawSingleCell(int x, int y) {
  int cellX = mineStartX + x * mineCellSize;
  int cellY = mineStartY + y * mineCellSize;
  
  // 绘制格子背景
  if (cellState[y][x] == CELL_REVEALED) {
    // 已翻开：浅灰色背景
    tft.fillRect(cellX, cellY, mineCellSize, mineCellSize, MINE_REVEALED_COLOR);
  } else {
    // 未翻开：从背景图片取对应区域显示
    const uint8_t* bg = getMineBackground();
    if (bg != nullptr) {
      // 显示背景图片对应位置
      for (int py = 0; py < mineCellSize; py++) {
        for (int px = 0; px < mineCellSize; px++) {
          int imgX = cellX + px;
          int imgY = cellY + py;
          if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
            uint8_t high = pgm_read_byte(&bg[pixelIdx]);
            uint8_t low = pgm_read_byte(&bg[pixelIdx + 1]);
            tft.drawPixel(imgX, imgY, (high << 8) | low);
          }
        }
      }
    } else {
      // 纯黑背景
      tft.fillRect(cellX, cellY, mineCellSize, mineCellSize, MINE_BG_COLOR);
    }
  }
  
  // 绘制网格线
  tft.drawRect(cellX, cellY, mineCellSize, mineCellSize, MINE_GRID_COLOR);
  
  // 绘制格子内容
  if (cellState[y][x] == CELL_REVEALED) {
    if (mineField[y][x] == -1) {
      tft.fillCircle(cellX + mineCellSize/2, cellY + mineCellSize/2, 4, RED);
    } else if (mineField[y][x] > 0) {
      uint16_t numColor;
      switch(mineField[y][x]) {
        case 1: numColor = BLUE; break;
        case 2: numColor = GREEN; break;
        case 3: numColor = RED; break;
        default: numColor = 0x7800; break;
      }
      char numStr[2];
      sprintf(numStr, "%d", mineField[y][x]);
      tft.drawChar(numStr[0], cellX + 3, cellY + 2, numColor);
    }
  } else if (cellState[y][x] == CELL_FLAGGED) {
    // 旗子 - 先恢复背景再绘制
    const uint8_t* bg = getMineBackground();
    if (bg != nullptr) {
      for (int py = 0; py < mineCellSize; py++) {
        for (int px = 0; px < mineCellSize; px++) {
          int imgX = cellX + px;
          int imgY = cellY + py;
          if (imgX >= 0 && imgX < SCREEN_WIDTH && imgY >= 0 && imgY < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (imgY * SCREEN_WIDTH + imgX) * 2;
            uint8_t high = pgm_read_byte(&bg[pixelIdx]);
            uint8_t low = pgm_read_byte(&bg[pixelIdx + 1]);
            tft.drawPixel(imgX, imgY, (high << 8) | low);
          }
        }
      }
    } else {
      tft.fillRect(cellX, cellY, mineCellSize, mineCellSize, MINE_BG_COLOR);
    }
    tft.fillTriangle(cellX + 3, cellY + 2, 
                     cellX + 9, cellY + 5,
                     cellX + 3, cellY + 8, RED);
    tft.drawLine(cellX + 3, cellY + 2, cellX + 3, cellY + 10, WHITE);
  }
}

void clearCursor(int x, int y) {

  drawSingleCell(x, y);

  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      int nx = x + dx;
      int ny = y + dy;
      if (nx >= 0 && nx < MINE_SIZE && ny >= 0 && ny < MINE_SIZE) {
        drawSingleCell(nx, ny);
      }
    }
  }

  int cursorX = mineStartX + x * mineCellSize;
  int cursorY = mineStartY + y * mineCellSize;
  
  const uint8_t* bg = getMineBackground();
  
  // 左边缘超出
  if (cursorX - 2 < mineStartX) {
    if (bg != nullptr) {
      // 从背景图片恢复
      for (int py = cursorY - 2; py < cursorY + mineCellSize + 2; py++) {
        for (int px = 0; px < mineStartX; px++) {
          if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (py * SCREEN_WIDTH + px) * 2;
            uint8_t high = pgm_read_byte(&bg[pixelIdx]);
            uint8_t low = pgm_read_byte(&bg[pixelIdx + 1]);
            tft.drawPixel(px, py, (high << 8) | low);
          }
        }
      }
    } else {
      tft.fillRect(0, cursorY - 2, mineStartX, mineCellSize + 4, BLACK);
    }
  }
  
  // 右边缘超出
  if (cursorX + mineCellSize + 2 > mineStartX + mineGridWidth) {
    int rightX = mineStartX + mineGridWidth;
    int rightW = SCREEN_WIDTH - rightX;
    if (bg != nullptr) {
      for (int py = cursorY - 2; py < cursorY + mineCellSize + 2; py++) {
        for (int px = rightX; px < SCREEN_WIDTH; px++) {
          if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (py * SCREEN_WIDTH + px) * 2;
            uint8_t high = pgm_read_byte(&bg[pixelIdx]);
            uint8_t low = pgm_read_byte(&bg[pixelIdx + 1]);
            tft.drawPixel(px, py, (high << 8) | low);
          }
        }
      }
    } else {
      tft.fillRect(rightX, cursorY - 2, rightW, mineCellSize + 4, BLACK);
    }
  }
  
  // 上边缘超出
  if (cursorY - 2 < mineStartY) {
    if (bg != nullptr) {
      for (int py = 0; py < mineStartY; py++) {
        for (int px = cursorX - 2; px < cursorX + mineCellSize + 2; px++) {
          if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (py * SCREEN_WIDTH + px) * 2;
            uint8_t high = pgm_read_byte(&bg[pixelIdx]);
            uint8_t low = pgm_read_byte(&bg[pixelIdx + 1]);
            tft.drawPixel(px, py, (high << 8) | low);
          }
        }
      }
    } else {
      tft.fillRect(cursorX - 2, 0, mineCellSize + 4, mineStartY, BLACK);
    }
  }
  
  // 下边缘超出
  if (cursorY + mineCellSize + 2 > mineStartY + mineGridWidth) {
    int bottomY = mineStartY + mineGridWidth;
    int bottomH = SCREEN_HEIGHT - bottomY;
    if (bg != nullptr) {
      for (int py = bottomY; py < SCREEN_HEIGHT; py++) {
        for (int px = cursorX - 2; px < cursorX + mineCellSize + 2; px++) {
          if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
            int pixelIdx = 8 + (py * SCREEN_WIDTH + px) * 2;
            uint8_t high = pgm_read_byte(&bg[pixelIdx]);
            uint8_t low = pgm_read_byte(&bg[pixelIdx + 1]);
            tft.drawPixel(px, py, (high << 8) | low);
          }
        }
      }
    } else {
      tft.fillRect(cursorX - 2, bottomY, mineCellSize + 4, bottomH, BLACK);
    }
  }
}

// 绘制光标
void drawCursor(int x, int y, uint16_t color) {
  int cursorX = mineStartX + x * mineCellSize;
  int cursorY = mineStartY + y * mineCellSize;
  
  tft.drawRect(cursorX - 1, cursorY - 1, mineCellSize + 2, mineCellSize + 2, color);
  tft.drawRect(cursorX - 2, cursorY - 2, mineCellSize + 4, mineCellSize + 4, color);
}
void updateGameStatus() {
  tft.fillRect(0, 115, SCREEN_WIDTH, 13, BLACK);
  
 
  int remainingMines = mineCount - flaggedCount;
  char infoBuf[20];
  sprintf(infoBuf, "M:%d", remainingMines);
  tft.drawString(infoBuf, 5, 115, YELLOW);
  
  if (mineGameOver) {
    tft.drawString("GAME OVER", 45, 115, RED);
  } else if (mineGameWin) {
    tft.drawString("YOU WIN!", 45, 115, GREEN);
  }
}
void handleMinesweeperInput() {
static unsigned long lastMoveTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastMoveTime < 100) return;
  
  bool upPressed = (digitalRead(BTN_UP) == LOW);
  bool downPressed = (digitalRead(BTN_DOWN) == LOW);
  bool leftPressed = (digitalRead(BTN_LEFT) == LOW);
  bool okPressedNow = (digitalRead(BTN_OK) == LOW);
  
  bool right = (downPressed && leftPressed);
  bool up = upPressed;
  bool down = downPressed && !right;
  bool left = leftPressed && !right;
  
  // 左键返回（
  if (left && mineCursorX == 0) {
    currentState = STATE_GAME_SELECT;
    lastState = STATE_MINESWEEPER;  
    gameSelection = 1;
    drawGameSelect();
    lastMoveTime = currentTime;
    return;
  }
  
 
  bool okDownReturn = (okPressedNow && downPressed);
  if (okDownReturn) {
    currentState = STATE_GAME_SELECT;
    lastState = STATE_MINESWEEPER;
    gameSelection = 1;
    drawGameSelect();
    lastMoveTime = currentTime;
    return;
  }
  
  
  if ((mineGameOver || mineGameWin) && okPressedNow) {
    currentState = STATE_GAME_SELECT;
    lastState = STATE_MINESWEEPER;
    gameSelection = 1;
    drawGameSelect();
    lastMoveTime = currentTime;
    return;
  }
  if (!mineGameOver && !mineGameWin) {
    // 光标移动
 
if (up) {
  clearCursor(mineCursorX, mineCursorY);
  mineCursorY = (mineCursorY - 1 + MINE_SIZE) % MINE_SIZE;
  drawCursor(mineCursorX, mineCursorY, YELLOW);
  lastMoveTime = currentTime;
}
else if (down) {
  clearCursor(mineCursorX, mineCursorY);
  mineCursorY = (mineCursorY + 1) % MINE_SIZE;
  drawCursor(mineCursorX, mineCursorY, YELLOW);
  lastMoveTime = currentTime;
}
else if (left) {
  clearCursor(mineCursorX, mineCursorY);
  mineCursorX = (mineCursorX - 1 + MINE_SIZE) % MINE_SIZE;
  drawCursor(mineCursorX, mineCursorY, YELLOW);
  lastMoveTime = currentTime;
}
else if (right) {
  clearCursor(mineCursorX, mineCursorY);
  mineCursorX = (mineCursorX + 1) % MINE_SIZE;
  drawCursor(mineCursorX, mineCursorY, YELLOW);
  lastMoveTime = currentTime;
}
    else if (okPressedNow && !downPressed) {  // 单独OK翻开
      if (cellState[mineCursorY][mineCursorX] == CELL_HIDDEN) {
        revealCell(mineCursorX, mineCursorY);
        clearCursor(mineCursorX, mineCursorY);
        drawCursor(mineCursorX, mineCursorY, YELLOW);
      //  updateGameStatus();
      }
      lastMoveTime = currentTime;
    }
    else if (right) {  // DOWN+LEFT组合标记旗子
      if (cellState[mineCursorY][mineCursorX] == CELL_HIDDEN) {
        cellState[mineCursorY][mineCursorX] = CELL_FLAGGED;
        flaggedCount++;
      } else if (cellState[mineCursorY][mineCursorX] == CELL_FLAGGED) {
        cellState[mineCursorY][mineCursorX] = CELL_HIDDEN;
        flaggedCount--;
      }
      drawSingleCell(mineCursorX, mineCursorY);
      clearCursor(mineCursorX, mineCursorY);
      drawCursor(mineCursorX, mineCursorY, YELLOW);
    //  updateGameStatus();
      lastMoveTime = currentTime + 200;
    }
  }
}
void handleMineCountSetting() {
  static unsigned long lastAdjustTime = 0;
  unsigned long currentTime = millis();
  
  bool upPressed = (digitalRead(BTN_UP) == LOW);
  bool downPressed = (digitalRead(BTN_DOWN) == LOW);
  bool leftPressed = (digitalRead(BTN_LEFT) == LOW);
  bool okPressed = (digitalRead(BTN_OK) == LOW);
  bool rightPressed = (digitalRead(BTN_DOWN) == LOW && digitalRead(BTN_LEFT) == LOW);
  
  // 左
  if (leftPressed && (currentTime - lastAdjustTime > 200)) {
    tetrisSpeedSelectIndex = (tetrisSpeedSelectIndex - 1 + 2) % 2;
    drawMineCountSetting();
    lastAdjustTime = currentTime;
    return;
  }
  
  // 右
  if (rightPressed && (currentTime - lastAdjustTime > 200)) {
    tetrisSpeedSelectIndex = (tetrisSpeedSelectIndex + 1) % 2;
    drawMineCountSetting();
    lastAdjustTime = currentTime;
    return;
  }
  
  // 上
  if (upPressed && (currentTime - lastAdjustTime > 200)) {
    if (tetrisSpeedSelectIndex == 0) {
      // 调整地雷数量
      if (mineCount < maxMineCount) {
        mineCount++;
      }
    } else {
      // 调整方块速度
      if (tetrisSpeed < maxTetrisSpeed) {
        tetrisSpeed += 50;
      }
    }
    drawMineCountSetting();
    lastAdjustTime = currentTime;
  }
  
  // 下
  if (downPressed && (currentTime - lastAdjustTime > 200)) {
    if (tetrisSpeedSelectIndex == 0) {
      // 调整地雷数量
      if (mineCount > minMineCount) {
        mineCount--;
      }
    } else {
      // 调整方块速度
      if (tetrisSpeed > minTetrisSpeed) {
        tetrisSpeed -= 50;
      }
    }
    drawMineCountSetting();
    lastAdjustTime = currentTime;
  }
  
  // OK
  if (okPressed && (currentTime - lastAdjustTime > 200)) {
    currentState = STATE_SETTINGS;
    drawSettings();
    lastAdjustTime = currentTime;
  }
}



void initTetris() {
  // 清空游戏板
  for (int y = 0; y < TETRIS_ROWS; y++) {
    for (int x = 0; x < TETRIS_COLS; x++) {
      tetrisBoard[y][x] = -1;
    }
  }
  
  tetrisGameOver = false;
  tetrisScore = 0;
  lastFallTime = millis();

  tft.fillScreen(BLACK);
  tft.drawRect(tetrisStartX - 2, tetrisStartY - 2, 
               TETRIS_COLS * TETRIS_CELL_SIZE + 4, 
               TETRIS_ROWS * TETRIS_CELL_SIZE + 4, WHITE);
  tft.fillRect(tetrisStartX, tetrisStartY, 
               TETRIS_COLS * TETRIS_CELL_SIZE, 
               TETRIS_ROWS * TETRIS_CELL_SIZE, BLACK);
  spawnShape();
  drawPreview();
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (currentShape[y][x]) {
        int boardY = currentY + y;
        int boardX = currentX + x;
        if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
          drawTetrisCell(boardX, boardY, currentShapeColor);
        }
      }
    }
  }

  tft.fillRect(0, 0, SCREEN_WIDTH, 13, BLACK);
  tft.drawString("S:", 0, 2, WHITE);
  tft.drawNumber(0, 15, 2, YELLOW);
  
  //// 操作提示
  //tft.drawString("L/R:move", 2, 122, 0x8410);
  //tft.drawString("UP:rotate", 62, 122, 0x8410);
}
void spawnShape() {
  // 第一次调用时，生成两个方块
  static bool firstSpawn = true;
  
  if (firstSpawn) {
    firstSpawn = false;
    // 生成当前方块
    currentShapeType = random(0, 5);
    currentShapeColor = tetrisColors[currentShapeType];
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        currentShape[y][x] = tetrisShapes[currentShapeType][y][x];
      }
    }
    // 生成下一个方块
    nextShapeType = random(0, 5);
    nextShapeColor = tetrisColors[nextShapeType];
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        nextShape[y][x] = tetrisShapes[nextShapeType][y][x];
      }
    }
  } else {
    // 将下一个方块变为当前方块
    currentShapeType = nextShapeType;
    currentShapeColor = nextShapeColor;
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        currentShape[y][x] = nextShape[y][x];
      }
    }
    // 生成新的下一个方块
    nextShapeType = random(0, 5);
    nextShapeColor = tetrisColors[nextShapeType];
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        nextShape[y][x] = tetrisShapes[nextShapeType][y][x];
      }
    }
    // 重绘预览区域
    drawPreview();
  }
  
  currentX = TETRIS_COLS / 2 - 2;
  currentY = 0;
  
  if (checkCollision(currentShape, currentX, currentY)) {
    tetrisGameOver = true;
  }
}
// 绘制下一个方块预览
void drawPreview() {
  tft.fillRect(previewX, previewY, 4 * previewSize + 10, 4 * previewSize + 20, BLACK);
  tft.drawString("NEXT", previewX + 2, previewY - 8, WHITE);
  
  // 绘制预览方块
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (nextShape[y][x]) {
        int px = previewX + 8 + x * previewSize;
        int py = previewY + y * previewSize + 3;
        tft.fillRect(px, py, previewSize, previewSize, nextShapeColor);
        tft.drawRect(px, py, previewSize, previewSize, 0x8410);
      }
    }
  }
}
bool checkCollision(int shape[4][4], int px, int py) {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (shape[y][x]) {
        int boardX = px + x;
        int boardY = py + y;
        
        if (boardX < 0 || boardX >= TETRIS_COLS || boardY >= TETRIS_ROWS) {
          return true;
        }
        if (boardY >= 0 && tetrisBoard[boardY][boardX] != -1) {
          return true;
        }
      }
    }
  }
  return false;
}

void placeShape() {
  // 将当前方块写入游戏板
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (currentShape[y][x]) {
        int boardY = currentY + y;
        int boardX = currentX + x;
        if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
          tetrisBoard[boardY][boardX] = currentShapeType;
        }
      }
    }
  }
  
  // 消除行并重绘
  for (int y = TETRIS_ROWS - 1; y >= 0; y--) {
    bool lineFull = true;
    for (int x = 0; x < TETRIS_COLS; x++) {
      if (tetrisBoard[y][x] == -1) { lineFull = false; break; }
    }
    
    if (lineFull) {
      tetrisScore += 1;
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < TETRIS_COLS; x++) {
          tetrisBoard[yy][x] = tetrisBoard[yy - 1][x];
        }
      }
      for (int x = 0; x < TETRIS_COLS; x++) {
        tetrisBoard[0][x] = -1;
      }
      y++;
    }
  }
  
  // 重绘游戏板
  for (int y = 0; y < TETRIS_ROWS; y++) {
    for (int x = 0; x < TETRIS_COLS; x++) {
      if (tetrisBoard[y][x] != -1) {
        drawTetrisCell(x, y, tetrisColors[tetrisBoard[y][x]]);
      } else {
        clearTetrisCell(x, y);
      }
    }
  }
  
  spawnShape();
  if (!tetrisGameOver) {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (currentShape[y][x]) {
          int boardY = currentY + y;
          int boardX = currentX + x;
          if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
            drawTetrisCell(boardX, boardY, currentShapeColor);
          }
        }
      }
    }
  }
  
  // 刷新分数
  tft.fillRect(0, 0, 40, 13, BLACK);
  tft.drawString("S:", 0, 2, WHITE);
  tft.drawNumber(tetrisScore, 15, 2, YELLOW);
  
  // 游戏结束
  if (tetrisGameOver) {
    tft.fillRect(0, tetrisStartY + 50, 20, 30, BLACK);
    tft.drawString("GAME OVER", 20, tetrisStartY + 55, RED);
    tft.drawString("OK to return", 15, tetrisStartY + 70, GREEN);
  }
}



void rotateShape() {
  int rotated[4][4] = {0};
  
  // 顺时针旋转90度
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      rotated[x][3 - y] = currentShape[y][x];
    }
  }
  
  // 检查旋转后是否碰撞
  if (!checkCollision(rotated, currentX, currentY)) {
    // 应用旋转
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        currentShape[y][x] = rotated[y][x];
      }
    }
  }
}

// 绘制单个方块格子
void drawTetrisCell(int boardX, int boardY, uint16_t color) {
  int px = tetrisStartX + boardX * TETRIS_CELL_SIZE;
  int py = tetrisStartY + boardY * TETRIS_CELL_SIZE;
  tft.fillRect(px, py, TETRIS_CELL_SIZE, TETRIS_CELL_SIZE, color);
  tft.drawRect(px, py, TETRIS_CELL_SIZE, TETRIS_CELL_SIZE, 0x8410);
}

// 清除单个方块格子
void clearTetrisCell(int boardX, int boardY) {
  int px = tetrisStartX + boardX * TETRIS_CELL_SIZE;
  int py = tetrisStartY + boardY * TETRIS_CELL_SIZE;
  tft.fillRect(px, py, TETRIS_CELL_SIZE, TETRIS_CELL_SIZE, BLACK);
}


void handleTetrisInput() {
  if (currentState != STATE_TETRIS) return;
  
  static unsigned long lastMoveTime = 0;
  unsigned long currentTime = millis();
  // 游戏结束处理
  if (tetrisGameOver) {
    if (digitalRead(BTN_OK) == LOW && (currentTime - lastMoveTime > 200)) {
      currentState = STATE_GAME_SELECT;
      lastState = STATE_TETRIS;
      gameSelection = 2;
      drawGameSelect();
      lastMoveTime = currentTime;
    }
    return;
  }
  
  if (currentTime - lastMoveTime < 300) return;
  
  bool upPressed = (digitalRead(BTN_UP) == LOW);
  bool downPressed = (digitalRead(BTN_DOWN) == LOW);
  bool leftPressed = (digitalRead(BTN_LEFT) == LOW);
  bool okPressedNow = (digitalRead(BTN_OK) == LOW);
  
  bool rightKey = (downPressed && leftPressed);

  if (leftPressed && !rightKey) {
    if (!checkCollision(currentShape, currentX - 1, currentY)) {
      // 清除旧位置
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
          if (currentShape[y][x]) {
            int boardY = currentY + y;
            int boardX = currentX + x;
            if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
              clearTetrisCell(boardX, boardY);
            }
          }
        }
      }
      currentX--;
      // 绘制新位置
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
          if (currentShape[y][x]) {
            int boardY = currentY + y;
            int boardX = currentX + x;
            if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
              drawTetrisCell(boardX, boardY, currentShapeColor);
            }
          }
        }
      }
    }
    lastMoveTime = currentTime;
  }
  
  // 右键移动方块
  if (rightKey) {
    if (!checkCollision(currentShape, currentX + 1, currentY)) {
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
          if (currentShape[y][x]) {
            int boardY = currentY + y;
            int boardX = currentX + x;
            if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
              clearTetrisCell(boardX, boardY);
            }
          }
        }
      }
      currentX++;
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
          if (currentShape[y][x]) {
            int boardY = currentY + y;
            int boardX = currentX + x;
            if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
              drawTetrisCell(boardX, boardY, currentShapeColor);
            }
          }
        }
      }
    }
    lastMoveTime = currentTime;
  }
  

  if (upPressed) {
    // 清除旧方块
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (currentShape[y][x]) {
          int boardY = currentY + y;
          int boardX = currentX + x;
          if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
            clearTetrisCell(boardX, boardY);
          }
        }
      }
    }
    rotateShape();
    // 绘制新方块
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (currentShape[y][x]) {
          int boardY = currentY + y;
          int boardX = currentX + x;
          if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
            drawTetrisCell(boardX, boardY, currentShapeColor);
          }
        }
      }
    }
    lastMoveTime = currentTime;
  }
  
  // OK键返回
  if (okPressedNow && (currentTime - lastMoveTime > 200)) {
    currentState = STATE_GAME_SELECT;
    lastState = STATE_TETRIS;
    gameSelection = 2;
    drawGameSelect();
    lastMoveTime = currentTime;
    return;
  }
  
// 自动下落
if (currentTime - lastFallTime > tetrisSpeed) { 
  if (!checkCollision(currentShape, currentX, currentY + 1)) {
    // 清除旧位置
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (currentShape[y][x]) {
          int boardY = currentY + y;
          int boardX = currentX + x;
          if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
            clearTetrisCell(boardX, boardY);
          }
        }
      }
    }
    currentY++;
    // 绘制新位置
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (currentShape[y][x]) {
          int boardY = currentY + y;
          int boardX = currentX + x;
          if (boardY >= 0 && boardY < TETRIS_ROWS && boardX >= 0 && boardX < TETRIS_COLS) {
            drawTetrisCell(boardX, boardY, currentShapeColor);
          }
        }
      }
    }
  } else {
    placeShape();
  }
  lastFallTime = currentTime;
}
}

// ========== 跳一跳游戏实现 ==========

void initJumpGame() {
  jumpGame.platformCount = 3;
  jumpGame.score = 0;
  jumpGame.gameOver = false;
  jumpGame.isCharging = false;
  jumpGame.nextDistance = 30;
  jumpGame.cameraX = 0;  // 初始相机位置为0
  
  // 初始化第一个平台
  jumpGame.platforms[0].x = 20;
  jumpGame.platforms[0].y = JUMP_PLAYER_Y;
  jumpGame.platforms[0].width = JUMP_PLATFORM_WIDTH;
  jumpGame.platforms[0].height = JUMP_PLATFORM_HEIGHT;
  
  // 生成后续平台
  jumpGame.platforms[1].x = 20 + JUMP_PLATFORM_WIDTH + 25;
  jumpGame.platforms[1].y = JUMP_PLAYER_Y;
  jumpGame.platforms[1].width = JUMP_PLATFORM_WIDTH;
  jumpGame.platforms[1].height = JUMP_PLATFORM_HEIGHT;
  
  jumpGame.platforms[2].x = jumpGame.platforms[1].x + JUMP_PLATFORM_WIDTH + 35;
  jumpGame.platforms[2].y = JUMP_PLAYER_Y;
  jumpGame.platforms[2].width = JUMP_PLATFORM_WIDTH;
  jumpGame.platforms[2].height = JUMP_PLATFORM_HEIGHT;
  
  // 初始化玩家
  jumpGame.player.x = jumpGame.platforms[0].x + JUMP_PLATFORM_WIDTH/2 - JUMP_PLAYER_SIZE/2;
  jumpGame.player.y = JUMP_PLAYER_Y - JUMP_PLAYER_SIZE;
  jumpGame.player.vx = 0;
  jumpGame.player.vy = 0;
  jumpGame.player.size = JUMP_PLAYER_SIZE;
  jumpGame.player.onGround = true;
  jumpGame.player.isJumping = false;
  jumpGame.player.currentPlatformX = jumpGame.platforms[0].x;
  jumpGame.player.currentPlatformY = jumpGame.platforms[0].y;
  
  jumpGame.highScore = 0;
  
  tft.fillScreen(BLACK);
  drawJumpGame();
}

void drawJumpGame() {
  // 清屏
  tft.fillRect(0, 0, SCREEN_WIDTH, JUMP_PLAYER_Y + 30, BLACK);
  
  // 绘制所有平台
  for (int i = 0; i < jumpGame.platformCount; i++) {
    int drawX = jumpGame.platforms[i].x - jumpGame.cameraX;
    // 只绘制屏幕内的平台
    if (drawX + jumpGame.platforms[i].width > 0 && drawX < SCREEN_WIDTH) {
      drawJumpPlatform(drawX, jumpGame.platforms[i].y, 
                       jumpGame.platforms[i].width, jumpGame.platforms[i].height);
    }
  }
  
  // 绘制玩家（使用相机偏移）
  int playerDrawX = jumpGame.player.x - jumpGame.cameraX;
  if (playerDrawX + jumpGame.player.size > 0 && playerDrawX < SCREEN_WIDTH) {
    drawJumpPlayer(playerDrawX, jumpGame.player.y, jumpGame.player.size);
  }
  
  // 绘制分数
  drawJumpScore();
  
  
  
  drawTimeOnScreen(62, 2);
}

void drawJumpPlatform(int x, int y, int width, int height) {
  // 绘制平台
  tft.fillRect(x, y, width, height, 0x8410);
  tft.drawRect(x, y, width, height, WHITE);
  // 平台顶部高亮
  tft.drawLine(x, y, x + width - 1, y, YELLOW);
}

void drawJumpPlayer(int x, int y, int size) {
  // 绘制玩家（方形小方块）
  if (!jumpGame.gameOver) {
    tft.fillRect(x, y, size, size, BLUE);
    tft.drawRect(x, y, size, size, WHITE);
    // 绘制眼睛
    tft.fillCircle(x + size - 3, y + 3, 1, WHITE);
    tft.fillCircle(x + 3, y + 3, 1, WHITE);
  } else {
    // 游戏结束时的X眼睛
    tft.fillRect(x, y, size, size, 0x8410);
    tft.drawRect(x, y, size, size, WHITE);
    tft.drawLine(x + 2, y + 2, x + size - 3, y + size - 3, RED);
    tft.drawLine(x + size - 3, y + 2, x + 2, y + size - 3, RED);
  }
}

void drawJumpScore() {
  char scoreBuf[20];
  sprintf(scoreBuf, "S:%d", jumpGame.score);
  tft.fillRect(0, 0, 50, 20, BLACK);
  tft.drawString(scoreBuf, 5, 2, YELLOW);
  
  if (jumpGame.highScore > 0) {
    sprintf(scoreBuf, "B:%d", jumpGame.highScore);
    tft.drawString(scoreBuf, 5, 12, CYAN);
  }
  
  if (jumpGame.gameOver) {
    tft.fillRect(25, 50, 80, 45, BLACK);
    tft.drawRect(25, 50, 80, 45, WHITE);
    tft.drawString("GAME OVER", 28, 55, RED);
    sprintf(scoreBuf, "SCORE:%d", jumpGame.score);
    tft.drawString(scoreBuf, 28, 70, WHITE);
    tft.drawString("OK TO EXIT", 28, 85, GREEN);
  }
}

void generateNextPlatform() {
  // 随机生成下一个平台的距离
  jumpGame.nextDistance = random(JUMP_MIN_DISTANCE, JUMP_MAX_DISTANCE + 1);
  
  // 获取最后一个平台
  JumpPlatform* lastPlatform = &jumpGame.platforms[jumpGame.platformCount - 1];
  
  // 创建新平台
  jumpGame.platforms[jumpGame.platformCount].x = lastPlatform->x + lastPlatform->width + jumpGame.nextDistance;
  jumpGame.platforms[jumpGame.platformCount].y = JUMP_PLAYER_Y;
  jumpGame.platforms[jumpGame.platformCount].width = JUMP_PLATFORM_WIDTH;
  jumpGame.platforms[jumpGame.platformCount].height = JUMP_PLATFORM_HEIGHT;
  
  jumpGame.platformCount++;
}

void startJumpCharge() {
  if (!jumpGame.player.isJumping && jumpGame.player.onGround && !jumpGame.gameOver) {
    jumpGame.isCharging = true;
    jumpGame.chargeStartTime = millis();
  }
}

void releaseJumpCharge() {
  if (!jumpGame.isCharging) return;
  
  // 计算蓄力力度
  unsigned long chargeTime = millis() - jumpGame.chargeStartTime;
  int power = constrain(chargeTime * JUMP_MAX_POWER / JUMP_CHARGE_TIME, 
                        JUMP_MIN_POWER, JUMP_MAX_POWER);
  
  jumpGame.currentChargePower = power;
  jumpGame.isCharging = false;
  
  // 执行跳跃
  jumpGame.player.onGround = false;
  jumpGame.player.isJumping = true;
  // 水平速度
  jumpGame.player.vx = power;
  jumpGame.player.vy = JUMP_FORCE; 
  
  // 清除蓄力条显示区域
  //tft.fillRect((SCREEN_WIDTH - 80)/2, 2, 80, 18, BLACK);
}

void updateJumpGame() {
  if (jumpGame.gameOver) return;

  int oldDrawX = jumpGame.player.x - jumpGame.cameraX;
  int oldY = jumpGame.player.y;

  if (jumpGame.player.isJumping) {

    jumpGame.player.vy += GRAVITY;
    jumpGame.player.y += jumpGame.player.vy;
    jumpGame.player.x += jumpGame.player.vx;
    

    if (jumpGame.player.y >= JUMP_PLAYER_Y - jumpGame.player.size) {
      jumpGame.player.y = JUMP_PLAYER_Y - jumpGame.player.size;
      jumpGame.player.vy = 0;
      jumpGame.player.vx = 0;
      jumpGame.player.isJumping = false;
      jumpGame.player.onGround = true;
      

      bool onValidPlatform = false;
      
      for (int i = 0; i < jumpGame.platformCount; i++) {
        int platformLeft = jumpGame.platforms[i].x;
        int platformRight = jumpGame.platforms[i].x + jumpGame.platforms[i].width;
        int playerLeft = jumpGame.player.x;
        int playerRight = jumpGame.player.x + jumpGame.player.size;
        
        if (playerRight > platformLeft && playerLeft < platformRight) {
          onValidPlatform = true;
          break;
        }
      }
      
      if (onValidPlatform) {
        jumpGame.score++;
        
        if (jumpGame.score > jumpGame.highScore) {
          jumpGame.highScore = jumpGame.score;
        }
        
        generateNextPlatform();
        

        int targetCameraX = jumpGame.player.x - 40;
        if (targetCameraX < 0) targetCameraX = 0;
        

        if (jumpGame.platformCount > 0) {
          int lastPlatformRight = jumpGame.platforms[jumpGame.platformCount - 1].x + 
                                   jumpGame.platforms[jumpGame.platformCount - 1].width;
          int maxCameraX = lastPlatformRight - SCREEN_WIDTH + 20;
          if (maxCameraX > 0 && targetCameraX > maxCameraX) {
            targetCameraX = maxCameraX;
          }
        }
        
        jumpGame.cameraX = targetCameraX;
        

        drawJumpGame();
      } else {
        jumpGame.gameOver = true;
        drawJumpGame();
      }
    }
    
    // 边界限制
    if (jumpGame.player.x < 0) {
      jumpGame.player.x = 0;
      jumpGame.gameOver = true;
      drawJumpGame();
      return;
    }
  }
  
  // 更新显示
  int newDrawX = jumpGame.player.x - jumpGame.cameraX;
  
  if (oldDrawX != newDrawX || oldY != jumpGame.player.y) {
    tft.fillRect(oldDrawX, oldY, jumpGame.player.size, jumpGame.player.size, BLACK);
    if (newDrawX + jumpGame.player.size > 0 && newDrawX < SCREEN_WIDTH) {
      drawJumpPlayer(newDrawX, jumpGame.player.y, jumpGame.player.size);
    }
  }
}
void setup() {
  tft.begin();
  delay(10);
  tft.setRotation(2);
  //showImageFast(gImage_set1img);
  showImageFast(gImage_set1img, 0, 0, 128, 128); 
  delay(500);
  Serial.begin(115200);
  initBuzzer();
  dht.begin();
  pinMode(BL_PIN, OUTPUT);
  ledcAttach(BL_PIN, 5000, 8);
  ledcWrite(BL_PIN, brightnessValues[currentBrightnessLevel]);
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  
  //randomSeed(analogRead(0));
  Serial.println("start");
  setHour = 0;
  setMinute = 0;
  setSecond = 0;
  alarmHour = 0;
  alarmMinute = 0;
  alarmSecond = 0;
  alarmEnabled = false;
  currentBrightnessLevel = 4;
  currentState = STATE_SPLASH;
  menuSelection = 0;
  gameSelection = 1;
  settingsSelection = 0;
  isScreenOff = false;
   //randomSeed(analogRead(0)); 
  randomSeed(esp_random());
  updateTimeString();
  //showImageFast(gImage_img1);
  showImageFast(gImage_img1, 0, 0, 128, 128); 
  drawSplashScreen();
}
void loop() {
  readDHT11();
  test ();
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastTimeUpdate >= 1000) {
    lastTimeUpdate = currentMillis;
    
    setSecond++;
    if (setSecond >= 60) {
      setSecond = 0;
      setMinute++;
      if (setMinute >= 60) {
        setMinute = 0;
        setHour++;
        if (setHour >= 24) {
          setHour = 0;
        }
      }
    }
    updateTimeString();
    
    if (!isScreenOff) {
      if (currentState == STATE_SPLASH) {
        drawLargeTimeOnSplash();
        drawTempHumOnSplash();
      } else {
        drawTimeOnScreen(62 ,2);
      }
    }
  }
  

  checkAlarm();
  
 
  static unsigned long lastTempHumUpdate = 0;
  if (currentMillis - lastTempHumUpdate > 2000) {
    lastTempHumUpdate = currentMillis;
    if (!isScreenOff && currentState == STATE_SPLASH) {
      drawTempHumOnSplash();
    }
  }
  

  bool currentOkState = (digitalRead(BTN_OK) == LOW);
  
  if (currentOkState && !okPressedState) {
    lastOkPressTime = currentMillis;
    okPressedLong = false;
  }
  
  if (currentOkState && !okPressedLong && (currentMillis - lastOkPressTime > 1000)) {
    okPressedLong = true;
    
    if (isScreenOff) {

        esp_pm_config_t pm_config = {
    .max_freq_mhz = 160,     
    .min_freq_mhz = 10,     
    .light_sleep_enable = false
  };
  esp_pm_configure(&pm_config);
      isScreenOff = false;
      ledcWrite(BL_PIN, brightnessValues[currentBrightnessLevel]);
    } else {

      esp_pm_config_t pm_config = {
    .max_freq_mhz = 10,      
    .min_freq_mhz = 10,    
    .light_sleep_enable = false
  };
  esp_pm_configure(&pm_config);
      isScreenOff = true;
      ledcWrite(BL_PIN, 0);
    }
  }
  
  if (!currentOkState && okPressedState) {
    okPressedLong = false;
  }
  
  okPressedState = currentOkState;
  
 
  if (isScreenOff) {
    delay(50);
    return;
  }


  checkButtons();
  if (currentState == STATE_SETTING_DETAIL && lastState != currentState) {
    drawSettingDetail();
  }

   if (currentState == STATE_GAME_DINO && !dinoGameOver) {
    static unsigned long lastFrame = 0;
    if (currentMillis - lastFrame > 33) {
      lastFrame = currentMillis;
      updateDinoGame();
    }
  }
  if (currentState == STATE_JUMP_GAME && !jumpGame.gameOver) {
  static unsigned long lastJumpFrame = 0;
  if (currentMillis - lastJumpFrame > 33) {
    lastJumpFrame = currentMillis;
    updateJumpGame();
  }
}
}
void test () {
   unsigned long currentMillis = millis();
  static unsigned long lastDebugPrint = 0;
  if (currentMillis - lastDebugPrint >= 1000) {
    lastDebugPrint = currentMillis;
    
    Serial.print("主频:");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.print("MHz 内存:");
    Serial.print(ESP.getFreeHeap());
    Serial.print("/");
    Serial.print(ESP.getHeapSize());
    Serial.print("B Flash:");
    Serial.print(ESP.getFlashChipSize() / 1024);
    Serial.print("KB ");
    unsigned long seconds = currentMillis / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    Serial.print(hours);
    Serial.print("h");
    Serial.print(minutes % 60);
    Serial.print("m");
    Serial.print(seconds % 60);
    Serial.println("s");
  }
}