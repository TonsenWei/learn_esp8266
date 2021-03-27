/*
 * @Author: Wei Dongcheng
 * @Date: 2021-03-26 10:22:05
 * @LastEditTime: 2021-03-27 00:57:36
 * @LastEditors: Wei.Dongcheng
 * @Description: multi menu
 */
#include "menu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

extern u8g2_t u8g2;
extern uint8_t draw_color;


#define GPIO_LED_NUM 2 /*LED*/

typedef struct menu        //定义一个菜单
{
    unsigned char range_from,range_to; //当前显示的项开始及结束序号
    unsigned char itemCount;           //项目总数
    unsigned char selected;            //当前选择项
    char *menuItems[17];               //菜单项目
    struct menu **subMenus;            //子菜单
    struct menu *parent;               //上级菜单 ,如果是顶级则为null
    void (**func)();                   //选择相应项按确定键后执行的函数
} Menu;

/**
 * You can set itemCount to 1,so just Press Enter to submenu
*/
Menu MainMenu = { //定义主菜单
  0,3,1,0,//默认显示0-3项，总共4项，当前选择第0项
  {
  "SET01           ",
  "Suspend        >",
  "Resume         >",
  "Inquiry        >",
  "5               ",
  "6               ",
  "7               ",
  "8               "
  }
};

Menu searchMenu = {//查询菜单
0,3,6,0,
{
  "Menu Detail    ",
  "Unrecorded     ",
  "Device Info    ",
  "IP addr        ",
  "Space Size     ",
  "Soft Info      "
}
};


Menu runningMenu = {
0,3,4,0,
{
  "ADC Reading    ",
  "IP addr        ",
  "Space Size     ",
  "Soft Info      "
}
};

Menu *currentMenu;//当前的菜单

// 用于显示菜单项
void display(uint8_t line) //显示菜单项并设置选中的项反白
{
    int i;
    line = 3 - (currentMenu->range_to - line);
    u8g2_SetDrawColor(&u8g2, draw_color);
    u8g2_SetFont(&u8g2, u8g2_font_6x12_tr);//u8g2_font_crox5tb_tr 
    // u8g2_SetFont(&u8g2, u8g2_font_courR18_tr);//u8g2_font_crox5tb_tr
    u8g2_ClearBuffer(&u8g2);
    for(i = 0;i<4;i++)
    {
        // 显示四行
        u8g2_DrawStr(&u8g2, 12, (i+1)*16 - 2, currentMenu->menuItems[i+currentMenu->range_from]);
    }
    u8g2_DrawStr(&u8g2, 0, (line + 1) * 16 - 1, ">");
    u8g2_SendBuffer(&u8g2);
}

void led_on(void)
{
  gpio_set_level(GPIO_LED_NUM, 0);        /* 熄灭 */
}

void led_off(void)
{
  gpio_set_level(GPIO_LED_NUM, 1);        /* 点亮 */
}


void func(void)
{
    printf("hello");
}

extern TaskHandle_t ADC_Task_Handler;
void suspendAdcTask(void)
{
    if (ADC_Task_Handler != NULL)
    {
      vTaskSuspend(ADC_Task_Handler);
    }
    // vTaskSuspend(ADC_Task_Handler);
    
}
extern void adc_task(void *pvParameters);
void resumeAdcTask(void)
{
    if (ADC_Task_Handler != NULL)
    {
      vTaskResume(ADC_Task_Handler);
    }
    // vTaskResume(ADC_Task_Handler);
    // xTaskCreate((TaskFunction_t )adc_task,          /* 任务函数 */
    //         (const char*    )"adc_task",        /* 任务名称 */          
    //         (uint16_t       )1024,              /* 任务堆栈大小，单位为字节 */        
    //         (void*          )NULL,              /* 传递给任务函数的参数 */
    //         (UBaseType_t    )10,                /* 任务优先级,最高优先级为24 */
    //         (TaskHandle_t*  )ADC_Task_Handler); /* 任务句柄,在不需要使用任务句柄时，可以填入NULL */ 
}

