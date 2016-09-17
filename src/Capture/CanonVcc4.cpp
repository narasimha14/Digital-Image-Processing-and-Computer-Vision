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
#include "CanonVcc4.h"
//#include <afxwin.h>  // CString
#include <assert.h>

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
// IMPORTANT:  The order of these rows must be manually kept in sync with the enum values
//             in the .h file.
// See the bottom of this file for a complete list of the commands, from Canon.
// Each asterisk (*) indicates a byte that must be determined from parameters to the
// command; these asterisks serve no purpose in the code and are here solely for readability.
CanonVcc4Basic::Vcc4CmdBrief CanonVcc4Basic::m_commands[CMD_NCOMMANDS] =
{
  { 3, "\x00\xA0\x30" },  // CMD_POWER_OFF
  { 3, "\x00\xA0\x31" },  // CMD_POWER_ON
  { 2, "\x00\x57" },      // CMD_HOME
  { 2, "\x00\x50***" },   // CMD_SET_PAN_SPEED
  { 2, "\x00\x51***" },   // CMD_SET_TILT_SPEED
  { 3, "\x00\x52\x30" },  // CMD_GET_PAN_SPEED
  { 3, "\x00\x52\x31" },  // CMD_GET_TILT_SPEED
  { 3, "\x00\x53\x30" },  // CMD_STOP_PANTILT
  { 3, "\x00\x53\x31" },  // CMD_PAN_RIGHT
  { 3, "\x00\x53\x32" },  // CMD_PAN_LEFT
  { 3, "\x00\x53\x33" },  // CMD_TILT_UP
  { 3, "\x00\x53\x34" },  // CMD_TILT_DOWN
  { 2, "\x00\x60**" },    // CMD_PAN_TILT
  { 3, "\x00\xA2\x30" },  // CMD_ZOOM_STOP
  { 3, "\x00\xA2\x31" },  // CMD_ZOOM_WIDE
  { 3, "\x00\xA2\x32" },  // CMD_ZOOM_TELE
  { 3, "\x00\xA2\x33" },  // CMD_ZOOM_HI_WIDE
  { 3, "\x00\xA2\x34" },  // CMD_ZOOM_HI_TELE
  { 3, "\x00\xB4\x31*" }, // CMD_SET_ZOOM_SPEED
  { 3, "\x00\xB4\x32" },  // CMD_GET_ZOOM_SPEED
  { 3, "\x00\x90\x30" },  // CMD_HOST_MODE
  { 3, "\x00\x90\x31" },  // CMD_LOCAL_MODE
  { 3, "\x00\x94\x31" },  // CMD_NOTIFY_ON
  { 3, "\x00\x94\x30" },  // CMD_NOTIFY_OFF
  { 2, "\x00\x86" },      // CMD_STATUS_REQUEST
  { 3, "\x00\x86\x30" },  // CMD_EXTENDED_STATUS_REQUEST
};

void CanonVcc4Basic::Vcc4CmdActual::AddByte(char c)
{
  m_str[m_len++] = c;
}

void CanonVcc4Basic::Vcc4CmdActual::AddBytes(const char* c, int nbytes)
{
  memcpy(m_str + m_len, c, nbytes);
  m_len += nbytes;
}

CanonVcc4Basic::Vcc4CmdActual::Vcc4CmdActual(const Vcc4CmdBrief& c)
{
  Clear();
  AddByte( '\xFF' );          // header
  AddByte( '\x30' );          // device number
  AddByte( '\x30' );
  AddBytes( c.str, c.len );   // command
  AddByte( '\xEF' );          // end mark
}

CanonVcc4Basic::Vcc4CmdActual::Vcc4CmdActual(const Vcc4CmdBrief& c, const char* embed_str, int embed_len)
{
  Clear();
  AddByte( '\xFF' );                  // header
  AddByte( '\x30' );                  // device number
  AddByte( '\x30' );
  AddBytes( c.str, c.len );           // command
  AddBytes( embed_str, embed_len );   // parameters
  AddByte( '\xEF' );                  // end mark
}

CanonVcc4Basic::CanonVcc4Basic(unsigned port_number)
{
  DWORD baud_rate = 9600, byte_size = 8, parity = 0, stop_bits = 0;
  COMMTIMEOUTS to;
  to.ReadIntervalTimeout = 0xFFFFFFFF;   // Maximum time between read chars
  to.ReadTotalTimeoutMultiplier = 0;  // Multiplier of characters
  to.ReadTotalTimeoutConstant = 1000;       // Constant in milliseconds
  to.WriteTotalTimeoutMultiplier = 2*CBR_9600 / baud_rate;  // Multiplier of characters
  to.WriteTotalTimeoutConstant = 0;      // Constant in milliseconds

  m_port.Open(port_number);
  if (!m_port.IsCtsOn())
  {
    m_port.Close();
    BLEPO_ERROR("Serial port:  CTS is not on");
  }
  m_port.Configure(baud_rate, byte_size, parity, stop_bits);
  m_port.SetTimeouts(&to);

  m_port.SetDtr();
  m_port.SetRts();
}

