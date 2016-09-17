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

#ifndef __BLEPO_CANONVCC4_H__
#define __BLEPO_CANONVCC4_H__

//#include <vector>
#include "Utilities/Array.h"
#include "SerialPort.h"

/**
@class CanonVcc4Basic

This class encapsulates the basic interface of controlling the Canon VC-C4 pan-tilt-zoom 
camera via the serial port.  To actually grab the images themselves, use a different class.  

The camera has two modes:
  - Local mode:  camera responds only to remote control; will ignore all commands sent via the serial port
  - Host mode:  camera responds onto to serial port; will ignore the remote control
IMPORTANT:  You must set the camera to host mode, or else it will ignore all commands that you send.

Whenever you send a command to the camera, an "answer" is sent back indicating either 
success or failure (and possible a requested value).  Failure occurs when (1) the camera is 
busy, (2) the camera is in the wrong mode, (3) the command itself is not recognized, or
(4) the parameter (if any) is bad.

There are two types of commands:
  - Synchronous (Type 1):  The command completes execution before the "answer" is sent back.
  - Asynchronous (Type 2):  The command may continue executing after sending back the "answer".
If you set the "termination notification" on, then Asynchronous commands will also send back
a termination notification when their execution is complete.

Note:  Keep in mind that "power on" is a type 2 (asynchronous) command, so the only way to
know when it has completed is to sleep for a couple of seconds and hope it worked, or check
the status repeatedly until it says the power is on, or wait till the notification is 
received (if notifications are turned on).  But you cannot know whether notifications are 
turned on without setting them, and you cannot set them unless the power is on.

Bug:  Right now the class opens the serial port without Overlapped I/O.  Perhaps we
want to change this in the future, if we have a separate thread reading the answers
coming back from the camera.

@author Stan Birchfield (STB)
*/

namespace blepo
{

class CanonVcc4Basic
{
public:
  enum Command 
  // IMPORTANT:  The order of these enum values must be manually kept in sync 
  //             with the command list initializer in the .cpp file
  // Next to each command is listed its type (see above), and the corresponding 
  // page number in the Canon reference manual (external/doc/Canon/Manual516_E.pdf)
  { 
    CMD_POWER_OFF,          // type 2
    CMD_POWER_ON,           // type 2
    CMD_HOME,               // type 2 (p. 30)
    CMD_SET_PAN_SPEED,      // type 1 (p. 21)
    CMD_SET_TILT_SPEED,     // type 1
    CMD_GET_PAN_SPEED,      // type 1
    CMD_GET_TILT_SPEED,     // type 1
    CMD_STOP_PANTILT,       // type 1
    CMD_PAN_RIGHT,          // type 2
    CMD_PAN_LEFT,           // type 2
    CMD_TILT_UP,            // type 2
    CMD_TILT_DOWN,          // type 2
    CMD_PAN_TILT,           // type 2 (p. 44)
    CMD_ZOOM_STOP,          // type 2 (p. 66)
    CMD_ZOOM_WIDE,          // type 2
    CMD_ZOOM_TELE,          // type 2
    CMD_ZOOM_HI_WIDE,       // type 2
    CMD_ZOOM_HI_TELE,       // type 2
    CMD_SET_ZOOM_SPEED,     // type 1
    CMD_GET_ZOOM_SPEED,     // type 1
    CMD_HOST_MODE,          // type 1 (p. 128)
    CMD_LOCAL_MODE,         // type 1
    CMD_NOTIFY_ON,          // type 1 (p. 143)  termination notification (for type 2 asynchronous commands)
    CMD_NOTIFY_OFF,         // type 1
    CMD_STATUS_REQUEST,     // type 1 (p. 113)
    CMD_EXTENDED_STATUS_REQUEST, // type 1 (p. 114)
    CMD_NCOMMANDS           // number of commands; should always be the last item of the list
  };

  /// return code for commands
  enum Return { 
    RET_SUCCESS,          // success; command was sent and received by VCC4 device
    RET_BUSY,             // failure: command was sent, but device was busy
    RET_MODE_ERROR,       // failure: command was sent, but device rejected it 
                          //   (camera is probably not in host mode)
    RET_COMMAND_ERROR,    // failure: command was sent, but device rejected it
                          //   (command was probably sent improperly)
    RET_PARAMETER_ERROR,   // failure: command was sent, but device rejected it
                          //   (command was probably sent improperly)
    RET_INVALID           // invalid message
  };
public:
  /// Opens a serial (COM) port
  /// 'port_number' indicates which COM port, e.g., 1 means COM1
  CanonVcc4Basic(unsigned port_number = 1);

