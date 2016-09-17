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

#include "Mutex.h"

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

namespace blepo
{


/////////////////////////////////////////////////////////////////////////
// version for Microsoft Windows

#if defined(_WINDOWS) || defined(WINDOWS)

//#include <windows.h>

struct MutexData
{
  MutexData() : m_mutex_handle(NULL) {}
  HANDLE m_mutex_handle;
};

Mutex::Mutex() 
{
  m_mutex_data = new MutexData();
  HANDLE& handle = m_mutex_data->m_mutex_handle;
  handle = CreateMutex(NULL,false,NULL);
  if (handle == NULL)
  {
    BLEPO_ERROR("Unable to create mutex");
  }
}

Mutex::~Mutex()
{
  HANDLE& handle = m_mutex_data->m_mutex_handle;
  int ret = CloseHandle(handle);
  assert(ret);
  delete m_mutex_data;
}

void Mutex::Lock()
{
  DWORD ret;
  HANDLE& handle = m_mutex_data->m_mutex_handle; 
  ret = WaitForSingleObject(handle, INFINITE);
  if (ret != WAIT_FAILED) 
  {
  }
  else
  { 
    BLEPO_ERROR("Unable to lock mutex");
  }     
}

void Mutex::Unlock()
{
  int ret;
  HANDLE& handle = m_mutex_data->m_mutex_handle;
  ret = ReleaseMutex(handle);
  if( ret == 0){ 
    BLEPO_ERROR("Unable to release mutex");
  }
}

// Semaphore

Semaphore::Semaphore() : m_semaphore(NULL)
{
  m_semaphore = ::CreateSemaphore(NULL, 0, INT_MAX, NULL);
  if (m_semaphore == NULL)  BLEPO_ERROR("Unable to create semaphore");
}

Semaphore::~Semaphore()
{
  if (m_semaphore)  ::CloseHandle(m_semaphore);
}

void Semaphore::Signal()
{
  ::ReleaseSemaphore(m_semaphore, 1, NULL);
}

bool Semaphore::Wait(int timeout)
{
  if (timeout < 0)  timeout = INFINITE;
  return WaitForSingleObject(m_semaphore, timeout) == WAIT_OBJECT_0;
}

// Thread

Thread::Thread(bool start_it) : m_handle(NULL)
{
  m_handle = ::CreateThread(NULL, 0, &StaticThreadProc, this, start_it ? 0 : CREATE_SUSPENDED, &m_thread_id);
  if (m_handle == NULL)  BLEPO_ERROR("Unable to create thread");
}

Thread::~Thread()
{
  if (m_handle)  CloseHandle(m_handle);
}

void Thread::Start()
{
  Resume();
}

void Thread::Suspend()
{
  if (m_handle)
  {
    DWORD ret = ::SuspendThread(m_handle);
  }
}

void Thread::Resume()
{
  if (m_handle)
  {
    DWORD ret = ::ResumeThread(m_handle);
    // need a while loop here?
  }
}

DWORD WINAPI Thread::StaticThreadProc(void* param)
{
  // Call function and return the result
  return static_cast<Thread*>( param ) -> MainRoutine();
}


/////////////////////////////////////////////////////////////////////////
// version for Linux

#else // begin posix

#include <pthread.h>

struct MutexData
{
  pthread_mutexattr_t m_mattr;
  int pshared;
  pthread_mutex_t m_mutex;
};

Mutex::Mutex() 
  : m_If_Created(false)
{
  m_mutex_data = new MutexData();
  pthread_mutexattr_t m_mattr = m_mutex_data->m_mattr;
  int pshared = m_mutex_data->pshared;
  int ret = pthread_mutexattr_init(&m_mattr);  
  if(ret != 0)
  {
    BLEPO_ERROR("Unable to initialize mutex");
  }
  pthread_mutex_t m_mutex = m_mutex_data->m_mutex;
}

Mutex::~Mutex()
{
  pthread_mutex_t m_mutex = m_mutex_data->m_mutex;
  int ret = pthread_mutex_destroy(&m_mutex);  
  if (ret != 0)
  {
    BLEPO_ERROR("Unable to destroy mutex");
  }
  delete m_mutex_data;
}

void Mutex::Lock()
{
  pthread_mutex_t m_mutex = m_mutex_data->m_mutex;
  int ret = pthread_mutex_lock(&m_mutex);

  if (ret != 0)
  {
    BLEPO_ERROR("Unable to lock mutex");
  }
} 
 
void Mutex::Unlock() 
{
  pthread_mutex_t m_mutex = m_mutex_data->m_mutex;
  int ret = pthread_mutex_unlock(&m_mutex);
  if (ret != 0)
  {
    BLEPO_ERROR("Unable to unlock mutex");
  }
}

#endif           
        
};  // end namespace blepo