CanonVcc4Basic::~CanonVcc4Basic()
{
  m_port.Close();
}

const char* CanonVcc4Basic::GetLastCommandSent(int* len)
{
  assert(len);
  *len = m_last_command_sent.Len();
  return m_last_command_sent.Str();
}

bool CanonVcc4Basic::iSendCommand(const Vcc4CmdActual& cmd)
{
  m_last_command_sent = cmd;
  return m_port.WriteBytes(cmd.Len(), static_cast<void*>( const_cast<char*>( cmd.Str() ) ));
}

bool CanonVcc4Basic::SendCommand(Command cmd_id)
{
  Vcc4CmdActual cmd(m_commands[cmd_id]);
  return iSendCommand(cmd);
}

// converts single hex digit to its ASCII value
// 0 -> 0x30
//   ...
// 9 -> 0x39
// A -> 0x41
//   ...
// F -> 0x46
char HexDigit2AsciiValue(char a)
{
  assert(a>=0x00 && a<=0x0F);
  return (a <= 0x09) ? (a | 0x30) : (a + 0x37);
}

char AsciiValue2HexDigit(char a)
{
  assert(a>=0x30 && a<=0x46);
  assert(a<=0x39 || a>=0x41);
  return (a <= 0x39) ? (a & 0x0F) : (a - 0x37);
}

// Embed the value 'val' as 3 ASCII characters (e.g., the value 380 decimal
// becomes "380" = "\x33\x38\x30") inside the command, and send the command
bool CanonVcc4Basic::SendCommandEmbed3(Command cmd_id, unsigned val)
{
  const int n = 3;
  char p[n];
  p[0] = HexDigit2AsciiValue( ((val & 0xF00) >> 8) );
  p[1] = HexDigit2AsciiValue(((val & 0x0F0) >> 4) );
  p[2] = HexDigit2AsciiValue(((val & 0x00F)     ) );
  Vcc4CmdActual cmd(m_commands[cmd_id], p, n);
  return iSendCommand(cmd);
}

// Embed the value 'val' as 1 ASCII character (e.g., the value 8 decimal
// becomes "8" = "\x38") inside the command, and send the command
bool CanonVcc4Basic::SendCommandEmbed1(Command cmd_id, unsigned val)
{
  const int n = 1;
  char p[n];
  p[0] = HexDigit2AsciiValue( val );
  Vcc4CmdActual cmd(m_commands[cmd_id], p, n);
  return iSendCommand(cmd);
}

bool CanonVcc4Basic::ReadMessage(Array<char>* msg)
{
  msg->Reset(6);
  if (!m_port.ReadBytes(6, msg->Begin()))  return false;
  char c = (*msg)[5];
  while (c != '\xEF')
  {
    if (!m_port.ReadBytes(1, &c))  return false;
    msg->Push(c);
  }
  return true;
}

bool CanonVcc4Basic::IsAnswer(const Array<char>& msg)
{
  if (msg.Len() == 0)  return false;
  return msg[0] == '\xFE';
}

bool CanonVcc4Basic::IsTerminationNotification(const Array<char>& msg)
{
  if (msg.Len() == 0)  return false;
  return msg[0] == '\xFA';
}

CanonVcc4Basic::Return CanonVcc4Basic::DecodeAnswer(const Array<char>& msg)
{
  if (msg.Len() < 6)  return RET_INVALID;
  if (msg[0] != '\xFE')  { return RET_INVALID; }  // something wrong
  if (msg[1] != '\x30')  { return RET_INVALID; }  // something wrong
  if (msg[2] != '\x30')  { return RET_INVALID; }  // something wrong
  if (msg[4] != '\x30')  { return RET_INVALID; }  // something wrong
  switch (msg[3])
  {
  case '\x30':  return RET_SUCCESS;
  case '\x31':  return RET_BUSY;
  case '\x33':  return RET_COMMAND_ERROR;
  case '\x35':  return RET_PARAMETER_ERROR;
  case '\x39':  return RET_MODE_ERROR;
  default:
    return RET_INVALID;  // something wrong
  };
}

