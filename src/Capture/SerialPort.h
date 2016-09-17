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

#ifndef __BLEPO_SERIALPORT_H__
#define __BLEPO_SERIALPORT_H__

//#include <vector>
#include <windows.h>  // COMMTIMEOUTS, HANDLE
#include "Utilities/Array.h"

/**
@class SerialPort

Simple class to encapsulate communication over a serial (COM) port.
This class was written for the Canon VCC4 and has not been tested
with other devices. 

@author Stan Birchfield (STB)
*/

namespace blepo
{

class SerialPort
{
public:
  SerialPort() : m_open(false) {}
  ~SerialPort() { Close(); }

  /// Open the port for reading and writing
  /// port_number indicates the com port (e.g., "COM1")
  void Open(unsigned port_number = 1);
  void Close();
  void Configure(unsigned baud_rate, unsigned byte_size, unsigned parity, unsigned stop_bits);
  void SetTimeouts(COMMTIMEOUTS* to);

  // These functions return true for success, false otherwise
  bool WriteBytes(int nbytes, void* p);
  bool ReadBytes(int nbytes, void* p);
  bool WriteBytes(const Array<char>& buff);  
  bool ReadBytes(int nbytes, Array<char>* buff);
  bool WriteByte(char c) { return WriteBytes(1, &c); }
  bool ReadByte(char* c) { return ReadBytes(1, c); }

  // clears all input and output buffers
  void PurgeBuffers();

  // sends the DTR (data-terminal-ready) signal
  void SetDtr();
  // sends the RTS (request-to-send) signal
  void SetRts();
  // returns whether CTS (clear-to-send) signal is on
  bool IsCtsOn();

private:
  HANDLE m_h;
  bool m_open;
};

};  // end namespace blepo

#endif // __BLEPO_SERIALPORT_H__
