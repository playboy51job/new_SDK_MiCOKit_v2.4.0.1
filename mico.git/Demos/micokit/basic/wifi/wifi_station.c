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

/*����·����ssid��keyword��Ϣ��ָ����ʽ����*/
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
  case NOTIFY_STATION_UP://���ӳɹ�
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
      ����mico_config.h 
      1�����������MICO_WLAN_CONNECTION_ENABLE��,
       ����mico_system_init�������˺����������һϵ�к�����
       ��������ϵͳ�� ��Ƶ�ĳ�ʼ����EasyLink��ע���첽
       �¼���EasyLink��ť,�־û��洢��
      2����������壬ȫ�������Լ��ֶ�д
   */
  mico_system_init( mico_system_context_init( 0 ) );
#ifdef MICO_WLAN_CONNECTION_ENABLE 
   //�մ��룬easylink�����豸Stationģʽ
  wifi_station_log("entering easylink mode");
#else
  //ע���û�������������״̬�ı�ʱ��Ӧ
  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler, NULL );
  require_noerr( err, exit ); 
  //ע���û�����������������ʧ��
  err = mico_system_notify_register( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler, NULL );
  require_noerr( err, exit );
  
  //��ʼ�����߲���
  strcpy((char*)wNetConfigAdv.ap_info.ssid, ap_ssid);//wlan ssid�ַ���
  strcpy((char*)wNetConfigAdv.key, ap_key);          // wlan key �ַ���
  wNetConfigAdv.key_len = strlen(ap_key);            // wlan key ���� *
  wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;//wlan ��ȫģʽ 
  wNetConfigAdv.ap_info.channel = 0;                  // �Զ�ѡ��ͨ��
  wNetConfigAdv.dhcpMode = DHCP_Client;               // ��DHCP �������л�ȡIP��ַ
  wNetConfigAdv.wifi_retry_interval = 100;            //һ������ʧ�ܺ���������
  //��ʼ����......
  wifi_station_log("connecting to %s...", ap_ssid);
  micoWlanStartAdv(&wNetConfigAdv);

#endif

  /* �����·״̬*/
  do
  {
     LinkStatusTypeDef link_status={0};
     err=micoWlanGetLinkStatus(&link_status);
     msleep(100);
	 //��������Ƿ�ɹ�
     if (link_status.is_connected == 1)
       break;
  }while(1);
  
  //��ȡIP ״̬��Ϣ
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

