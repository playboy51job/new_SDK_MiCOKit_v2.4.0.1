/**
******************************************************************************
* @file    dns.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   Get the IP address from a host name.(DNS)
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

#define dns_log(M, ...) custom_log("DNS", M, ##__VA_ARGS__)


static char *domain = "www.baidu.com";


int application_start( void )
{
  OSStatus err = kNoErr;
  char ipstr[16];
  //��Ҫ����easylinkģʽ

  #ifndef MICO_WLAN_CONNECTION_ENABLE
    dns_log( "please define macro MICO_WLAN_CONNECTION_ENABLE" );
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

  
  /* ����DNS��ַ ���˺���������*/
  dns_log( "resolve dns address" );
  err = gethostbyname( domain, (uint8_t *)ipstr, 16 );
  require_noerr( err, exit );
  dns_log( "%s ip address is %s",domain, ipstr );
  
exit:  
  mico_rtos_delete_thread( NULL );
  return err;
}