bool CanonVcc4Basic::CompareTerminationNotification(const Array<char>& msg, Command cmd_id)
{
  Vcc4CmdBrief& cmd = m_commands[cmd_id];
  char str[] = "\xFA\x30\x30******************************";
  int i;
  for (i=0 ; i<cmd.len ; i++)  str[3+i] = cmd.str[i];
  str[3+i] = '\xEF';

  if (msg.Len() != cmd.len+4)  return false;
  for (i=0 ; i<cmd.len+4 ; i++)  if (str[i] != msg[i])  return false;
  return true;
}

// Note:  Since CMD_EXTENDED_STATUS_REQUEST returns a superset of CMD_STATUS_REQUEST,
// we choose to send it instead.  Alternatively, for some of the parameters one could
// send CMD_STATUS_REQUEST, but there does not appear any reason to do so.
bool CanonVcc4Basic::GetStatus(Status* stat)
{
  PurgeBuffers();
  if (!SendCommand(CMD_EXTENDED_STATUS_REQUEST))  { assert(0);  return false; }
  Array<char> msg;
  if (!ReadMessage(&msg))  { assert(0);  return false; }
  if (!IsAnswer(msg))  { assert(0);  return false; }
  int a = msg.Len();
  if (msg.Len() != 11)  { assert(0);  return false; }
  if (msg[0] != '\xFE')  { assert(0);  return false; }
  if (msg[1] != '\x30')  { assert(0);  return false; }
  if (msg[2] != '\x30')  { assert(0);  return false; }
  if (msg[3] != '\x30')  { assert(0);  return false; }
  if (msg[4] != '\x30')  { assert(0);  return false; }
  char s0 = msg[5] & 0x0F;
  char s1 = msg[6] & 0x0F;
  char s2 = msg[7] & 0x0F;
  char s3 = msg[8] & 0x0F;
  char s4 = msg[9] & 0x0F;
  stat->menuing                           =  ((s0 & 0x08) != 0);  // b19
  stat->host_mode                         = !((s0 & 0x04) != 0);  // b18
  stat->time_set                          = !((s0 & 0x02) != 0);  // b17
  stat->date_set                          = !((s0 & 0x01) != 0);  // b16
//                                          ((s1 & 0x08) != 0);  // b15  (unused)
//                                          ((s1 & 0x04) != 0);  // b14  (unused)
  stat->auto_exposure                     = !((s1 & 0x02) != 0);  // b13
  stat->auto_white_balance                = !((s1 & 0x01) != 0);  // b12
  stat->tilting                           =  ((s2 & 0x08) != 0);  // b11
  stat->tilt_movable_limit_position       =  ((s2 & 0x04) != 0);  // b10
  stat->panning                           =  ((s2 & 0x02) != 0);  // b9
  stat->pan_movable_limit_position        =  ((s2 & 0x01) != 0);  // b8
  stat->zooming                           =  ((s3 & 0x08) != 0);  // b7
  stat->remote_control_on                 = !((s3 & 0x04) != 0);  // b6
  stat->power_on                          = !((s3 & 0x02) != 0);  // b5
  stat->un_executing_pedestal_initialize  =  ((s3 & 0x01) != 0);  // b4
  stat->shutter_speed_flag1               =  ((s4 & 0x08) != 0);  // b3
  stat->shutter_speed_flag2               =  ((s4 & 0x04) != 0);  // b2
  stat->manual_focus_mode                 =  ((s4 & 0x02) != 0);  // b1
  stat->focusing                          =  ((s4 & 0x01) != 0);  // b0
  return true;
}

//bool CanonVcc4Basic::GetExtendedStatus(StatusEx* stat)
//{
//  PurgeBuffers();
//  if (!SendCommand(CMD_EXTENDED_STATUS_REQUEST))  { assert(0);  return false; }
//  std::vector<char> msg;
//  if (!ReadMessage(&msg))  { assert(0);  return false; }
//  if (!IsAnswer(msg))  { assert(0);  return false; }
//  int a = msg.Len();
//  if (msg.Len() != 11)  { assert(0);  return false; }
//  if (msg[0] != '\xFE')  { assert(0);  return false; }
//  if (msg[1] != '\x30')  { assert(0);  return false; }
//  if (msg[2] != '\x30')  { assert(0);  return false; }
//  if (msg[3] != '\x30')  { assert(0);  return false; }
//  if (msg[4] != '\x30')  { assert(0);  return false; }
//  char s0 = msg[5] & 0x0F;
//  char s1 = msg[6] & 0x0F;
//  char s2 = msg[7] & 0x0F;
//  char s3 = msg[8] & 0x0F;
//  char s4 = msg[9] & 0x0F;
//  stat->host_mode = !(s0 & 0x04);
//  // There are several more parameters that we could add here...
//  return true;
//}

