/**
******************************************************************************
* @file    os_timer.c 
* @author  William Xu
* @version V1.0.0
* @date    21-May-2015
* @brief   MiCO RTOS ��ʱ������
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

#define os_timer_log(M, ...) custom_log("OS", M, ##__VA_ARGS__)

mico_timer_t timer_handle;/*��ʱ�����*/
void alarm( void* arg );

/*1������һ�Σ���10�������ٸö�ʱ��*/
void alarm( void* arg )
{
  int* count = (int*)arg;/*��ȡ���ݵĵ�ַ*/
  os_timer_log("time is coming,value = %d", (*count)++ );

  if( *count == 10 )
  {
      mico_stop_timer( &timer_handle );
      mico_deinit_timer( &timer_handle );
      os_timer_log("timer stop and deinit");
  }
}

int application_start( void )
{
  OSStatus err = kNoErr;
  os_timer_log("timer demo");
  int arg = 0;

  /*��ʼ��һ��1���ӵĶ�ʱ���������ô�������alarm*/
  err = mico_init_timer(&timer_handle, 1000, alarm, &arg);
  require_noerr(err, exit);

  /*��ʼ��ʱ*/
  err = mico_start_timer(&timer_handle);
  require_noerr(err, exit);

  /*ע�⣺�˶�ʱ�����ӣ�application�̲߳����˳�*/
  mico_thread_sleep( MICO_NEVER_TIMEOUT );

exit:
  mico_rtos_delete_thread( NULL );
  return err;
}




