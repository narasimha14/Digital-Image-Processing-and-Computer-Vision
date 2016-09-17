/* 
 * Copyright (c) 2004 Clemson University.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __BLEPO_MUTEX_H__
#define __BLEPO_MUTEX_H__

#include "Exception.h"
//#include <windows.h>

/**
@class Mutex

  Basic mutex class, for Windows and Linux operating systems.

@author Shrinivas Pundlik (SJP)
*/

namespace blepo
{

class Mutex
{
public:
  Mutex();    
  ~Mutex();

  /// Locks the mutex 
  /// (i.e., waits until the mutex becomes free and then grabs it)
  void Lock();

  /// Unlocks the mutex
  /// (I.e., releases it so others can grab it)
  void Unlock();

private:
  struct MutexData* m_mutex_data;
};  

class AutoMutex  
{
public:
  AutoMutex(Mutex* mutex) : m_mutex(mutex) {  m_mutex->Lock(); }
  ~AutoMutex() { m_mutex->Unlock(); }
private:
  Mutex* m_mutex;
};

class Semaphore
{
public:
  Semaphore();
  ~Semaphore();
  // signal semaphore
  void Signal();
  // returns true if semaphore is signaled, false otherwise; 
  // timeout is in milliseconds (set to negative value for infinite)
  bool Wait(int timeout = -1);
private:
  HANDLE m_semaphore;
};

class Thread
{
public:
  // Create a thread in suspended mode.
  // @param start_it  If true, then thread starts immediately.  Same as
  //                  calling Start() just after default constructor.  
  Thread(bool start_it = false);
  ~Thread();

  void Start();
  void Suspend();
  void Resume();

protected:
  // This is the main routine for the thread.  Override this method
  // to perform the desired functionality and to 
  // return a value indicating success or failure.
  virtual unsigned long MainRoutine() = 0;

private:
  // static method used by class to initialize call to MainRoutine.
  static DWORD WINAPI StaticThreadProc(void* param);

  HANDLE m_handle;
  unsigned long m_thread_id;
};

};  // end namespace blepo

#endif //__BLEPO_MUTEX_H__
