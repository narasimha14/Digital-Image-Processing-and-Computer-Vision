/* 
 * Copyright (c) 2005 Clemson University.
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

#include "Utilities/Exception.h"
#include "SerialPort.h"

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

void SerialPort::Open(unsigned port_number)
{
  char name[100];
  wsprintf(name, "COM%d", port_number);
  m_h = CreateFile(
          name,                           // port name
          GENERIC_READ | GENERIC_WRITE,   // access
          0,                              // share
          0,                              // security
          OPEN_EXISTING,                  // creation disposition
          0,                              // flags
          0);                             // template file
  if (m_h == INVALID_HANDLE_VALUE)
  {
    BLEPO_ERROR(StringEx("Serial port:  Cannot open COM%d for exclusive read/write access", port_number));
  }
  m_open = true;
}

void SerialPort::Close()
{
  if (m_open)
  {
    CloseHandle(m_h);
    m_open = false;
  }
}

  /// Configure the port
void SerialPort::Configure(unsigned baud_rate, unsigned byte_size, unsigned parity, unsigned stop_bits)
{
  DCB dcb;
  BOOL ret;

  // get comm state
  ret = GetCommState(m_h, &dcb);
  if (ret == 0)  BLEPO_ERROR("Serial port");


  // configure comm state
  dcb.BaudRate = baud_rate;
  dcb.fBinary = TRUE;
  dcb.fParity = TRUE;
  dcb.fOutxCtsFlow = TRUE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fOutX = FALSE;
  dcb.fInX = FALSE;
  dcb.fNull = FALSE;
  dcb.fRtsControl = RTS_CONTROL_ENABLE ;
  dcb.ByteSize = byte_size;
  dcb.Parity = parity;
  dcb.StopBits = stop_bits;

  // set comm state
  ret = SetCommState(m_h, &dcb);
  if (ret == 0)  BLEPO_ERROR("Serial port error");
}

  /// Set timeouts
void SerialPort::SetTimeouts(COMMTIMEOUTS* to)
{
  BOOL ret = SetCommTimeouts(m_h, to);
  if (ret == 0)  BLEPO_ERROR("Serial port error");
}

// return true for success
bool SerialPort::WriteBytes(int nbytes, void* p)
{
  unsigned long nwritten;
  BOOL ret = WriteFile(m_h, p, nbytes, &nwritten, NULL);
  return (ret != 0 && nwritten == static_cast<unsigned long>(nbytes));
}

// return true for success
bool SerialPort::ReadBytes(int nbytes, void* p)
{
  unsigned long nread;
  BOOL ret = ReadFile(m_h, p, nbytes, &nread, NULL);
  return (ret != 0 && nread == static_cast<unsigned long>(nbytes));
}

bool SerialPort::WriteBytes(const Array<char>& buff) 
{ 
  return WriteBytes(buff.Len(), const_cast<char*>( buff.Begin() ));
}
  
bool SerialPort::ReadBytes(int nbytes, Array<char>* buff)
{ 
  buff->Reset(nbytes);
  return ReadBytes(buff->Len(), buff->Begin());
}

void SerialPort::PurgeBuffers()
{
  // purge any information in the buffers
  BOOL ret = PurgeComm( m_h, PURGE_TXABORT | PURGE_RXABORT |
                             PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
  if (ret == 0)  BLEPO_ERROR("Serial port error");
}

void SerialPort::SetDtr()
{
  EscapeCommFunction( m_h, SETDTR );
}

void SerialPort::SetRts()
{
  EscapeCommFunction( m_h, SETRTS );
}

bool SerialPort::IsCtsOn()
{
  DWORD modem_status;
  GetCommModemStatus(m_h, &modem_status);
  return (modem_status & MS_CTS_ON) != 0;
}


};  // end namespace blepo