//bool CanonVcc4Basic::GetStatus(Status* stat)
//{
//  PurgeBuffers();
//  if (!SendCommand(CMD_STATUS_REQUEST))  { assert(0);  return false; }
//  std::vector<char> msg;
//  if (!ReadMessage(&msg))  { assert(0);  return false; }
//  if (!IsAnswer(msg))  { assert(0);  return false; }
//  int a = msg.Len();
//  if (msg.Len() != 9)  { assert(0);  return false; }
//  if (msg[0] != '\xFE')  { assert(0);  return false; }
//  if (msg[1] != '\x30')  { assert(0);  return false; }
//  if (msg[2] != '\x30')  { assert(0);  return false; }
//  if (msg[3] != '\x30')  { assert(0);  return false; }
//  if (msg[4] != '\x30')  { assert(0);  return false; }
//  char s0 = msg[5] & 0x0F;
//  char s1 = msg[6] & 0x0F;
//  char s2 = msg[7] & 0x0F;
//  stat->focusing =  (s2 & 0x01);
//  stat->zooming  =  (s1 & 0x03);
//  stat->panning  =  (s0 & 0x02);
//  stat->tilting  =  (s0 & 0x08);
//  stat->manual_focus_mode =  (s2 & 0x01);
//  stat->power_on = !(s1 & 0x02);
//  // There are several more parameters that we could add here...
//  return true;
//}

void CanonVcc4Basic::SetPanSpeed(int speed)
{
  assert(speed >= MIN_PAN_SPEED && speed <= MAX_PAN_SPEED);
  if (!SendCommandEmbed3(CMD_SET_PAN_SPEED, speed)) { assert(0); }
//  std::vector<char> msg;
//  if (!ReadMessage(&msg)) { assert(0); }
//  if (!IsAnswer(msg)) { assert(0); }
//  Return ans = DecodeAnswer(msg);
//  if (ans != RET_SUCCESS && ans != RET_BUSY)  { assert(0); } 
}

int CanonVcc4Basic::GetPanSpeed()
{
  PurgeBuffers();
  if (!SendCommand(CMD_GET_PAN_SPEED))  { assert(0); }
  Array<char> msg;
  if (!ReadMessage(&msg))  { assert(0);  return -1; }
  if (!IsAnswer(msg))  { assert(0);  return -1; }
  int a = msg.Len();
  if (msg.Len() != 9)  { assert(0);  return -1; }
  if (msg[0] != '\xFE')  { assert(0);  return -1; }
  if (msg[1] != '\x30')  { assert(0);  return -1; }
  if (msg[2] != '\x30')  { assert(0);  return -1; }
  if (msg[3] != '\x30')  { assert(0);  return -1; }
  if (msg[4] != '\x30')  { assert(0);  return -1; }
  char s0 = AsciiValue2HexDigit( msg[5] );
  char s1 = AsciiValue2HexDigit( msg[6] );
  char s2 = AsciiValue2HexDigit( msg[7] );
  int speed = (s0 << 8) | (s1 << 4) | (s2);
  assert(speed >= MIN_PAN_SPEED && speed <= MAX_PAN_SPEED);
  return speed;
}


void CanonVcc4Basic::SetTiltSpeed(int speed)
{
  assert(speed >= MIN_TILT_SPEED && speed <= MAX_TILT_SPEED);
  if (!SendCommandEmbed3(CMD_SET_TILT_SPEED, speed)) { assert(0); }
//  std::vector<char> msg;
//  if (!ReadMessage(&msg)) { assert(0); }
//  if (!IsAnswer(msg)) { assert(0); }
}

int CanonVcc4Basic::GetTiltSpeed()
{
  PurgeBuffers();
  if (!SendCommand(CMD_GET_TILT_SPEED))  { assert(0); }
  Array<char> msg;
  if (!ReadMessage(&msg))  { assert(0);  return -1; }
  if (!IsAnswer(msg))  { assert(0);  return -1; }
  int a = msg.Len();
  if (msg.Len() != 9)  { assert(0);  return -1; }
  if (msg[0] != '\xFE')  { assert(0);  return -1; }
  if (msg[1] != '\x30')  { assert(0);  return -1; }
  if (msg[2] != '\x30')  { assert(0);  return -1; }
  if (msg[3] != '\x30')  { assert(0);  return -1; }
  if (msg[4] != '\x30')  { assert(0);  return -1; }
  char s0 = AsciiValue2HexDigit( msg[5] );
  char s1 = AsciiValue2HexDigit( msg[6] );
  char s2 = AsciiValue2HexDigit( msg[7] );
  int speed = (s0 << 8) | (s1 << 4) | (s2);
  assert(speed >= MIN_TILT_SPEED && speed <= MAX_TILT_SPEED);
  return speed;
}