// 初始化菜单:
void initMenu()
{
    MainMenu.subMenus = pvPortMalloc(sizeof(&MainMenu)*4);
    MainMenu.subMenus[0] = NULL;        //第1到3项没有子菜单置null,选择后程序会调用func中相应项中的函数
    MainMenu.subMenus[1] = &runningMenu;
    MainMenu.subMenus[2] = &runningMenu;
    MainMenu.subMenus[3] = &searchMenu; //第四项查询有子菜单
    MainMenu.func = pvPortMalloc(sizeof(&func)*8);
    MainMenu.func[0] = &suspendAdcTask;
    MainMenu.func[1] = &suspendAdcTask;
    MainMenu.func[2] = &resumeAdcTask;//当选择了并按了确定，会执行func函数
    MainMenu.func[3] = &func;
    MainMenu.func[4] = NULL;
    MainMenu.func[5] = NULL;
    MainMenu.func[6] = NULL;
    MainMenu.func[7] = NULL;
    MainMenu.parent = NULL;  //表示是顶级菜单,所以父菜单为空

    // running menu
    runningMenu.subMenus = pvPortMalloc(sizeof(&runningMenu)*4);
    runningMenu.subMenus[0] = runningMenu.subMenus[1] = runningMenu.subMenus[2] = runningMenu.subMenus[3] = NULL;
    runningMenu.func = pvPortMalloc(sizeof(&suspendAdcTask)*4);
    runningMenu.func[0] = &suspendAdcTask;
    runningMenu.func[1] = runningMenu.func[2] = runningMenu.func[3] = NULL;
    runningMenu.parent = &MainMenu;


    searchMenu.subMenus = pvPortMalloc(sizeof(&searchMenu)*6);
    searchMenu.subMenus[0] = searchMenu.subMenus[1] = searchMenu.subMenus[2] = searchMenu.subMenus[3] = searchMenu.subMenus[4] = searchMenu.subMenus[5] = NULL;
    searchMenu.func = pvPortMalloc(sizeof(&printf)*6);
    searchMenu.func[0] = searchMenu.func[1] = searchMenu.func[2] = searchMenu.func[3] = searchMenu.func[4] = searchMenu.func[5] = NULL;
    searchMenu.parent = &MainMenu;  //上一级菜单是MainMenu.进入查询子菜单后按返回键，将会显示这个菜单项
    currentMenu = &MainMenu;        //当前显示的菜单是主菜单
}

void updateMenu()
{
  display(currentMenu->selected);
}

/**
 * move to previous menu item
*/
void moveToPreItem()
{
  if(currentMenu->selected == 0)//loop menu
  {
    currentMenu->selected = currentMenu->itemCount - 1;
    currentMenu->range_to = currentMenu->itemCount - 1;
    currentMenu->range_from = currentMenu->range_to - 3;
  } 
  else
  {
    currentMenu->selected --;
    if(currentMenu->selected < currentMenu->range_from)
    {
      currentMenu->range_from = currentMenu->selected;
      currentMenu->range_to = currentMenu->range_from+3;
    }
  }
  display(currentMenu->selected);
}

/**
 * move to next menu item
*/
void moveToNextItem()
{
  if(currentMenu->selected == currentMenu->itemCount-1)//loop menu
  {
    currentMenu->selected = 0;
    currentMenu->range_from = 0;
    currentMenu->range_to = currentMenu->range_from + 3;
  } 
  else
  {
    currentMenu->selected ++;
    if(currentMenu->selected > currentMenu->range_to)
    {
      currentMenu->range_to = currentMenu->selected;
      currentMenu->range_from = currentMenu->range_to-3;
    }
  }
  display(currentMenu->selected);
}

/**
 * enter sub menu
*/
void enterSubMenu()
{
  if(currentMenu->subMenus[currentMenu->selected] != NULL)
  {
    if(currentMenu->func[currentMenu->selected] != NULL)//func
    {
      currentMenu->func[currentMenu->selected]();   //执行相应的函数
      currentMenu = currentMenu->subMenus[currentMenu->selected];//enter submenu
      display(currentMenu->selected);//返回后恢复原来的菜单状态
      // currentMenu->func[currentMenu->selected]();//执行相应的函数
      // currentMenu->parent->func[currentMenu->parent->selected]();
    } 
    else 
    {
      currentMenu = currentMenu->subMenus[currentMenu->selected];
      display(0);
    }
    //  currentMenu = currentMenu->subMenus[currentMenu->selected];
    //  display(0);
  }
  else
  {
    if(currentMenu->func[currentMenu->selected] != NULL)
    {
      currentMenu->func[currentMenu->selected]();   //执行相应的函数
      display(currentMenu->selected);               //返回后恢复原来的菜单状态
    }
  }
}


/**
 * back to uper menu
*/
void returnPreviousMenu()
{
  if(currentMenu->parent != NULL)  // 父菜单不为空，将显示父菜单
  {
    currentMenu = currentMenu->parent;
    display(currentMenu->selected);
  } 
}

