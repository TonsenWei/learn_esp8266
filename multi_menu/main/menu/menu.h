/*
 * @Author: Wei Dongcheng
 * @Date: 2021-03-26 10:21:59
 * @LastEditTime: 2021-03-26 10:39:32
 * @LastEditors: Wei Dongcheng
 * @Description: Multi menu
 */
#ifndef MENU_H
#define MENU_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../csrc/u8g2.h"
#include "driver/gpio.h"


void display(uint8_t line); //显示菜单项并设置选中的项反白
void func(void);
void initMenu();
void updateMenu();
void moveToPreItem();
void moveToNextItem();
void enterSubMenu();
void returnPreviousMenu();

#endif
