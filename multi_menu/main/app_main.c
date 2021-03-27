/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "csrc/u8g2.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/adc.h"

#include <string.h>
#include <stdlib.h>

#include "menu/menu.h"
#include "csrc/u8x8.h"

// #define GPIO_OUTPUT_IO_0    0
// #define GPIO_OUTPUT_IO_1    2
// #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

#define GPIO_KEY_UP   0   /*key*/
#define GPIO_KEY_DOWN 13  /*key*/
#define GPIO_KEY_OK   12  /*key*/
#define GPIO_KEY_BACK 14  /*key*/
#define GPIO_LED_NUM  2   /*LED*/

#define I2C_EXAMPLE_MASTER_SCL_IO 4   /*!< gpio number for I2C master clock */
#define I2C_EXAMPLE_MASTER_SDA_IO 5   /*!< gpio number for I2C master data  */
#define I2C_EXAMPLE_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master dev*/


/* 定义按键检测任务的任务句柄 */
TaskHandle_t Key_Task_Handler;
/* 声明按键检测任务函数 */
void key_task(void *pvParameters);


/* 定义LED flash 任务的任务句柄*/
TaskHandle_t Led_Task_Handler;
/* 声明LED flash任务函数 */
void led_task(void *pvParameters);


/* 定义ADC 任务的任务句柄 */
TaskHandle_t ADC_Task_Handler;
/* 声明ADC任务函数 */
void adc_task(void *pvParameters);


u8g2_t u8g2;
typedef u8g2_uint_t u8g_uint_t;
uint8_t flip_color = 0;
uint8_t draw_color = 1;
void draw_pixel(void);
void draw_char(void);
void draw_clip_test(void);
void draw_set_screen(void);
void show_result(const char *s, uint16_t fps);
void draw_line(void);
const char *convert_FPS(uint16_t fps);
const char *convert_ADC(uint16_t adc_value);
uint16_t execute_with_fps(void (*draw_fn)(void));
const char *TAG = "u8g2";

void drawBattery(int x, int y);
void drawBatteryFull(int x, int y);
void drawBatterycharging(int x, int y);
void drawBatteryLow(int x, int y);

esp_err_t i2c_example_master_init() {
  // int i2c_master_port = I2C_NUM_0;
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;
  conf.sda_pullup_en = 1;
  conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;
  conf.scl_pullup_en = 1;
  conf.clk_stretch_tick =
      150; // 300 ticks, Clock stretch is about 210us, you can make changes
           // according to the actual situation.
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode));
  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
  return ESP_OK;
}
uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                            U8X8_UNUSED void *arg_ptr) {
  portENTER_CRITICAL();
  switch (msg) {
  case U8X8_MSG_DELAY_MILLI:
    vTaskDelay(pdMS_TO_TICKS(arg_int));
    break;
  case U8X8_MSG_DELAY_I2C:
    ets_delay_us(arg_int <= 2 ? 5 : 2);
    break;
  }
  taskEXIT_CRITICAL();
  return 1;
}
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                         void *arg_ptr) {
  portENTER_CRITICAL();
  static i2c_cmd_handle_t cmd;
  static uint8_t buffer[32];
  static uint8_t buf_idx;
  static uint8_t *data;
  static uint8_t addr;
  switch (msg) {
  case U8X8_MSG_BYTE_SEND:
    data = (uint8_t *)arg_ptr;
    while (arg_int > 0) {
      buffer[buf_idx++] = *data;
      data++;
      arg_int--;
    }
    break;
  case U8X8_MSG_BYTE_START_TRANSFER:
    cmd = i2c_cmd_link_create();
    buf_idx = 0;
    break;
  case U8X8_MSG_BYTE_END_TRANSFER:
    addr = u8x8_GetI2CAddress(u8x8);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr | I2C_MASTER_WRITE, 1);
    i2c_master_write(cmd, buffer, buf_idx, 1);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    break;
  default:
    taskEXIT_CRITICAL();
    return 0;
  }
  taskEXIT_CRITICAL();
  return 1;
}