void CanonVcc4Basic::SetZoomSpeed(int speed)
{
  assert(speed >= MIN_ZOOM_SPEED && speed <= MAX_ZOOM_SPEED);
  if (!SendCommandEmbed1(CMD_SET_ZOOM_SPEED, speed)) { assert(0); }
//  std::vector<char> msg;
//  if (!ReadMessage(&msg)) { assert(0); }
//  if (!IsAnswer(msg)) { assert(0); }
}

int CanonVcc4Basic::GetZoomSpeed()
{
  PurgeBuffers();
  if (!SendCommand(CMD_GET_ZOOM_SPEED))  { assert(0); }
  Array<char> msg;
  if (!ReadMessage(&msg))  { assert(0);  return -1; }
  if (!IsAnswer(msg))  { assert(0);  return -1; }
  int a = msg.Len();
  if (msg.Len() != 7)  { assert(0);  return -1; }
  if (msg[0] != '\xFE')  { assert(0);  return -1; }
  if (msg[1] != '\x30')  { assert(0);  return -1; }
  if (msg[2] != '\x30')  { assert(0);  return -1; }
  if (msg[3] != '\x30')  { assert(0);  return -1; }
  if (msg[4] != '\x30')  { assert(0);  return -1; }
  char speed = AsciiValue2HexDigit( msg[5] );
  assert(speed >= MIN_ZOOM_SPEED && speed <= MAX_ZOOM_SPEED);
  return speed;
}


void CanonVcc4Basic::PanTilt(int pan, int tilt)
{
  const int n = 2;
  char p[n];
  p[0] = (pan<0) ? 0x32 : ((pan>0) ? 0x31 : 0x30);
  p[1] = (tilt<0) ? 0x31 : ((tilt>0) ? 0x32 : 0x30);
  Vcc4CmdActual cmd(m_commands[CMD_PAN_TILT], p, n);
  if (!iSendCommand(cmd))  { assert(0); }
//  std::vector<char> msg;
//  if (!ReadMessage(&msg)) { assert(0); }
//  if (!IsAnswer(msg)) { assert(0); }
//  Return ans = DecodeAnswer(msg);
//  if (ans != RET_SUCCESS && ans != RET_BUSY)  { assert(0); } 
}



};  // end namespace blepo


