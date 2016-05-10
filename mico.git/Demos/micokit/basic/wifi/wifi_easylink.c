/**
******************************************************************************
* @file    wifi_easylink.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   easylink to let device link to ap
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
#include "StringUtils.h"
#include "SocketUtils.h"



static mico_semaphore_t   easylink_sem;
static bool               easylink_success = false; /**< true: connect to wlan, false: start soft ap mode or roll back to previoude settings */
static uint32_t           easylinkIndentifier = 0; /**< Unique for an easylink instance. */
static mico_config_source_t source = CONFIG_BY_NONE;
static uint8_t            airkiss_random = 0x0;


#define wifi_easylink_log(M, ...) custom_log("WIFI", M, ##__VA_ARGS__)


/*mico EasyLink回调 step 1:获取ssid和key*/
static void EasyLinkNotify_EasyLinkCompleteHandler(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext)
{
  char ssid[maxSsidLen+1]={0};
  char key[maxKeyLen+1]={0};
  wifi_easylink_log("step 1:EasyLinkNotify_EasyLinkCompleteHandler");
  
  OSStatus err = kNoErr;
  require_action(nwkpara, exit, err = kTimeoutErr);
  
  //获取 SSID and KEY
  memcpy(ssid, nwkpara->wifi_ssid,maxSsidLen);
  memcpy(key, nwkpara->wifi_key, maxKeyLen);
  
  wifi_easylink_log("Get SSID: %s, Key: %s",ssid,key);
  //可以判断是EasyLink还是AirKiss配网包
  source = (mico_config_source_t)nwkpara->wifi_retry_interval;
  wifi_easylink_log("source =%d",source);
  
exit:
  if( err != kNoErr)
  {
    //EasyLink超时或者出错
    wifi_easylink_log("EasyLink step 1 ERROR, err: %d", err);
    easylink_success = false;
    mico_rtos_set_semaphore(&easylink_sem);    
  }
  return;
}

/* MiCO callback when EasyLink is finished step 2, return extra data 
data format: AuthData#Identifier(localIp/netMask/gateWay/dnsServer) 
Auth data: Provide to application, application will decide if this is a proter configuration for currnet device 
Identifier: Unique id for every easylink instance send by easylink mobile app
localIp/netMask/gateWay/dnsServer: Device static ip address, use DHCP if not exist 
*/
static void EasyLinkNotify_EasyLinkGetExtraDataHandler(int datalen, char* data, mico_Context_t * const inContext)
{
  wifi_easylink_log("step2:EasyLinkNotify_EasyLinkGetExtraDataHandler");
  
  OSStatus err = kNoErr;
  int index ;
  uint32_t *identifier, ipInfoCount;
  char *debugString;
  
  //require_action(inContext, exit, err = kParamErr);
  
  /*如果是微信的AirKiss包，比较简单
  会获得一个无符号8位的随机数
  */
  if( source == CONFIG_BY_AIRKISS){
    airkiss_random = *data;
    goto exit;
  }
  /*如果是庆科的EasyLink包，比较复杂
  逐个解析
  */
  
  debugString = DataToHexStringWithSpaces( ( const uint8_t *)data,datalen );
  wifi_easylink_log("Get user info: %s", debugString);
  free(debugString);
  
  // Find '#' that seperate anthdata and identifier
  for(index = datalen - 1; index>=0; index-- ){
    if(data[index] == '#' &&( (datalen - index) == 5 || (datalen - index) == 25 ) )
      break;
  }
  require_action(index >= 0, exit, err = kParamErr);
  
  //Check auth data by device 
  data[index++] = 0x0;
  err = mico_system_delegate_config_recv_auth_data( data );
  require_noerr(err, exit);
  
  // Read identifier 
  identifier = (uint32_t *)&data[index];
  easylinkIndentifier = *identifier;
  
  //Identifier: 1 x uint32_t or Identifier/localIp/netMask/gateWay/dnsServer: 5 x uint32_t 
  ipInfoCount = (datalen - index)/sizeof(uint32_t);
  require_action(ipInfoCount >= 1, exit, err = kParamErr);
  
  if(ipInfoCount == 1)
  { 
    //Use DHCP to obtain local ip address
    wifi_easylink_log("DHCP:Get auth info: %s, EasyLink identifier: %x", data, easylinkIndentifier);
  }
  else
  {
    wifi_easylink_log("Use Static IP address");
    //Use static ip address 
    bool			dhcpEnable;
    char			localIp[maxIpLen+1]  ={0};
    char			netMask[maxIpLen+1]  ={0};
    char			gateWay[maxIpLen+1]  ={0};
    char			dnsServer[maxIpLen+1]={0};
    dhcpEnable = false;
    //app想让设备静态获取ip地址
    inet_ntoa(localIp,   *(identifier+1));
    inet_ntoa(netMask,   *(identifier+2));
    inet_ntoa(gateWay,   *(identifier+3));
    inet_ntoa(dnsServer, *(identifier+4));
    
    wifi_easylink_log("Get auth info: %s, EasyLink identifier: %x", data, \
      easylinkIndentifier);
    wifi_easylink_log("local IP info:%s", localIp);
    wifi_easylink_log("netMask info:%s", netMask);
    wifi_easylink_log("gateWay info:%s", gateWay);
    wifi_easylink_log("dnsServer info:%s", dnsServer);
  }
  source = CONFIG_BY_EASYLINK_V2;
  
exit:
  if( err != kNoErr)
  {
    //EasyLink error    
    system_log("EasyLink step 2 ERROR, err: %d", err);
    easylink_success = false;
  }
  else
  {
    // Easylink success after step 1 and step 2 
    easylink_success = true;
  }
  mico_rtos_set_semaphore(&easylink_sem);    
  return;
}



int application_start( void )
{
  
  OSStatus err = kNoErr;
  wifi_easylink_log("wifi easylink api");
  
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
  
  wifi_easylink_log("please comment macro MICO_WLAN_CONNECTION_ENABLE");
  
#else
  
  //注册用户函数，当连接状态改变时响应
  err = mico_system_notify_register( mico_notify_EASYLINK_WPS_COMPLETED, (void *)EasyLinkNotify_EasyLinkCompleteHandler, NULL );
  require_noerr( err, exit ); 
  //注册用户函数，当无线连接失败
  err = mico_system_notify_register( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)EasyLinkNotify_EasyLinkGetExtraDataHandler, NULL );
  require_noerr( err, exit );
  
  mico_rtos_init_semaphore(&easylink_sem, 1);
  micoWlanStartEasyLinkPlus(30);//EasyLink_TimeOut/1000
  wifi_easylink_log("entering easylink combo mode,please wait");
  mico_rtos_get_semaphore(&easylink_sem, MICO_WAIT_FOREVER);
  if(easylink_success == true)
  {
    wifi_easylink_log("wifi easylink success");
  }
  else
  {
    wifi_easylink_log("wifi easylink failed");
  }
  
#endif
  
exit:
  mico_rtos_delete_thread(NULL);
  return err;
}

