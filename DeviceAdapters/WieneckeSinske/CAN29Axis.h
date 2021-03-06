///////////////////////////////////////////////////////////////////////////////
// FILE:          CAN29Axis.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Zeiss CAN29 axis 
//             
//
// AUTHOR:        S3L GmbH, info@s3l.de, www.s3l.de,  11/21/2017
// COPYRIGHT:     S3L GmbH, Rosdorf, 2017
// LICENSE:       This library is free software; you can redistribute it and/or
//                modify it under the terms of the GNU Lesser General Public
//                License as published by the Free Software Foundation.
//                
//                You should have received a copy of the GNU Lesser General Public
//                License along with the source distribution; if not, write to
//                the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
//                Boston, MA  02111-1307  USA
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.  
//
#ifndef _CAN29AXIS_H_
#define _CAN29AXIS_H_

#include "CAN29.h"

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
#define ERR_UNKNOWN_POSITION         10002
#define ERR_INVALID_SPEED            10003
#define ERR_PORT_CHANGE_FORBIDDEN    10004                                   
#define ERR_SET_POSITION_FAILED      10005                                   
#define ERR_INVALID_STEP_SIZE        10006                                   
#define ERR_LOW_LEVEL_MODE_FAILED    10007                                   
#define ERR_INVALID_MODE             10008 
#define ERR_DEVICE_NOT_ACTIVE        10012 


class CAN29Axis: CAN29Component
{
public:
	CAN29Axis(CAN29UByte canAddress, CAN29UByte devID, CAN29* can29):
	  CAN29Component(canAddress, devID, can29){};

	  ~CAN29Axis(){};

	  int Initialize();
	  int UnInitialize();

	  int ReceiveMessageHandler(Message& msg);

	  int GetApplicationName(std::string& applName);
	  int GetStatusCmd(CAN29ULong& status);
	  int GetStatus(CAN29ULong& status) 
	  { 
		  status = actStatus_;
		  return DEVICE_OK;
	  };

	  bool IsBusy() {return isBusy_;};

	  int GetPresent(bool& present, std::string expectedApplName);
	  int GetPositionCmd(CAN29Long& position);
	  int GetPosition(CAN29Long& position)
	  { 
		  position = actPosition_;
		  return DEVICE_OK;
	  };

	  int SetPosition(CAN29Long position, CAN29Byte movemode);
	  int SetRelativePosition(CAN29Long position, CAN29Byte movemode);
	  int Stop();

	  int Lock();
	  int Unlock();

	  int GetLowerHardwareStop(CAN29Long& position);
	  int GetUpperHardwareStop(CAN29Long& position);
	  int FindLowerHardwareStop();
	  int FindUpperHardwareStop();

      int SetTrajectoryVelocity(CAN29Long velocity);
      int SetTrajectoryAcceleration(CAN29Long acceleration);
      int GetTrajectoryVelocity(CAN29Long& velocity);
      int GetTrajectoryAcceleration(CAN29Long& acceleration);

	  
protected:
	int StartMonitoring();
	int StopMonitoring();

private:
	CAN29UByte moveMode_;

	CAN29ULong actStatus_;
	CAN29Long actPosition_;
	bool isBusy_;
};

#endif // _CAN29AXIS_H_