/* From Canon's example code vcc4.cpp:


VC4CMD VC4CmdConst::command[VCC4_CMDMAX] = {
{ 2, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA0','\x30','\xEF'      }, // set : power off
{ 3, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA0','\x31','\xEF'      }, // set : poewr on
{ 4, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA1','\x30','\xEF'      }, // set : focus mode AF
{ 5, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA1','\x31','\xEF'      }, // set : focus mode manual
{ 6, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA1','\x32','\xEF'      }, // set : focus near
{ 7, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA1','\x33','\xEF'      }, // set : focus far
{ 8, 1, 10 ,'\xFF','\x30','\x30','\x00','\xB0','*','*','*','*','\xEF' }, // set : focus position 
{ 9, 0, 7 ,'\xFF','\x30','\x30','\x00','\xB1','\x30','\xEF'      }, // request : focus position
{ 10, 0, 7 ,'\xFF','\x30','\x30','\x00','\xB1','\x31','\xEF'     }, // set : onepush AF
{ 11, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA2','\x30','\xEF'     }, // set : zooming stop
{ 12, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA2','\x31','\xEF'     }, // set : zooming wide
{ 13, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA2','\x32','\xEF'     }, // set : zooming tele
{ 14, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA2','\x33','\xEF'     }, // set : high zooming wide
{ 15, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA2','\x34','\xEF'     }, // set : high zooming tele
{ 16, 1, 8 ,'\xFF','\x30','\x30','\x00','\xA3','*','*','\xEF'    }, // set : zooming position1
{ 17, 0, 6 ,'\xFF','\x30','\x30','\x00','\xA4','\xEF'            }, // request : zooming position1
{ 18, 1, 10 ,'\xFF','\x30','\x30','\x00','\xB3','*','*','*','*','\xEF' }, // set : zooming position2
{ 19, 0, 7 ,'\xFF','\x30','\x30','\x00','\xB4','\x30','\xEF'     }, // request : zooming position2
{ 20, 1, 8 ,'\xFF','\x30','\x30','\x00','\xB4','\x31','*','\xEF' }, // set : zooming speed
{ 21, 0, 7 ,'\xFF','\x30','\x30','\x00','\xB4','\x32','\xEF'     }, // request : zooming speed
{ 22, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x30','\xEF'     }, // set : backlight off
{ 23, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x31','\xEF'     }, // set : backlight on
{ 24, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x32','\xEF'     }, // set : exposed auto
{ 25, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x33','\xEF'     }, // set : exposed manual
{ 26, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA8','\x30','\xEF'     }, // set : shutter speed program
{ 28, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x36','\xEF'     }, // request : shutter speed
{ 29, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA8','\x31','\xEF'     }, // set : shutter speed 1/60(PAL:1/50)
{ 30, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA8','\x32','\xEF'     }, // set : shutter speed 1/100(PAL:1/120)
{ 31, 1, 9 ,'\xFF','\x30','\x30','\x00','\xA5','\x37','*','*','\xEF' }, // set : AGC gain
{ 32, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x38','\xEF'     }, // request : AGC gain
{ 33, 1, 9 ,'\xFF','\x30','\x30','\x00','\xA5','\x39','*','*','\xEF' }, // set : iris
{ 34, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x3A','\xEF'     }, // request : iris
{ 35, 1, 9 ,'\xFF','\x30','\x30','\x00','\xA5','\x3B','*','*','\xEF' }, // set : AE target value
{ 36, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x3C','\xEF'     }, // request : AE target value
{ 37, 1, 8 ,'\xFF','\x30','\x30','\x00','\xA5','\x3D','*','\xEF' }, // set : gain select
{ 38, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA5','\x3E','\xEF'     }, // request : gain select
{ 39, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA7','\x32','\xEF'     }, // set : white balance manual
{ 40, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA7','\x33','\xEF'     }, // set : white balance high speed
{ 41, 1, 9 ,'\xFF','\x30','\x30','\x00','\xA7','\x34','*','*','\xEF' }, // set : white balance manual
{ 42, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA7','\x35','\xEF'     }, // request : white balance
{ 44, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA9','\x30','\xEF'     }, // set : fading mode normal
{ 45, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA9','\x31','\xEF'     }, // set : fading mode white
{ 46, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA9','\x32','\xEF'     }, // set : fading mode high speed white
{ 47, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA9','\x33','\xEF'     }, // set : fading mode high speed black
{ 48, 0, 6 ,'\xFF','\x30','\x30','\x00','\xAA','\xEF'            }, // set : camera reset
{ 49, 0, 6 ,'\xFF','\x30','\x30','\x00','\xAB','\xEF'            }, // request : zooming ratio
{ 50, 0, 6 ,'\xFF','\x30','\x30','\x00','\xAC','\xEF'            }, // request : CCD size
{ 53, 0, 7 ,'\xFF','\x30','\x30','\x00','\xB4','\x33','\xEF'     }, // request : zooming maximum value
{ 57, 0, 7 ,'\xFF','\x30','\x30','\x00','\xBE','\x30','\xEF'    },	// request : camera version
{ 58, 0, 7 ,'\xFF','\x30','\x30','\x00','\xBE','\x31','\xEF'    },	// request : eeprom version
{ 59, 1, 9 ,'\xFF','\x30','\x30','\x00','\x50','*','*','*','\xEF' }, // set : pan motor speed
{ 60, 1, 9 ,'\xFF','\x30','\x30','\x00','\x51','*','*','*','\xEF' }, // set : tilte motor speed
{ 61, 0, 7 ,'\xFF','\x30','\x30','\x00','\x52','\x30','\xEF'     }, // request : pan motor speed
{ 62, 0, 7 ,'\xFF','\x30','\x30','\x00','\x52','\x31','\xEF'     }, // request : tilte motor speed
{ 63, 0, 7 ,'\xFF','\x30','\x30','\x00','\x53','\x30','\xEF'     }, // set : pan/tilte stop
{ 64, 0, 7 ,'\xFF','\x30','\x30','\x00','\x53','\x31','\xEF'     }, // set : pan right start
{ 65, 0, 7 ,'\xFF','\x30','\x30','\x00','\x53','\x32','\xEF'     }, // set : pan left start
{ 66, 0, 7 ,'\xFF','\x30','\x30','\x00','\x53','\x33','\xEF'     }, // set : tilte up start
{ 67, 0, 7 ,'\xFF','\x30','\x30','\x00','\x53','\x34','\xEF'     }, // set : tilte down start
{ 69, 0, 6 ,'\xFF','\x30','\x30','\x00','\x57','\xEF'            }, // set : goto home position
{ 70, 0, 7 ,'\xFF','\x30','\x30','\x00','\x58','\x30','\xEF'     }, // set : pan/tilte motor initilaize1
{ 71, 0, 7 ,'\xFF','\x30','\x30','\x00','\x58','\x31','\xEF'     }, // set : pan/tilte motor initilaize2
{ 72, 0, 7 ,'\xFF','\x30','\x30','\x00','\x59','\x30','\xEF'     }, // request : pan motor minimum speed
{ 73, 0, 7 ,'\xFF','\x30','\x30','\x00','\x59','\x31','\xEF'     }, // request : pan motor maximum speed
{ 74, 0, 7 ,'\xFF','\x30','\x30','\x00','\x59','\x32','\xEF'     }, // request : tilte motor minimum speed
{ 75, 0, 7 ,'\xFF','\x30','\x30','\x00','\x59','\x33','\xEF'     }, // request : tilte motor maximum speed
{ 76, 0, 7 ,'\xFF','\x30','\x30','\x00','\x5B','\x30','\xEF'     }, // request : pan gear ratio
{ 77, 0, 7 ,'\xFF','\x30','\x30','\x00','\x5B','\x31','\xEF'     }, // request : tilte gear ratio
{ 78, 0, 7 ,'\xFF','\x30','\x30','\x00','\x5C','\x30','\xEF'     }, // request : pan motor minimum angle
{ 79, 0, 7 ,'\xFF','\x30','\x30','\x00','\x5C','\x31','\xEF'     }, // request : pan motor maximum angle
{ 80, 0, 7 ,'\xFF','\x30','\x30','\x00','\x5C','\x32','\xEF'     }, // request : tilte motor minimum angle
{ 81, 0, 7 ,'\xFF','\x30','\x30','\x00','\x5C','\x33','\xEF'     }, // request : tilte motor maximum angle
{ 82, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x30','\x30','\xEF' }, // set : pan/tilte stop
{ 83, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x30','\x31','\xEF' }, // set : tilte up start
{ 84, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x30','\x32','\xEF' }, // set : tilte down start
{ 85, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x31','\x30','\xEF' }, // set : pan right start
{ 86, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x32','\x30','\xEF' }, // set : pan left start
{ 87, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x31','\x31','\xEF' }, // set : pan right and tilte up start
{ 88, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x31','\x32','\xEF' }, // set : pan right and tilte down start
{ 89, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x32','\x31','\xEF' }, // set : pan left and tilte up start
{ 90, 0, 8 ,'\xFF','\x30','\x30','\x00','\x60','\x32','\x32','\xEF' }, // set : pan left and tilte down start
{ 92, 2, 14 ,'\xFF','\x30','\x30','\x00','\x62','*','*','*','*','*','*','*','*','\xEF' }, // set : pan/tilte angle
{ 93, 0, 6 ,'\xFF','\x30','\x30','\x00','\x63','\xEF'            }, // request : pan/tilte angle
{ 94, 2, 15 ,'\xFF','\x30','\x30','\x00','\x64','\x30','*','*','*','*','*','*','*','*','\xEF' }, // set : pan movement angle
{ 95, 2, 15 ,'\xFF','\x30','\x30','\x00','\x64','\x31','*','*','*','*','*','*','*','*','\xEF' }, // set : tilte movement angle
{ 96, 0, 7 ,'\xFF','\x30','\x30','\x00','\x65','\x30','\xEF'     }, // request : pan movement angle
{ 97, 0, 7 ,'\xFF','\x30','\x30','\x00','\x65','\x31','\xEF'     }, // request : tilte movement angle
{ 102, 0, 7 ,'\xFF','\x30','\x30','\x00','\x80','\x30','\xEF'    }, // set : remote command on
{ 103, 0, 7 ,'\xFF','\x30','\x30','\x00','\x80','\x31','\xEF'    }, // set : remote command off
{ 104, 0, 6 ,'\xFF','\x30','\x30','\x00','\x86','\xEF'           }, // request : movement status
{ 106, 0, 6 ,'\xFF','\x30','\x30','\x00','\x87','\xEF'           }, // request : unit name
{ 107, 0, 6 ,'\xFF','\x30','\x30','\x00','\x88','\xEF'           }, // request : rom version
{ 108, 1, 7 ,'\xFF','\x30','\x30','\x00','\x89','*','\xEF'       }, // set : preset memory
{ 109, 1, 7 ,'\xFF','\x30','\x30','\x00','\x8A','*','\xEF'       }, // set : movement preset memory
{ 110, 0, 6 ,'\xFF','\x30','\x30','\x00','\x8B','\xEF'           }, // request : preset status
{ 113, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8D','\x30','\xEF'    }, // set : remote command pass on
{ 114, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8D','\x31','\xEF'    }, // set : remote command pass off
{ 118, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8F','\x30','\xEF'    }, // set : cascade on
{ 119, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8F','\x31','\xEF'    }, // set : cascade off
{ 120, 0, 7 ,'\xFF','\x30','\x30','\x00','\x90','\x30','\xEF'    }, // set : host mode
{ 121, 0, 7 ,'\xFF','\x30','\x30','\x00','\x90','\x31','\xEF'    }, // set local mode
{ 122, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x30','\xEF' }, // set : onscreen off
{ 123, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x31','\xEF' }, // set : onscreen on
{ 124, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x32','\xEF' }, // set : screen title display off
{ 125, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x33','\xEF' }, // set : screen title display on
{ 126, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x34','\xEF' }, // set : screen time display off
{ 127, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x35','\xEF' }, // set : screen time display on (mode1)
{ 128, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x36','\xEF' }, // set : screen time display on (mode2)
{ 129, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x37','\xEF' }, // set : screen date display off
{ 130, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x38','\xEF' }, // set : screen date display on (mode1)
{ 131, 0, 8 ,'\xFF','\x30','\x30','\x00','\x91','\x30','\x39','\xEF' }, // set : screen date display on (mode2)
{ 132, 3, 12 ,'\xFF','\x30','\x30','\x00','\x91','\x31','*','*','*','*','*','\xEF' }, // set : screen title
{ 133, 2, 10 ,'\xFF','\x30','\x30','\x00','\x91','\x32', '*','*','*','\xEF'    }, // request : screen title
{ 134, 3, 13 ,'\xFF','\x30','\x30','\x00','\x91','\x33','*','*','*','*','*','*','\xEF' }, // set : screen date
{ 135, 0, 7 ,'\xFF','\x30','\x30','\x00','\x91','\x34','\xEF'    }, // request : screen date
{ 136, 3, 13 ,'\xFF','\x30','\x30','\x00','\x91','\x35','*','*','*','*','*','*','\xEF' }, // set : screen time
{ 137, 0, 7 ,'\xFF','\x30','\x30','\x00','\x91','\x36','\xEF'    }, // request : screen time
{ 138, 0, 7 ,'\xFF','\x30','\x30','\x00','\x92','\x30','\xEF'    }, // request : camera power on time
{ 139, 0, 7 ,'\xFF','\x30','\x30','\x00','\x92','\x31','\xEF'    }, // request : pedestal power on time
{ 140, 0, 7 ,'\xFF','\x30','\x30','\x00','\x93','\x30','\xEF'    }, // set : default reset
{ 147, 0, 7 ,'\xFF','\x30','\x30','\x00','\x86','\x30','\xEF'    }, // request : extend movement status
{ 148, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8B','\x30','\xEF'    }, // request : extend preset status
{ 149, 1, 9 ,'\xFF','\x30','\x30','\x00','\xA5','\x35','*','*','\xEF' }, // set : shutter speed
{ 150, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA7','\x30','\xEF'    }, // set : white balance normal
{ 151, 0, 7 ,'\xFF','\x30','\x30','\x00','\xA7','\x31','\xEF'    }, // set : white balance lock
{ 152, 0, 7 ,'\xFF','\x30','\x30','\x00','\xB1','\x32','\xEF'    }, // request : focus range
{ 155, 0, 7 ,'\xFF','\x30','\x30','\x00','\x94','\x30','\xEF'    }, // set : notify command off
{ 156, 0, 7 ,'\xFF','\x30','\x30','\x00','\x94','\x31','\xEF'    }, // set : notify command on
{ 157, 0, 7 ,'\xFF','\x30','\x30','\x00','\x95','\x30','\xEF'    }, // set : cascade global notify off
{ 158, 0, 7 ,'\xFF','\x30','\x30','\x00','\x95','\x31','\xEF'    }, // set : cascade global notify on
{ 159, 0, 8 ,'\xFF','\x30','\x30','\x00','\xA5','\x34','\x30','\xEF' }, // set : AE lock off
{ 160, 0, 8 ,'\xFF','\x30','\x30','\x00','\xA5','\x34','\x31','\xEF' }, // set : AE lock on
{ 164, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8E','\x30','\xEF'    }, // set : LED normal
{ 165, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8E','\x31','\xEF'    }, // set : LED green on
{ 166, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8E','\x32','\xEF'    }, // set : LED all off 
{ 167, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8E','\x33','\xEF'    }, // set : LED red on
{ 168, 0, 7 ,'\xFF','\x30','\x30','\x00','\x8E','\x34','\xEF'    }, // set : LED orange on
{ 170, 0, 7 ,'\xFF','\x30','\x30','\x00','\x9A','\x30','\xEF'    }, // request : pedestal model
{ 171, 0, 7 ,'\xFF','\x30','\x30','\x00','\x9A','\x31','\xEF'    }  // request : camera model
};
*/