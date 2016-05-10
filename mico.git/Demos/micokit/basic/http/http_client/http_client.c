/**
******************************************************************************
* @file    http_client.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   http 客户端例子演示怎么从服务器获取数据
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a 
copy 
*  of this software and associated documentation files (the "Software"), to 
deal
*  in the Software without restriction, including without limitation the 
rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is 
furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
******************************************************************************
*/

#include "MICO.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"
#include "StringUtils.h"

#define http_client_log(M, ...) custom_log("HTTP", M, ##__VA_ARGS__)

//接收服务器数据
static OSStatus onReceivedData(struct _HTTPHeader_t * httpHeader, 
                               uint32_t pos, 
                               uint8_t *data, 
                               size_t len,
                               void * userContext);
//清理结构体数据
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext );


//服务器返回的数据结构体，含有数据内容和数据长度
typedef struct _http_response_data{
  char *content;
  uint64_t content_length;
} http_response_data;

//http 例子
void simple_http_get( char* host, char* query );

//另开辟一个线程运行https，http+ssl例子线程
void https_client_thread(void *arg);
void simple_https_get( char* host, char* query );

/*
HTTP 报文格式是面向文本的，报文中的每一个字段
都是一些ASCII码，各个字段的长度是不确定的。HTTP有两种报文:请求报文和响应报文。
请求报文格式:

典型如下：
请求方法+空格+URL+空格+协议版本+回车符+换行符(请求行)
头部字段名+ : + 值+回车符+换行符
...                  ...
头部字段名+ : + 值+回车符+换行符(可选) 
回车符+换行符（格式很重要）
请求数据
*/
/*百度服务器目前支持http和https两种，please ref：  
http://www.admin10000.com/document/6233.html*/
#define BAIDU_HOST "www.baidu.com"
#define BAIDU_SIMPLE_GET_REQUEST \
"GET / HTTP/1.1\r\n" \
"Host:www.baidu.com\r\n" \
"\r\n"

/*火车票查询服务器只支持http，此测试服务器不太稳定，单个IP有访问次数限制*/
#define TRAIN_HOST "webservice.webxml.com.cn"
#define TRAIN_SIMPLE_GET_REQUEST \
"GET /WebServices/TrainTimeWebService.asmx/getStationAndTimeByTrainCode?TrainCode=d11&UserId= HTTP/1.1\r\n" \
"Host:webservice.webxml.com.cn\r\n" \
"\r\n"

/*公积金服务器只支持https*/
#define GJJ_HOST "persons.shgjj.com"
#define GJJ_SIMPLE_GET_REQUEST \
"GET /SsoLogin?url=https://persons.shgjj.com/MainServlet?ID=1 HTTP/1.1\r\n" \
"Host:persons.shgjj.com\r\n" \
"\r\n"

int application_start( void )
{
  OSStatus err = kNoErr;
  //需要进入easylink模式
#ifndef MICO_WLAN_CONNECTION_ENABLE
  http_client_log( "please define macro MICO_WLAN_CONNECTION_ENABLE" );
  goto exit;
#endif
  
  /* Start MiCO system functions according to mico_config.h*/
  err = mico_system_init( mico_system_context_init( 0 ) );
  require_noerr( err, exit ); 
  
  /* 检查链路状态，是否连接上wifi*/
  do
  {
    LinkStatusTypeDef link_status={0};
    err=micoWlanGetLinkStatus(&link_status);
    msleep(100);
    //检查连接是否成功
    if (link_status.is_connected == 1)
      break;
  }while(1);

  /*从baidu服务器读取http数据 */
  simple_http_get(BAIDU_HOST, BAIDU_SIMPLE_GET_REQUEST);
  /*https由于ssl启动需要占用大量内存，所以需要另外开一个线程并且分配大的线程堆栈空间
  application_start堆栈太小(在mico_config.c定义)，运行ssl会overflow*/
  
  //err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "https_client", https_client_thread, 0x3000, NULL );
  //require_noerr_action( err, exit, http_client_log("ERROR: Unable to start the https client thread.") );
    
exit:
  mico_rtos_delete_thread( NULL );
  return err;
}

void https_client_thread(void *arg)
{
  UNUSED_PARAMETER(arg);
  //secure socket layer
  simple_https_get(GJJ_HOST, GJJ_SIMPLE_GET_REQUEST );
  mico_rtos_delete_thread(NULL);
}



