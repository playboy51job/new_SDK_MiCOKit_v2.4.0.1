/**
******************************************************************************
* @file    udp_broadcast.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   show kit how to broadcast data
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

#define udp_broadcast_log(M, ...) custom_log("UDP", M, ##__VA_ARGS__)

#define LOCAL_UDP_PORT 60000
#define REMOTE_UDP_PORT 60001


/*create udp socket*/
void udp_broadcast_thread(void *arg)
{
  UNUSED_PARAMETER( arg );

  OSStatus err;
  struct sockaddr_t addr;
  int udp_fd = -1 ;
  
  /*
  ����UDP socket���󶨶˿�Ϊ60000
  */
  udp_fd = socket( AF_INET, SOCK_DGRM, IPPROTO_UDP );
  require_action( IsValidSocket( udp_fd ), exit, err = kNoResourcesErr );
  
  addr.s_ip = INADDR_ANY;
  addr.s_port = LOCAL_UDP_PORT;
  
  err = bind(udp_fd, &addr, sizeof(addr));
  require_noerr( err, exit );
  

  udp_broadcast_log("Start UDP broadcast mode, local port: %d, remote port: %d", LOCAL_UDP_PORT, REMOTE_UDP_PORT);

  while(1)
  {
    char* data = "UDP broadcast data";
    udp_broadcast_log( "broadcast now!" );

    addr.s_ip = INADDR_BROADCAST;
    addr.s_port = REMOTE_UDP_PORT;
    /*
    ע���������˵Ķ˿ڷ��ͣ�����Ҫ������߽���UDP client���Ұ�60001�˿ڲ����յ��㲥����
    ���磺�Է�IP��255.255.255.255
          �Է��˿ڣ�60000
          ���ض˿ڣ�60001
    */
    sendto( udp_fd, data, strlen(data), 0, &addr, sizeof(addr) );

    mico_thread_sleep(2);
  }
  
exit:
  mico_rtos_delete_thread(NULL);
}

int application_start( void )
{
  OSStatus err = kNoErr;
  
 //��Ҫ����easylinkģʽ
#ifndef MICO_WLAN_CONNECTION_ENABLE
    tcp_server_log( "please define macro MICO_WLAN_CONNECTION_ENABLE" );
    goto exit;
#endif
  
  /* Start MiCO system functions according to mico_config.h*/
  err = mico_system_init( mico_system_context_init( 0 ) );
  require_noerr( err, exit ); 
  
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

  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "udp_broadcast", udp_broadcast_thread, 0x800, NULL );
  require_noerr_string( err, exit, "ERROR: Unable to start the UDP thread." );
  
exit:
  mico_rtos_delete_thread( NULL );
  return err;
}

