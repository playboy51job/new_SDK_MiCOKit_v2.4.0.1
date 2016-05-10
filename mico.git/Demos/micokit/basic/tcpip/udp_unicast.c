/**
******************************************************************************
* @file    udp_unicast.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   ʵ�ֵ�Ե��ͨ�ţ�UDP A�ͻ��˺�UDP B�ͻ��ˣ�
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

#define udp_unicast_log(M, ...) custom_log("UDP", M, ##__VA_ARGS__)

#define LOCAL_UDP_PORT 60000

/*create udp socket*/
void udp_unicast_thread(void *arg)
{
  UNUSED_PARAMETER(arg);

  OSStatus err;
  struct sockaddr_t addr;
  fd_set readfds;
  socklen_t addrLen = sizeof(addr);
  int udp_fd = -1 , len;
  char ip_address[16];
  uint8_t *buf = NULL;
  IPStatusTypedef para={0};

  buf = malloc(1024);
  require_action(buf, exit, err = kNoMemoryErr);
  
  /*Establish a UDP port to receive any data sent to this port*/
  udp_fd = socket( AF_INET, SOCK_DGRM, IPPROTO_UDP );
  require_action( IsValidSocket( udp_fd ), exit, err = kNoResourcesErr );

  addr.s_ip = INADDR_ANY;/*local ip address*/
  addr.s_port = LOCAL_UDP_PORT;/*60000*/
  err = bind( udp_fd, &addr, sizeof(addr) );
  require_noerr( err, exit );

  //��A�ͻ��˵� UDP socket��ip��port��ӡ����������B�ͻ��˾Ϳ��Ը���ip&port����������
  micoWlanGetIPStatus(&para, Station);
  udp_unicast_log("kit's udp ip is %s ,port is %d",para.ip,LOCAL_UDP_PORT);

  while(1)
  {
    FD_ZERO(&readfds);
    FD_SET(udp_fd, &readfds);

    require_action( select(udp_fd + 1, &readfds, NULL, NULL, NULL) >= 0, exit, err = kConnectionErr );

    /*Read data from udp and send data back */ 
    if (FD_ISSET( udp_fd, &readfds )) 
    {
      /*ע�⣬��ʱaddr�ᱩ¶��B�ͻ��˵�ip��port��
      Ҫ��B�ͻ��˽���UDP client������A�ͻ��˵�IP�Ͷ˿ڼ���
      */
      len = recvfrom(udp_fd, buf, 1024, 0, &addr, &addrLen);
      require_action( len >= 0, exit, err = kConnectionErr );

      inet_ntoa( ip_address, addr.s_ip );
      udp_unicast_log( "udp recv from %s:%d, len:%d", ip_address,addr.s_port, len );
      sendto( udp_fd, buf, len, 0, &addr, sizeof(struct sockaddr_t) );
    }
  }
  
exit:
  if( buf != NULL ) free(buf);
  mico_rtos_delete_thread(NULL);
}

int application_start( void )
{
  OSStatus err = kNoErr;
  //��Ҫ����easylinkģʽ

#ifndef MICO_WLAN_CONNECTION_ENABLE
    udp_unicast_log( "please define macro MICO_WLAN_CONNECTION_ENABLE" );
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
  

  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "udp_unicast", udp_unicast_thread, 0x800, NULL );
  require_noerr_string( err, exit, "ERROR: Unable to start the UDP thread." );
  
exit:
  mico_rtos_delete_thread( NULL );
  return err;
}

