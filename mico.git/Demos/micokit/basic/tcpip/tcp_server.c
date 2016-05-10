/**
******************************************************************************
* @file    tcp_server.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   kit start a tcp server,echo demo
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
#include "SocketUtils.h"

#define tcp_server_log(M, ...) custom_log("TCP", M, ##__VA_ARGS__)

#define SERVER_PORT 60000 /*set up a tcp server,port at 60000*/

void tcp_client_thread(void *arg)
{
  OSStatus err = kNoErr;
  int fd = (int)arg;
  int len = 0;
  char *buf = NULL; 
  struct timeval_t t;
  
  buf=(char*)malloc(1024);
  require_action(buf, exit, err = kNoMemoryErr); 
  
  while(1)
  {
      memset(buf,0,1024);
      len = recv( fd, buf, 1024, 0 );//如果没有数据，此函数会阻塞
      require_action( len >= 0, exit, err = kConnectionErr );
      
      if( len == 0){
        tcp_server_log( "TCP Client is disconnected, fd: %d", fd );
        goto exit;
      }  
      
      tcp_server_log("fd: %d, recv data %d from client", fd, len);
      len = send( fd, buf, len, 0 );
      tcp_server_log("fd: %d, send data %d to client", fd, len);
  }
 
exit:
  if( err != kNoErr ) tcp_server_log( "TCP client thread exit with err: %d", err );
  if( buf != NULL ) free( buf );
  SocketClose( &fd );
  mico_rtos_delete_thread( NULL );
}

/* TCP server 监听线程 */
void tcp_server_thread( void *arg )
{
  OSStatus err = kNoErr;
  struct sockaddr_t server_addr,client_addr;
  socklen_t sockaddr_t_size = sizeof( client_addr );
  char  client_ip_str[16];
  int tcp_listen_fd = -1, client_fd = -1;
  fd_set readfds;
  
  tcp_listen_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  require_action(IsValidSocket( tcp_listen_fd ), exit, err = kNoResourcesErr );
  
  server_addr.s_ip =  INADDR_ANY;  /* Accept conenction request on all network interface */
  server_addr.s_port = SERVER_PORT;/* Server listen on port: 60000 */
  err = bind( tcp_listen_fd, &server_addr, sizeof( server_addr ) );
  require_noerr( err, exit );
  
  err = listen( tcp_listen_fd, 0);
  require_noerr( err, exit );
  
  while(1)
  {
    FD_ZERO( &readfds );
    FD_SET( tcp_listen_fd, &readfds );
    
    require( select(tcp_listen_fd+1, &readfds, NULL, NULL, NULL) >= 0, exit );
    
    if(FD_ISSET(tcp_listen_fd, &readfds))
    {
        //只要产生一个连接就开辟一个工作者线程
        client_fd = accept( tcp_listen_fd, &client_addr, &sockaddr_t_size );
        if( IsValidSocket( client_fd ) ) 
        {
            inet_ntoa( client_ip_str, client_addr.s_ip );
            tcp_server_log( "TCP Client %s:%d connected, fd: %d", client_ip_str, client_addr.s_port, client_fd );
            if ( kNoErr !=  mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "TCP Clients", tcp_client_thread, 0x800, (void *)client_fd ) )
              SocketClose( &client_fd );      
        }
    }
  }
exit:
  if( err != kNoErr ) tcp_server_log( "Server listerner thread exit with err: %d", err );
  SocketClose( &tcp_listen_fd );
  mico_rtos_delete_thread(NULL );
}


int application_start( void )
{
  OSStatus err = kNoErr;
  IPStatusTypedef para={0};
   //需要进入easylink模式
  
#ifndef MICO_WLAN_CONNECTION_ENABLE
    tcp_server_log( "please define macro MICO_WLAN_CONNECTION_ENABLE" );
    goto exit;
#endif
  
  /* Start MiCO system functions according to mico_config.h*/
  err = mico_system_init( mico_system_context_init( 0 ) );
  require_noerr( err, exit ); 
  
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
  
  //将server的ip和port打印出来，这样客户端就可以根据ip&port连接服务器了
  micoWlanGetIPStatus(&para, Station);
  tcp_server_log("server's ip is %s ,port is %d",para.ip,SERVER_PORT);

  
  /* Start TCP server listener thread*/
  err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "TCP_server", tcp_server_thread, 0x800, NULL );
  require_noerr_string( err, exit, "ERROR: Unable to start the tcp server thread." );
  
exit:
  mico_rtos_delete_thread( NULL );
  return err;
}

