/*
 * 恐龙游戏相关代码
 * 原始来源：https://github.com/pan1024/google-dinosaur-game-for-esp32
 * 原作者：pan1024
 * 说明：原始项目未声明许可证。本文件是基于原项目代码的修改版本。
*/

#include<Object.h>
Object::Object(uint8_t width,uint8_t height,int index_x,int index_y,const uint16_t *data)
{
    this->width=width;
    this->height=height;
    this->index_x=index_x;
    this->index_y=index_y;
    this->data=data;
}

Object::~Object()
{
}
