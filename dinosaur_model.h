/*
 * 恐龙游戏相关代码
 * 原始来源：https://github.com/pan1024/google-dinosaur-game-for-esp32
 * 原作者：pan1024
 * 说明：原始项目未声明许可证。本文件是基于原项目代码的修改版本。
*/

#include<dinosaur_model/dinosaur.h>
#include<dinosaur_model/road.h>
#include<dinosaur_model/obstacle.h>
#include<dinosaur_model/cloud.h>
#include<dinosaur_model/Object.h>

bool draw();
void draw_dinosaur();
void draw_road();
void draw_obstacle();
void draw_cloud();

bool dinosaur_move();
void road_move();
void obstacle_move();
void cloud_move();

void pushImage(Object obj);
void game_start();
void game_over();
void game_init();
void set_max_score();
bool collision_detection(Object dinosaur,Object obstacle);