  /// Closes the port
  virtual ~CanonVcc4Basic();

  /// Send a command.
  /// Returns true if the command was sent successfully, false otherwise (WriteFile failed for some reason)
  bool SendCommand(Command cmd_id);

  /// Read a message from the camera.
  /// Performs a blocking read on the COM port.
  /// Returns true if a message was read, false otherwise (ReadFile failed b/c there was no message, etc.)
  bool ReadMessage(Array<char>* msg);

  /// There are two kinds of messages:
  ///    1.  Answer
  ///    2.  Termination Notification
  static bool IsAnswer(const Array<char>& msg);
  static bool IsTerminationNotification(const Array<char>& msg);

  /// Only call this with an answer (not a termination notification)
  Return DecodeAnswer(const Array<char>& msg);

  /// Returns true if the termination notification corresponds to the command
  static bool CompareTerminationNotification(const Array<char>& msg, Command cmd_id);

  struct Status
  {
    bool power_on;
    bool host_mode;
    bool manual_focus_mode;
    bool remote_control_on;
    bool focusing;  // camera is currently in the process of focusing
    bool panning;   // camera is currently in the process of panning
    bool tilting;   // camera is currently in the process of tilting
    bool zooming;   // camera is currently in the process of zooming
    bool menuing;
    bool time_set;
    bool date_set;
    bool auto_exposure;
    bool auto_white_balance;
    bool tilt_movable_limit_position;
    bool pan_movable_limit_position;
    bool un_executing_pedestal_initialize;
    bool shutter_speed_flag1;
    bool shutter_speed_flag2;
  };

  // return true for success
  bool GetStatus(Status* stat);

  // clear the input and output buffers of the serial port
  void PurgeBuffers() { m_port.PurgeBuffers(); }

  // Note:  speed is in the range [MIN_PAN_SPEED, MAX_PAN_SPEED]
  void SetPanSpeed(int speed);
  int GetPanSpeed();

  // Note:  speed is in the range [MIN_TILT_SPEED, MAX_TILT_SPEED]
  void SetTiltSpeed(int speed);
  int GetTiltSpeed();

  // Note:  speed is in the range [MIN_ZOOM_SPEED, MAX_ZOOM_SPEED]
  void SetZoomSpeed(int speed);
  int GetZoomSpeed();

  enum { MIN_PAN_SPEED = 8, MAX_PAN_SPEED = 0x320 };
  enum { MIN_TILT_SPEED = 8, MAX_TILT_SPEED = 0x26E };
  enum { MIN_ZOOM_SPEED = 0, MAX_ZOOM_SPEED = 0x7 };

  // Returns the last command sent; sets 'len' to the length of the string,
  // which is necessary because there may be '\x00' bytes in the string,
  // so that NULL termination cannot be used to determine the length.
  const char* GetLastCommandSent(int* len);

  // start or stop pan and tilt simultaneously.
  // - negative value means pan left / tilt up
  // - zero value means stop pan / stop tilt
  // - positive value means pan right / tilt down
  void PanTilt(int pan, int tilt);

private:
  // stripped down command
  struct Vcc4CmdBrief
  {
    int len;       // length in bytes
    char str[10];  // command string
  };
  // actual command to send / receive
  class Vcc4CmdActual
  {
    int m_len;       // length in bytes
    char m_str[10];  // command string
  public:
    Vcc4CmdActual() : m_len(0) {}
    Vcc4CmdActual(const Vcc4CmdBrief& c);
    Vcc4CmdActual(const Vcc4CmdBrief& c, const char* embed_str, int embed_len);
    void Clear() { m_len = 0; }
    void AddByte(char c);
    void AddBytes(const char* c, int nbytes);
    int Len() const { return m_len; }
    const char* Str() const { return m_str; }
};

  bool SendCommandEmbed1(Command cmd_id, unsigned val);
  bool SendCommandEmbed3(Command cmd_id, unsigned val);
  bool iSendCommand(const Vcc4CmdActual& cmd);

  SerialPort m_port;
  static Vcc4CmdBrief m_commands[CMD_NCOMMANDS];
  Vcc4CmdActual m_last_command_sent;
};

};  // end namespace blepo

#endif //__BLEPO_CANONVCC4_H__
