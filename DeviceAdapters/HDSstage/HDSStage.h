///////////////////////////////////////////////////////////////////////////////
// FILE:          HDS Stage.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   HDS XYStage device adapter.
//                
// AUTHOR:        OUYANG JIAJUN, 9/12/2021
//
// COPYRIGHT:     University of California, San Francisco, 2007
//

#ifndef _SUTTERSTAGE_H_
#define _SUTTERSTAGE_H_

#include "MMDevice.h"
#include "DeviceBase.h"
#include <string>
#include <map>

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
#define ERR_INVALID_ID               10009
#define ERR_UNRECOGNIZED_ANSWER      10010
#define ERR_INVALID_SHUTTER_STATE    10011
#define ERR_INVALID_SHUTTER_NUMBER   10012
#define ERR_INVALID_COMMAND_LEVEL    10013
#define ERR_MODULE_NOT_FOUND         10014
#define ERR_INVALID_WHEEL_NUMBER     10015
#define ERR_INVALID_WHEEL_POSITION   10016
#define ERR_NO_ANSWER                10017
#define ERR_WHEEL_HOME_FAILED        10018
#define ERR_WHEEL_POSITION_FAILED    10019
#define ERR_SHUTTER_COMMAND_FAILED   10020
#define ERR_COMMAND_FAILED           10021
#define ERR_INVALID_DEVICE_NUMBER    10023
#define ERR_DEVICE_CHANGE_NOT_ALLOWED 10024
#define ERR_SHUTTER_USED             10025
#define ERR_WHEEL_USED               10026
#define ERR_NO_CONTROLLER            10027

// MMCore name of serial port
std::string port_ = "";

int clearPort(MM::Device& device, MM::Core& core, const char* port);

class XYStage : public CXYStageBase<XYStage>
{
public:
   XYStage();
   ~XYStage();
  
   // Device API
   // ----------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const;
   bool Busy();

   // XYStage API
   // -----------
  int SetPositionSteps(long x, long y);
  int GetPositionSteps(long& x, long& y);
  int SetOrigin();
  int Home();
  int Stop();
  int GetStepLimits(long& xMin, long& xMax, long& yMin, long& yMax);
  int GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax);
  double GetStepSizeXUm() {return resolution_;}
  double GetStepSizeYUm() {return resolution_;}
  int IsXYStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}

   // action interface
   // ----------------
   int OnPort(MM::PropertyBase* pProp, MM::ActionType pAct);
   int OnXPosition(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnYPosition(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnRelativelyXPosition(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnRelativelyYPosition(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   bool AxisBusy(const char* axis);
   int ExecuteCommand(const std::string& cmd, std::string& response);
   
   bool initialized_;
   double position_x;
   double position_y;
   double relative_position_x;
   double relative_position_y;
   double resolution_;
   double originX_;
   double originY_;
};
#endif //_SUTTERSTAGE_H_
