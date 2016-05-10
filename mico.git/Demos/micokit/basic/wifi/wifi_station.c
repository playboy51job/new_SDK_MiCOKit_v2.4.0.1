/**
******************************************************************************
* @file    wifi_station.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   First MiCO application to say hello world!
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

#include "MICO.h"

/*本地路由器ssid和keyword信息，指定方式连接*/
char *ap_ssid = "mxchip";
char *ap_key  = "88888888";



#define wifi_station_log(M, ...) custom_log("WIFI", M, ##__VA_ARGS__)

static void micoNotify_ConnectFailedHandler(OSStatus err, void* inContext)
{
  wifi_station_log("join Wlan failed Err: %d", err);
}

static void micoNotify_WifiStatusHandler(WiFiEvent event,  void* inContext)
{
  switch (event) 
  {
  case NOTIFY_STATION_UP://连接成功
      wifi_station_log("Station up");
      break;
  case NOTIFY_STATION_DOWN:
      wifi_station_log("Station down");
      break;
  default:
      break;
  }
}


int application_start( void )
{

  OSStatus err = kNoErr;
  network_InitTypeDef_adv_st  wNetConfigAdv={0}; 
  wifi_station_log("wifi station api");

  /* 
      根据mico_config.h 
      1、如果定义了MICO_WLAN_CONNECTION_ENABLE宏,
       调用mico_system_init函数，此函数将会调用一系列函数，
       包括操作系统和 射频的初始化，EasyLink，注册异步
       事件，EasyLink按钮,持久化存储等
      2、如果不定义，全部函数自己手动写
   */
  mico_system_init( mico_system_context_init( 0 ) );
#ifdef MICO_WLAN_CONNECTION_ENABLE 
   //空代码，easylink配置设备Station模式
  wifi_station_log("entering easylink mode");
#else
  //注册用户函数，当连接状态改变时响应
  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler, NULL );
  require_noerr( err, exit ); 
  //注册用户函数，当无线连接失败
  err = mico_system_notify_register( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler, NULL );
  require_noerr( err, exit );
  
  //初始化无线参数
  strcpy((char*)wNetConfigAdv.ap_info.ssid, ap_ssid);//wlan ssid字符串
  strcpy((char*)wNetConfigAdv.key, ap_key);          // wlan key 字符串
  wNetConfigAdv.key_len = strlen(ap_key);            // wlan key 长度 *
  wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;//wlan 安全模式 
  wNetConfigAdv.ap_info.channel = 0;                  // 自动选择通道
  wNetConfigAdv.dhcpMode = DHCP_Client;               // 从DHCP 服务器中获取IP地址
  wNetConfigAdv.wifi_retry_interval = 100;            //一次连接失败后的重连间隔
  //开始连接......
  wifi_station_log("connecting to %s...", ap_ssid);
  micoWlanStartAdv(&wNetConfigAdv);

#endif

  /* 检查链路状态*/
  do
  {
     LinkStatusTypeDef link_status={0};
     err=micoWlanGetLinkStatus(&link_status);
     msleep(100);
	 //检查连接是否成功
     if (link_status.is_connected == 1)
       break;
  }while(1);
  
  //获取IP 状态信息
  IPStatusTypedef ip_status={0};
  micoWlanGetIPStatus(&ip_status, Station);
  //wifi_station_log("Already connected to %s successful", ap_ssid);
  wifi_station_log("IP Address : %s", ip_status.ip );
  wifi_station_log("Mask : %s", ip_status.mask );
  wifi_station_log("Gateway : %s", ip_status.gate );
  wifi_station_log("Broadcast : %s", ip_status.broadcastip );
  wifi_station_log("DNS Server : %s", ip_status.dns );
  wifi_station_log("Mac Address: %s", ip_status.mac);
  switch (ip_status.dhcp)
  {
        case DHCP_Disable:
            wifi_station_log("DHCP Off");
            break;
        case DHCP_Client:
            wifi_station_log("DHCP Client");
            break;
        case DHCP_Server:
	    wifi_station_log("DHCP Server");
            break;
        default:
            break;
   }

exit:
  mico_rtos_delete_thread(NULL);
  return err;
}