void task_oled(void *para) {
  printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n==========Begin===========\n");
  i2c_example_master_init();
  // u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_hw_i2c, u8x8_gpio_and_delay);//u8g2_Setup_ssd1306_i2c_128x64_noname_1
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_hw_i2c, u8x8_gpio_and_delay);//u8g2_Setup_ssd1306_i2c_128x64_noname_1
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0); // wake up display
  initMenu();
  updateMenu();
  while (1) {
    // if (eTaskGetState(ADC_Task_Handler) == 2) //suspend
    // {
    //   updateMenu();
    // }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void adc_task(void *pvParameters) {
  ESP_LOGI(TAG, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n==========adc read start==========\r\n");
  adc_config_t adc_config;
  adc_config.mode = ADC_READ_TOUT_MODE;
  adc_config.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
  ESP_ERROR_CHECK(adc_init(&adc_config));
  ESP_LOGI(TAG, "adc init ok, start read...\r\n");
  uint16_t adc_data[100];
  /*suspend task after init adc, when select the resume,then resume task*/
  vTaskSuspend(ADC_Task_Handler);
  while (1) {
    if (ESP_OK == adc_read(&adc_data[0])) {
        ESP_LOGI(TAG, "adc read: %d\r\n", adc_data[0]);
        u8g2_SetDrawColor(&u8g2, draw_color);
        u8g2_SetFont(&u8g2, u8g2_font_6x12_tr);//u8g2_font_crox5tb_tr 
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawStr(&u8g2, 0, 16*2 - 1, "read value:");
        u8g2_DrawStr(&u8g2, 0, 16*3 - 1, convert_ADC(adc_data[0]));
        u8g2_SendBuffer(&u8g2);
    }

    // ESP_LOGI(TAG, "adc read fast:\r\n");

    // if (ESP_OK == adc_read_fast(adc_data, 100)) {
    //     for (x = 0; x < 100; x++) {
    //         printf("%d\n", adc_data[x]);
    //     }
    // }

    vTaskDelay(1000 / portTICK_RATE_MS);
  }
}

void app_main()
{
 
    // printf("Hello world!\n");

    // /* Print chip information */
    // esp_chip_info_t chip_info;
    // esp_chip_info(&chip_info);
    // printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
    //         chip_info.cores);

    // printf("silicon revision %d, ", chip_info.revision);

    // printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
    //         (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");


    printf("Init GPIO2...\n");
    //输出模式，禁止中断
    gpio_config_t io_conf;
    //禁止中断
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //设置为输出模式
    io_conf.mode = GPIO_MODE_OUTPUT;
    //管脚的位
    io_conf.pin_bit_mask = (1ULL << 2);
    //禁止下拉
    io_conf.pull_down_en = 0;
    //禁止上拉
    io_conf.pull_up_en = 0;
    //开始配置管脚
    gpio_config(&io_conf);

    /* 初始化gpio配置结构体 */
    io_conf.pin_bit_mask = (1ULL << GPIO_KEY_UP);/* 选择gpio0 GPIO_KEY_OK*/
    io_conf.mode = GPIO_MODE_INPUT;               /* 输入模式 */
    io_conf.pull_up_en = 0;                       /* 不上拉,因为外部有接上拉电阻 */
    io_conf.pull_down_en = 0;                     /* 不下拉 */
    io_conf.intr_type = GPIO_INTR_DISABLE;        /* 禁止中断 */ 
    /* 根据设定参数初始化并使能 */  
	  gpio_config(&io_conf);

    /* 初始化gpio配置结构体 */
    io_conf.pin_bit_mask = ((1ULL << GPIO_KEY_DOWN) | (1ULL << GPIO_KEY_OK) | (1ULL << GPIO_KEY_BACK));/* 选择gpio0 GPIO_KEY_OK*/
    io_conf.mode = GPIO_MODE_INPUT;               /* 输入模式 */
    io_conf.pull_up_en = 1;                       /* 上拉，因为外部没有接上拉电阻，使用内部上拉 */
    io_conf.pull_down_en = 0;                     /* 不下拉 */
    io_conf.intr_type = GPIO_INTR_DISABLE;        /* 禁止中断 */ 
    /* 根据设定参数初始化并使能 */  
	  gpio_config(&io_conf);

    gpio_set_level(GPIO_LED_NUM, 0);              /* 点亮LED */

    /* 创建按键检测任务 */
    xTaskCreate((TaskFunction_t )key_task,          /* 任务函数 */
                (const char*    )"key_task",        /* 任务名称*/          
                (uint16_t       )2048,              /* 任务堆栈大小，单位为字节*/        
                (void*          )NULL,              /* 传递给任务函数的参数*/
                (UBaseType_t    )10,                /* 任务优先级,最高优先级为24 */
                &Key_Task_Handler); /* 任务句柄,在不需要使用任务句柄时，可以填入NULL*/ 
    
    /* 创建oled显示任务 */
    xTaskCreate((TaskFunction_t )task_oled,         /* 任务函数 */
                (const char*    )"task_oled",       /* 任务名称 */          
                (uint16_t       )8192,              /* 任务堆栈大小，单位为字节 */        
                (void*          )NULL,              /* 传递给任务函数的参数 */
                (UBaseType_t    )10,                /* 任务优先级,最高优先级为24 */
                (TaskHandle_t*  )NULL);             /* 任务句柄,在不需要使用任务句柄时，可以填入NULL */

    /* 创建led显示任务 */
    xTaskCreate((TaskFunction_t )led_task,         /* 任务函数 */
                (const char*    )"led_task",       /* 任务名称 */          
                (uint16_t       )1024,              /* 任务堆栈大小，单位为字节 */        
                (void*          )NULL,              /* 传递给任务函数的参数 */
                (UBaseType_t    )10,                /* 任务优先级,最高优先级为24 */
                &Led_Task_Handler); /* 任务句柄,在不需要使用任务句柄时，可以填入NULL */

    /* 创建ADC任务 */
    xTaskCreate((TaskFunction_t )adc_task,          /* 任务函数 */
                (const char*    )"adc_task",        /* 任务名称 */          
                (uint16_t       )1024,              /* 任务堆栈大小，单位为字节 */        
                (void*          )NULL,              /* 传递给任务函数的参数 */
                (UBaseType_t    )10,                /* 任务优先级,最高优先级为24 */
                &ADC_Task_Handler); /* 任务句柄,在不需要使用任务句柄时，可以填入NULL */ 

    while(1)
    {
        // gpio_set_level(GPIO_LED_NUM, 1);            /* 熄灭 */
        // vTaskDelay(500 / portTICK_PERIOD_MS);       /* 延时300ms */
        // gpio_set_level(GPIO_LED_NUM, 0);            /* 点亮 */
        vTaskDelay(500 / portTICK_PERIOD_MS);       /* 延时300ms */
    }
    // for (int i = 10; i >= 0; i--) {
    //     printf("Restarting in %d seconds...\n", i);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     // gpio_set_level(2, i % 2);
    // }
    // printf("Restarting now.\n");
    // fflush(stdout);
    // esp_restart();
}

/* 按键检测任务函数 */
void key_task(void *pvParameters)
{
    static int key_up = 1;   /* 按键松开标志 */
    while (1)
    {
        /* 检测按键是否按下 */
        if (key_up && ((gpio_get_level(GPIO_KEY_UP) == 0) 
                      | (gpio_get_level(GPIO_KEY_DOWN) == 0)
                      | (gpio_get_level(GPIO_KEY_OK) == 0)
                      | (gpio_get_level(GPIO_KEY_BACK) == 0)
                      ) )
        {
            vTaskDelay(50 / portTICK_PERIOD_MS);   /* 延时50ms消抖 */
            key_up = 0;
            if (gpio_get_level(GPIO_KEY_UP) == 0)
            {
              /* 按键BOOT按下，按键按下处理*/
              printf("BOOT Key pressed!\n");
              moveToPreItem();
            } 
            else if (gpio_get_level(GPIO_KEY_DOWN) == 0)
            {
              moveToNextItem();
            } 
            else if (gpio_get_level(GPIO_KEY_OK) == 0)
            {
              enterSubMenu();
            } 
            else if (gpio_get_level(GPIO_KEY_BACK) == 0)
            {
              returnPreviousMenu();
            }
        }
        else if( (gpio_get_level(GPIO_KEY_UP) == 1) 
                // && (gpio_get_level(GPIO_KEY_UP) == 1)
                && (gpio_get_level(GPIO_KEY_DOWN) == 1)
                && (gpio_get_level(GPIO_KEY_OK) == 1)
                && (gpio_get_level(GPIO_KEY_BACK) == 1)
                )
        {
            key_up = 1;     /* 按键已松开 */
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


/*LED任务函数 */
void led_task(void *pvParameters)
{
    static int led_time_counter = 0;
    while (1)
    {
        led_time_counter ++;
        if (led_time_counter > 1000)
        {
            led_time_counter = 0;
            printf("Toogle LED %d!\n", led_time_counter);
        }
        gpio_set_level(GPIO_LED_NUM, led_time_counter % 2);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

/**
 * @description: 使用画实心方形的方式把屏幕清空
 * @return {*}
 */
void draw_set_screen(void) {
  // graphic commands to redraw the complete screen should be placed here
  u8g2_SetDrawColor(&u8g2, flip_color);  // flip_color=0 
  // 画实心方形
  u8g2_DrawBox(&u8g2, 0, 0, u8g2_GetDisplayWidth(&u8g2),
               u8g2_GetDisplayHeight(&u8g2));
}

void draw_clip_test(void) {
  u8g_uint_t i, j, k;
  char buf[3] = "AB";
  k = 0;
  u8g2_SetDrawColor(&u8g2, draw_color);
  u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
  for (i = 0; i < 6; i++) {
    for (j = 1; j < 8; j++) {
      u8g2_DrawHLine(&u8g2, i - 3, k, j);
      u8g2_DrawHLine(&u8g2, i - 3 + 10, k, j);
      u8g2_DrawVLine(&u8g2, k + 20, i - 3, j);
      u8g2_DrawVLine(&u8g2, k + 20, i - 3 + 10, j);
      k++;
    }
  }
  u8g2_SetFontDirection(&u8g2, 0);
  u8g2_DrawStr(&u8g2, 0 - 3, 50, buf);
  u8g2_SetFontDirection(&u8g2, 2);
  u8g2_DrawStr(&u8g2, 0 + 3, 50, buf);
  u8g2_SetFontDirection(&u8g2, 0);
  u8g2_DrawStr(&u8g2, u8g2_GetDisplayWidth(&u8g2) - 3, 40, buf);
  u8g2_SetFontDirection(&u8g2, 2);
  u8g2_DrawStr(&u8g2, u8g2_GetDisplayWidth(&u8g2) + 3, 40, buf);
  u8g2_SetFontDirection(&u8g2, 1);
  u8g2_DrawStr(&u8g2, u8g2_GetDisplayWidth(&u8g2) - 10, 0 - 3, buf);
  u8g2_SetFontDirection(&u8g2, 3);
  u8g2_DrawStr(&u8g2, u8g2_GetDisplayWidth(&u8g2) - 10, 3, buf);
  u8g2_SetFontDirection(&u8g2, 1);
  u8g2_DrawStr(&u8g2, u8g2_GetDisplayWidth(&u8g2) - 20,
               u8g2_GetDisplayHeight(&u8g2) - 3, buf);
  u8g2_SetFontDirection(&u8g2, 3);
  u8g2_DrawStr(&u8g2, u8g2_GetDisplayWidth(&u8g2) - 20,
               u8g2_GetDisplayHeight(&u8g2) + 3, buf);
  u8g2_SetFontDirection(&u8g2, 0);
}

/**
 * @description: 刷满屏的@字符
 * @return {*}
 */
void draw_char(void) {
  char buf[2] = "@";
  u8g_uint_t i, j;
  // graphic commands to redraw the complete screen should be placed here
  u8g2_SetDrawColor(&u8g2, draw_color);
  u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
  j = 8;
  for (;;) {
    i = 0;
    for (;;) {
      u8g2_DrawStr(&u8g2, i, j, buf);
      i += 8;
      if (i > u8g2_GetDisplayWidth(&u8g2))
        break;
    }
    j += 8;
    if (j > u8g2_GetDisplayHeight(&u8g2))
      break;
  }
}

/**
 * @description: 待分析
 * @return {*}
 */
void draw_pixel(void) {
  u8g_uint_t x, y, w2, h2;
  u8g2_SetDrawColor(&u8g2, draw_color);
  w2 = u8g2_GetDisplayWidth(&u8g2);
  h2 = u8g2_GetDisplayHeight(&u8g2);
  w2 /= 2;  //宽度的一半
  h2 /= 2;  //高度的一半
  for (y = 0; y < h2; y++) {  //高度的一半
    for (x = 0; x < w2; x++) {//宽度的一半
      if ((x + y) & 1) {      //这个干吗的？
        u8g2_DrawPixel(&u8g2, x, y);           // y = 0, x = 0 - 宽度的一半
        u8g2_DrawPixel(&u8g2, x, y + h2);      // y = 高度的一半, x = 0 - 宽度的一半
        u8g2_DrawPixel(&u8g2, x + w2, y);
        u8g2_DrawPixel(&u8g2, x + w2, y + h2);
      }
    }
  }
}

/**
 * @description: 画从0，0 到 127 63的对角线
 * @return {*}
 */
void draw_line(void) {
  u8g2_SetDrawColor(&u8g2, draw_color);
  u8g2_DrawLine(&u8g2, 0, 0, u8g2_GetDisplayWidth(&u8g2) - 1,//显示宽度 - 1 (127)
                u8g2_GetDisplayHeight(&u8g2) - 1);//显示器高度-1 （63）
}

/**
 * 计算帧率
 * 一定时间内看能刷多少帧
 * xTaskGetTickCount() ： 开机以来已经经过的毫秒数比如，这里时间为10秒
 * 然后刷新了FPS10帧（假设为200），则帧率 = 200 / 10 = 20FPS
*/
uint16_t execute_with_fps(void (*draw_fn)(void)) {
  uint16_t FPS10 = 0;
  uint32_t time;
  time = xTaskGetTickCount() + 10 * 1000;
  // picture loop
  do {
    u8g2_ClearBuffer(&u8g2);
    draw_fn();
    u8g2_SendBuffer(&u8g2);
    FPS10++;
    flip_color = flip_color ^ 1;
  } while (xTaskGetTickCount() < time);
  return FPS10;
}

/**
 * @description: 计算帧率并显示结果
 * @return {void}
 * @param {const char} *s 提示字符串，用于说明当前的帧率是什么帧率
 * @param {uint16_t} fps 待转换为实际帧率的帧率值
 */
void show_result(const char *s, uint16_t fps) {
  // assign default color value
  u8g2_SetDrawColor(&u8g2, draw_color);
  u8g2_SetFont(&u8g2, u8g2_font_8x13B_tf);
  u8g2_ClearBuffer(&u8g2);
  u8g2_DrawStr(&u8g2, 0, 12, s);
  u8g2_DrawStr(&u8g2, 0, 24, convert_FPS(fps));
  u8g2_SendBuffer(&u8g2);
}

/**
 * @description: 帧率转换，十秒的帧数换算为每秒帧数，即帧率，把帧率转换为字符串，如012.5
 * @return {*}
 * @param {uint16_t} fps
 */
const char *convert_FPS(uint16_t fps) {
  static char buf[6];
  strcpy(buf, u8g2_u8toa((uint8_t)(fps / 10), 3));  // 字符串拷贝
  buf[3] = '.';
  buf[4] = (fps % 10) + '0';
  buf[5] = '\0';
  return buf;
}

/**
 * @description: 帧率转换，十秒的帧数换算为每秒帧数，即帧率，把帧率转换为字符串，如012.5
 * @return {*}
 * @param {uint16_t} fps
 */
const char *convert_ADC(uint16_t adc_value) {
  static char buf[6];
  strcpy(buf, u8g2_u16toa(adc_value, 4));  // 字符串拷贝,max uint16 is 65535(5),now is 4(1024 ad value)
  // buf[3] = '.';
  // buf[4] = (adc_value % 10) + '0';
  buf[5] = '\0';
  return buf;
}

/**
 * @description: 画电池图案
 * @return {*}
 * @param {int} x 从哪个位置开始画
 * @param {int} y
 */
void drawBattery(int x, int y)
{
  u8g2_DrawBox(&u8g2, x, y + 8, 3, 14);  // 画实心方形，电池凸起的正极
  u8g2_DrawLine(&u8g2, x + 2, y + 1, x + 2, y + 28);//亮点之间画线
  u8g2_DrawFrame(&u8g2, x + 3, y, 54, 30);//画空心方形
  u8g2_DrawFrame(&u8g2, x + 4, y + 1, 52, 28);
  u8g2_DrawLine(&u8g2, x + 57, y + 1, x + 57, y + 28);
}

void drawBatteryFull(int x, int y)
{
  u8g2_DrawBox(&u8g2, x + 7, y + 4, 46, 22);
}

void drawBatterycharging(int x, int y)
{
  int x1 = x + 18;
  int y1 = y + 5;
  // 画实心三角形，两个三角形组成一个闪电图案，表示充电中
  u8g2_DrawTriangle(&u8g2, x1, y1 + 4, x1 + 14, y1 + 18, x1 + 15, y1 + 9);
  u8g2_DrawTriangle(&u8g2, x1 + 10, y1, x1 + 26, y1 + 15, x1 + 11, y1 + 9);
}

void drawBatteryLow(int x, int y)
{
  u8g2_DrawBox(&u8g2, x + 50, y + 4, 3, 22);
}
