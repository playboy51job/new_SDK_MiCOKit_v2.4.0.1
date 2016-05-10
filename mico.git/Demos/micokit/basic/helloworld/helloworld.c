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


/*自定义一个宏，能往调试串口打印出模块名字，
时间戳，文件名，行号等信息*/

#define os_helloworld_log(format, ...)  custom_log("helloworld", format, ##__VA_ARGS__)

//应用程序入口
int application_start( void )
{ 
  /* 打印信息到调试串口*/
  OSStatus err=kNoErr;/*操作系统运行状态*/
  /*分配空间*/
  char *buf=(char*)malloc(1000);
  char *data=(char*)malloc(1000);
  int x = 10;
  int selectResult = -2;/*设置这个变量不满足要求，会跳转*/
  os_helloworld_log( "helloworld!" );
  
  /*1:要求表达式为真，否则跳到exit,并且会打印出调试信息
  事实上只有简单的行号，文件和函数名字等信息*/
  require(x,exit);
  os_helloworld_log( "require ok" );
  
  /*2:要求表达式为真，否则跳到exit,不会打印调试信息*/
  require_quiet(buf,exit);
  os_helloworld_log( "require_quiet ok" );
  /*3:要求表达式为真，否则跳到exit,并且会打印出自定义的
  调试信息*/
  require_string( data, exit, "ERROR: malloc failed" );
  os_helloworld_log( "require_string ok" );
  /*4:要求表达式为真，否则跳到LABEL,并且会打印调试信息，
  并执行ACTION语句，此处给出操作系统状态error codes
  */
  
  require_action( selectResult >= 1, exit, err = kFlagErr);/*不满足*/
  
  os_helloworld_log( "require_action ok" );
exit:
  if( err != kNoErr ) os_helloworld_log( "exit with err: %d", err );
  os_helloworld_log("exit now");
  return 1;
}


