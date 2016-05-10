/**
******************************************************************************
* @file    http_client.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   http �ͻ���������ʾ��ô�ӷ�������ȡ����
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

//���շ���������
static OSStatus onReceivedData(struct _HTTPHeader_t * httpHeader, 
                               uint32_t pos, 
                               uint8_t *data, 
                               size_t len,
                               void * userContext);
//����ṹ������
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext );


//���������ص����ݽṹ�壬�����������ݺ����ݳ���
typedef struct _http_response_data{
  char *content;
  uint64_t content_length;
} http_response_data;

//http ����
void simple_http_get( char* host, char* query );

//����һ���߳�����https��http+ssl�����߳�
void https_client_thread(void *arg);
void simple_https_get( char* host, char* query );

/*
HTTP ���ĸ�ʽ�������ı��ģ������е�ÿһ���ֶ�
����һЩASCII�룬�����ֶεĳ����ǲ�ȷ���ġ�HTTP�����ֱ���:�����ĺ���Ӧ���ġ�
�����ĸ�ʽ:

�������£�
���󷽷�+�ո�+URL+�ո�+Э��汾+�س���+���з�(������)
ͷ���ֶ���+ : + ֵ+�س���+���з�
...                  ...
ͷ���ֶ���+ : + ֵ+�س���+���з�(��ѡ) 
�س���+���з�����ʽ����Ҫ��
��������
*/
/*�ٶȷ�����Ŀǰ֧��http��https���֣�please ref��  
http://www.admin10000.com/document/6233.html*/
#define BAIDU_HOST "www.baidu.com"
#define BAIDU_SIMPLE_GET_REQUEST \
"GET / HTTP/1.1\r\n" \
"Host:www.baidu.com\r\n" \
"\r\n"

/*��Ʊ��ѯ������ֻ֧��http���˲��Է�������̫�ȶ�������IP�з��ʴ�������*/
#define TRAIN_HOST "webservice.webxml.com.cn"
#define TRAIN_SIMPLE_GET_REQUEST \
"GET /WebServices/TrainTimeWebService.asmx/getStationAndTimeByTrainCode?TrainCode=d11&UserId= HTTP/1.1\r\n" \
"Host:webservice.webxml.com.cn\r\n" \
"\r\n"

/*�����������ֻ֧��https*/
#define GJJ_HOST "persons.shgjj.com"
#define GJJ_SIMPLE_GET_REQUEST \
"GET /SsoLogin?url=https://persons.shgjj.com/MainServlet?ID=1 HTTP/1.1\r\n" \
"Host:persons.shgjj.com\r\n" \
"\r\n"

