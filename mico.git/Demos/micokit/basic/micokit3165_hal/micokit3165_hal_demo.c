/**
******************************************************************************
* @file    micokit3165_hal_demo.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   Sensor test for MiCOKit 3165 board
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/

#include "mico.h" 
#include "micokit_ext.h"


#define app_log(format, ...)  custom_log("MiCOKit3165_ext", format, ##__VA_ARGS__)

void micokit_STmems_key1_clicked_callback(void)
{
  dc_motor_set(0);
}

void micokit_STmems_key2_clicked_callback(void)
{
  dc_motor_set(1);
}


int application_start( void )
{
  OSStatus err = kNoErr;
  int rgb_led_hue = 0;
  uint8_t dht11_temperature=0;
  uint8_t dht11_humidity=0;
  uint16_t infrared_reflective_data=0;
  uint16_t apds9930_Prox=0;
  uint16_t apds9930_Lux=0;
    
  char oled_string[32];
  mico_Context_t* context;
  // Start MiCO system functions according to mico_config.h
  err = mico_system_init( mico_system_context_init( 0 ) );
  require_noerr_string( err, exit, "ERROR: Unable to Init MiCO core" );
  
  context = mico_system_context_get();//获取系统信息

  micokit_ext_init();
  infrared_reflective_init();
  require_noerr_action( err, exit, app_log("ERROR: Unable to Init MiCO core" ));
  
  DHT11_Init(); //Init DHT11
  
  OLED_Init();
  OLED_Clear();
  //显示在第一行
  OLED_ShowString( 0, OLED_DISPLAY_ROW_1, (uint8_t *)context->micoStatus.mac );
  while(1)
  {
     mico_thread_sleep(1);

     
     hsb2rgb_led_init();
     hsb2rgb_led_open(rgb_led_hue, 100, 5);//氛围灯变色
     rgb_led_hue = (rgb_led_hue + 120)%360;
     
     DHT11_Read_Data(&dht11_temperature,&dht11_humidity); //Read DHT11 Value
     sprintf( oled_string, "T:%dC   H:%d%%", dht11_temperature, dht11_humidity);
     OLED_ShowString( 0, OLED_DISPLAY_ROW_2, (uint8_t *)oled_string );
     app_log("Temp:%dC Humi%d%%",dht11_temperature, dht11_humidity);
     
     
     infrared_reflective_read(&infrared_reflective_data); //Read refrared Value
     sprintf( oled_string, "IR:%d", infrared_reflective_data);
     OLED_ShowString( 0, OLED_DISPLAY_ROW_3, (uint8_t *)oled_string );
     app_log("IR data:%d",infrared_reflective_data);
     
     err=apds9930_sensor_init();
     require_noerr_action( err, exit, app_log("ERROR: Unable to Init MiCO core" ));
     apds9930_data_readout(&apds9930_Prox, &apds9930_Lux);
     sprintf( oled_string, "P:%d L:%d", apds9930_Prox, apds9930_Lux);
     OLED_ShowString( 0, OLED_DISPLAY_ROW_4, (uint8_t *)oled_string );
     app_log("Prox:%d Lux%d",apds9930_Prox, apds9930_Lux);
     
  }
exit:
  mico_rtos_delete_thread( NULL );
  return err; 
}