void simple_http_get( char* host, char* query )
{
  OSStatus err;
  int client_fd = -1;
  fd_set readfds;
  char ipstr[16];
  struct sockaddr_t addr;
  HTTPHeader_t *httpHeader = NULL;
  http_response_data response = { NULL, 0 };

  //DNS域名解析，解析出域名的ip地址
  while(1)
  {
    err = gethostbyname(host, (uint8_t *)ipstr, 16);//有可能会出错，需要多次解析
    if(err==kNoErr)
    {
      break;
    }
  }
  http_client_log("http server address:%s, ip: %s", host, ipstr);
  
  /*此函数特别重要:分配1024个字节给http头部信息
   并设置一些回调函数，返回动态分配空间的指针
   最重要的是最后面的参数:传一个堆栈指针接收网络数据*/
  httpHeader = HTTPHeaderCreateWithCallback( 1024, onReceivedData, onClearData, &response );
  require_action( httpHeader, exit, err = kNoMemoryErr );
  
  client_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  addr.s_ip = inet_addr( ipstr );//字符型ip转整型ip
  addr.s_port = 80;//http默认是80的tcp端口
  err = connect( client_fd, &addr, sizeof(addr) ); 
  require_noerr_string( err, exit, "connect http server failed" );
  
  /* 发送 符合超文本传输协议的文本格式:
    HTTP get Request请求 ，http响应时间可能会比较长*/
  send( client_fd, query, strlen(query), 0 );
  
  FD_ZERO( &readfds );
  FD_SET( client_fd, &readfds );//加入观察文件描述符集合
  //永远超时等待服务端返回
  select( client_fd + 1, &readfds, NULL, NULL, NULL );
  if( FD_ISSET( client_fd, &readfds ) )
  {
    /*服务端返回数据，先解析http头部*/
    err = SocketReadHTTPHeader( client_fd, httpHeader );
    switch ( err )
    {
    case kNoErr:
      /*打印http返回的头部信息，
	  头部内含有消息体数据情况*/
      PrintHTTPHeader( httpHeader );//打印百度服务器返回的头数据
      err = SocketReadHTTPBody( client_fd, httpHeader );/*获取消息体数据,可能会有多个回调*/
      require_noerr( err, exit );
      /*获取到数据后打印*/
      http_client_log( "Content Data: %s", response.content );
      break;
    case EWOULDBLOCK:
    case kNoSpaceErr:
    case kConnectionErr:
    default:
      http_client_log("ERROR: HTTP Header parse error: %d", err);
      break;
    }
  }
  
exit:
  http_client_log( "Exit: Client exit with err = %d, fd:%d", err, client_fd );
  SocketClose( &client_fd );
  HTTPHeaderDestory( &httpHeader );
}

void simple_https_get( char* host, char* query )
{
  OSStatus err;
  int client_fd = -1;
  int ssl_errno = 0;
  mico_ssl_t client_ssl = NULL;
  fd_set readfds;
  char ipstr[16];
  struct sockaddr_t addr;
  HTTPHeader_t *httpHeader = NULL;
  http_response_data response = { NULL, 0 };//可以传送一个地址过去接收返回数据
  
  //DNS域名解析，解析出域名的ip地址
  while(1)
  {
    err = gethostbyname(host, (uint8_t *)ipstr, 16);//有可能会出错，需要多次解析
    if(err==kNoErr)
    {
      break;
    }
  }
  http_client_log("https server address:%s, ip: %s", host, ipstr);

  //HTTPHeaderCreateWithCallback set some callback functions，传送一个地址过去 
  httpHeader = HTTPHeaderCreateWithCallback( 1024, onReceivedData, onClearData, &response );
  require_action( httpHeader, exit, err = kNoMemoryErr );

  client_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  addr.s_ip = inet_addr( ipstr );
  addr.s_port = 443;//https端口
  err = connect( client_fd, &addr, sizeof(addr) ); 
  require_noerr_string( err, exit, "connect https server failed" );
  
  client_ssl = ssl_connect( client_fd, 0, NULL, &ssl_errno );
  require_string( client_ssl != NULL, exit, "ERROR: ssl disconnect" );
  
  http_client_log( "ssl_connect successfully" );
  //往服务器的443端口发送HTTP Request 
  ssl_send( client_ssl, query, strlen(query) );
  
  FD_ZERO( &readfds );
  FD_SET( client_fd, &readfds );
  
  select( client_fd + 1, &readfds, NULL, NULL, NULL );
  if( FD_ISSET( client_fd, &readfds ) )
  {
    //len=ssl_recv(ssl_active, buf, 1024);//读出来的数据都是有特殊格式的，我们可以采用函数解析
    //parse header
    err = SocketReadHTTPSHeader( client_ssl, httpHeader );
    switch ( err )
    {
    case kNoErr:
      PrintHTTPHeader( httpHeader );
      err = SocketReadHTTPSBody( client_ssl, httpHeader );//get body data
      require_noerr( err, exit );
      //get data and print
      //http_client_log( "Content Data: %s", response.content );
      break;
    case EWOULDBLOCK:
    case kNoSpaceErr:
    case kConnectionErr:
    default:
      http_client_log("ERROR: HTTP Header parse error: %d", err);
      break;
    }
  }
  
exit:
  http_client_log( "Exit: Client exit with err = %d, fd:%d", err, client_fd );
  if( client_ssl ) ssl_close( client_ssl );
  SocketClose( &client_fd );
  HTTPHeaderDestory( &httpHeader );
}

/* 一次网络请求可能会收到多次数据返回*/
static OSStatus onReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext )
{
  OSStatus err = kNoErr;
  http_response_data *context = inUserContext;/*获取传回来的堆栈指针*/
  if( inPos == 0 )
  {
    /*服务器返回消息长度会在头部中有告知
	一次分配好空间，百度返回信息量太大，过于浪费内存
      */
	//context->content = malloc( inHeader->contentLength + 1);
        //memset(context->content, 0x0, inHeader->contentLength + 1);
	//require_action( context->content, exit, err = kNoMemoryErr );
	//context->content_length = inHeader->contentLength;
  }
  //memcpy( context->content + inPos, inData, inLen );
  http_client_log( "response：=%s",inData );/*打印返回数据*/
exit:
  return err;
}

/* Called when HTTPHeaderClear is called */
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext )
{
  UNUSED_PARAMETER( inHeader );
  http_response_data *context = inUserContext;
  if( context->content ) {
    //free( context->content );
    //context->content = NULL;
  }
}