int application_start( void )
{
  OSStatus err = kNoErr;
  //��Ҫ����easylinkģʽ
#ifndef MICO_WLAN_CONNECTION_ENABLE
  http_client_log( "please define macro MICO_WLAN_CONNECTION_ENABLE" );
  goto exit;
#endif
  
  /* Start MiCO system functions according to mico_config.h*/
  err = mico_system_init( mico_system_context_init( 0 ) );
  require_noerr( err, exit ); 
  
  /* �����·״̬���Ƿ�������wifi*/
  do
  {
    LinkStatusTypeDef link_status={0};
    err=micoWlanGetLinkStatus(&link_status);
    msleep(100);
    //��������Ƿ�ɹ�
    if (link_status.is_connected == 1)
      break;
  }while(1);

  /*��baidu��������ȡhttp���� */
  simple_http_get(BAIDU_HOST, BAIDU_SIMPLE_GET_REQUEST);
  /*https����ssl������Ҫռ�ô����ڴ棬������Ҫ���⿪һ���̲߳��ҷ������̶߳�ջ�ռ�
  application_start��ջ̫С(��mico_config.c����)������ssl��overflow*/
  
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

  //DNS����������������������ip��ַ
  while(1)
  {
    err = gethostbyname(host, (uint8_t *)ipstr, 16);//�п��ܻ������Ҫ��ν���
    if(err==kNoErr)
    {
      break;
    }
  }
  http_client_log("http server address:%s, ip: %s", host, ipstr);
  
  /*�˺����ر���Ҫ:����1024���ֽڸ�httpͷ����Ϣ
   ������һЩ�ص����������ض�̬����ռ��ָ��
   ����Ҫ���������Ĳ���:��һ����ջָ�������������*/
  httpHeader = HTTPHeaderCreateWithCallback( 1024, onReceivedData, onClearData, &response );
  require_action( httpHeader, exit, err = kNoMemoryErr );
  
  client_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  addr.s_ip = inet_addr( ipstr );//�ַ���ipת����ip
  addr.s_port = 80;//httpĬ����80��tcp�˿�
  err = connect( client_fd, &addr, sizeof(addr) ); 
  require_noerr_string( err, exit, "connect http server failed" );
  
  /* ���� ���ϳ��ı�����Э����ı���ʽ:
    HTTP get Request���� ��http��Ӧʱ����ܻ�Ƚϳ�*/
  send( client_fd, query, strlen(query), 0 );
  
  FD_ZERO( &readfds );
  FD_SET( client_fd, &readfds );//����۲��ļ�����������
  //��Զ��ʱ�ȴ�����˷���
  select( client_fd + 1, &readfds, NULL, NULL, NULL );
  if( FD_ISSET( client_fd, &readfds ) )
  {
    /*����˷������ݣ��Ƚ���httpͷ��*/
    err = SocketReadHTTPHeader( client_fd, httpHeader );
    switch ( err )
    {
    case kNoErr:
      /*��ӡhttp���ص�ͷ����Ϣ��
	  ͷ���ں�����Ϣ���������*/
      PrintHTTPHeader( httpHeader );//��ӡ�ٶȷ��������ص�ͷ����
      err = SocketReadHTTPBody( client_fd, httpHeader );/*��ȡ��Ϣ������,���ܻ��ж���ص�*/
      require_noerr( err, exit );
      /*��ȡ�����ݺ��ӡ*/
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
  http_response_data response = { NULL, 0 };//���Դ���һ����ַ��ȥ���շ�������
  
  //DNS����������������������ip��ַ
  while(1)
  {
    err = gethostbyname(host, (uint8_t *)ipstr, 16);//�п��ܻ������Ҫ��ν���
    if(err==kNoErr)
    {
      break;
    }
  }
  http_client_log("https server address:%s, ip: %s", host, ipstr);

  //HTTPHeaderCreateWithCallback set some callback functions������һ����ַ��ȥ 
  httpHeader = HTTPHeaderCreateWithCallback( 1024, onReceivedData, onClearData, &response );
  require_action( httpHeader, exit, err = kNoMemoryErr );

  client_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  addr.s_ip = inet_addr( ipstr );
  addr.s_port = 443;//https�˿�
  err = connect( client_fd, &addr, sizeof(addr) ); 
  require_noerr_string( err, exit, "connect https server failed" );
  
  client_ssl = ssl_connect( client_fd, 0, NULL, &ssl_errno );
  require_string( client_ssl != NULL, exit, "ERROR: ssl disconnect" );
  
  http_client_log( "ssl_connect successfully" );
  //����������443�˿ڷ���HTTP Request 
  ssl_send( client_ssl, query, strlen(query) );
  
  FD_ZERO( &readfds );
  FD_SET( client_fd, &readfds );
  
  select( client_fd + 1, &readfds, NULL, NULL, NULL );
  if( FD_ISSET( client_fd, &readfds ) )
  {
    //len=ssl_recv(ssl_active, buf, 1024);//�����������ݶ����������ʽ�ģ����ǿ��Բ��ú�������
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

/* һ������������ܻ��յ�������ݷ���*/
static OSStatus onReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext )
{
  OSStatus err = kNoErr;
  http_response_data *context = inUserContext;/*��ȡ�������Ķ�ջָ��*/
  if( inPos == 0 )
  {
    /*������������Ϣ���Ȼ���ͷ�����и�֪
	һ�η���ÿռ䣬�ٶȷ�����Ϣ��̫�󣬹����˷��ڴ�
      */
	//context->content = malloc( inHeader->contentLength + 1);
        //memset(context->content, 0x0, inHeader->contentLength + 1);
	//require_action( context->content, exit, err = kNoMemoryErr );
	//context->content_length = inHeader->contentLength;
  }
  //memcpy( context->content + inPos, inData, inLen );
  http_client_log( "response��=%s",inData );/*��ӡ��������*/
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



