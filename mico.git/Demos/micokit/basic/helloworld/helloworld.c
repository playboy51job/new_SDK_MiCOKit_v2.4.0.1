/**
******************************************************************************
* @file    helloworld.c 
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


/*�Զ���һ���꣬�������Դ��ڴ�ӡ��ģ�����֣�
ʱ������ļ������кŵ���Ϣ*/

#define os_helloworld_log(format, ...)  custom_log("helloworld", format, ##__VA_ARGS__)

//Ӧ�ó������
int application_start( void )
{ 
  /* ��ӡ��Ϣ�����Դ���*/
  OSStatus err=kNoErr;/*����ϵͳ����״̬*/
  /*����ռ�*/
  char *buf=(char*)malloc(1000);
  char *data=(char*)malloc(1000);
  int x = 10;
  int selectResult = -2;/*�����������������Ҫ�󣬻���ת*/
  os_helloworld_log( "helloworld!" );
  
  /*1:Ҫ����ʽΪ�棬��������exit,���һ��ӡ��������Ϣ
  ��ʵ��ֻ�м򵥵��кţ��ļ��ͺ������ֵ���Ϣ*/
  require(x,exit);
  os_helloworld_log( "require ok" );
  
  /*2:Ҫ����ʽΪ�棬��������exit,�����ӡ������Ϣ*/
  require_quiet(buf,exit);
  os_helloworld_log( "require_quiet ok" );
  /*3:Ҫ����ʽΪ�棬��������exit,���һ��ӡ���Զ����
  ������Ϣ*/
  require_string( data, exit, "ERROR: malloc failed" );
  os_helloworld_log( "require_string ok" );
  /*4:Ҫ����ʽΪ�棬��������LABEL,���һ��ӡ������Ϣ��
  ��ִ��ACTION��䣬�˴���������ϵͳ״̬error codes
  */
  
  require_action( selectResult >= 1, exit, err = kFlagErr);/*������*/
  
  os_helloworld_log( "require_action ok" );
exit:
  if( err != kNoErr ) os_helloworld_log( "exit with err: %d", err );
  os_helloworld_log("exit now");
  return 1;
}


