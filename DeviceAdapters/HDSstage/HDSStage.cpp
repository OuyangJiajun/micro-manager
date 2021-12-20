///////////////////////////////////////////////////////////////////////////////
// FILE:          HDS Stage.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   HDS XYStage device adapter.
//                
// AUTHOR:        OUYANG JIAJUN, 9/12/2021
//

#ifdef WIN32
#include <windows.h>
#endif
#include "FixSnprintf.h"

#include "HDSStage.h"
#include "ModuleInterface.h"
#include "DeviceBase.h"
#include "DeviceUtils.h"

#include <cstdio>
#include <math.h>

// XYStage
const char* g_XYStageDeviceName = "XYStage";

const char* g_SutterStage_Reset = "Reset";
const char* g_SutterStage_TransmissionDelay = "TransmissionDelay";
const char* g_SutterStage_Axis_Id = "SutterStageSingleAxisName";

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
   RegisterDevice(g_XYStageDeviceName, MM::XYStageDevice, "XY Stage");
}                                                                            
                                                                             
MODULE_API MM::Device* CreateDevice(const char* deviceName)                  
{
   if (deviceName == 0)
      return 0;

   if (strcmp(deviceName, g_XYStageDeviceName) == 0)                         
   {                                                                         
      XYStage* pXYStage = new XYStage();                                     
      return pXYStage;                                                       
   }

   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

/*
 * Global Utility function for communication with the controller
 */
int clearPort(MM::Device& device, MM::Core& core, const char* port)
{
   // Clear contents of serial port
   const unsigned int bufSize = 255;
   unsigned char clear[bufSize];
   unsigned long read = bufSize;
   int ret;
   while (read == bufSize)
   {
      ret = core.ReadFromSerial(&device, port, clear, bufSize, read);//本质上就是每次从串口缓存读255字符，一直到读不够255个才停止循环，此时串口缓存已经清空
      if (ret != DEVICE_OK)
         return ret;
   }
   return DEVICE_OK;
}

XYStage::XYStage() :
   initialized_(false), 
   resolution_(2519.6851), 
   position_x(0),
   position_y(0),
   originX_(0),
   originY_(0)
{
   InitializeDefaultErrorMessages();

   // create pre-initialization properties
   // ------------------------------------
   // NOTE: pre-initialization properties contain parameters which must be defined fo

   // Name, read-only (RO)
   CreateProperty(MM::g_Keyword_Name, g_XYStageDeviceName, MM::String, true);

   // Description, RO
   CreateProperty(MM::g_Keyword_Description, "HDS XYstage driver adapter", MM::String, true);

   // Port:
   CPropertyAction* pAct = new CPropertyAction(this, &XYStage::OnPort);
   CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
}

XYStage::~XYStage()
{
   Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
// XYStage methods required by the API
///////////////////////////////////////////////////////////////////////////////

void XYStage::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_XYStageDeviceName);
}

/**
 * Performs device initialization.
 * Additional properties can be defined here too.
 */
int XYStage::Initialize()
{
    //Resolution, RO
    int ret = CreateProperty("Resolution", "2519.6851", MM::Float, true);

    //位置属性
    CPropertyAction* pAct = new CPropertyAction (this, &XYStage::OnXPosition);
    ret = CreateProperty("X Position", "0", MM::Float, false, pAct);
    if (ret != DEVICE_OK)
       return ret;
    SetPropertyLimits("X Position", -57, 57);

    pAct = new CPropertyAction(this, &XYStage::OnYPosition);
    ret = CreateProperty("Y Position", "0", MM::Float, false, pAct);
    if (ret != DEVICE_OK)
       return ret;
    SetPropertyLimits("Y Position", -62, 64);
    
    //相对位置属性
    pAct = new CPropertyAction(this, &XYStage::OnRelativelyXPosition);
    ret = CreateProperty("Relatively X Position", "0", MM::Float, false, pAct);
    if (ret != DEVICE_OK)
        return ret;
    SetPropertyLimits("Relatively X Position", -114, 114);

    pAct = new CPropertyAction(this, &XYStage::OnRelativelyYPosition);
    ret = CreateProperty("Relatively Y Position", "0", MM::Float, false, pAct);
    if (ret != DEVICE_OK)
        return ret;
    SetPropertyLimits("Relatively Y Position", -126, 126);

    Home();//复位
    initialized_ = true;
    return DEVICE_OK;
}

int XYStage::Shutdown()
{
   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

bool XYStage::Busy()
{
   if (AxisBusy("0") || AxisBusy("1"))
      return true;
   return false;
}

/**
 * Returns true if any axis (X or Y) is still moving.
 */
bool XYStage::AxisBusy(const char* axis)
{
    clearPort(*this, *GetCoreCallback(), port_.c_str());
   // format the command
   ostringstream cmd;
   cmd << "?MTYPE(" << axis << ")";
   string status;
   // send command
   int ret = ExecuteCommand(cmd.str().c_str(), status);
   if (ret != DEVICE_OK)
   {
      ostringstream os;
      os << "ExecuteCommand failed in XYStage::Busy, error code:" << ret;
      this->LogMessage(os.str().c_str(), false);
      // return false; // can't write, continue just so that we can read an answer in case write succeeded even though we received an error
   }
   if (status[0] == '0')
      return false;
   else
      return true;
}


/**
 * Sets position in steps.
 */
int XYStage::SetPositionSteps(long x, long y)
{
   // format the command
   ostringstream cmd;
   cmd << "base(0) moveabs(" << x << "*presx)"; 
   int ret = SendSerialCommand(port_.c_str(), cmd.str().c_str(), "");
   if (ret != DEVICE_OK)
      return ret;
   cmd.str("");//清空
   cmd << "base(1) moveabs(" << y << "*presx)";
   ret = SendSerialCommand(port_.c_str(), cmd.str().c_str(), "");
   if (ret != DEVICE_OK)
       return ret;
   return DEVICE_OK;
}

/**
 * Returns current position in steps.
 */
int XYStage::GetPositionSteps(long& x, long& y)
{
   string resp;
   int ret = ExecuteCommand("?DPOS(0)", resp);
   if (ret != DEVICE_OK)
      return ret;
   position_x = atof(resp.c_str())/resolution_;//resp字符串要变为double型，然后再除分辨率转为mm
   x=position_x;
   ret = ExecuteCommand("?DPOS(1)", resp);
   if (ret != DEVICE_OK)
      return ret;
   position_y = atof(resp.c_str())/resolution_;
   y=position_y;
   return DEVICE_OK;
}

//Defines current position as origin (0,0) coordinate of the controller.
 
int XYStage::SetOrigin()
{
   PurgeComPort(port_.c_str());
   long x, y;
   GetPositionSteps(x,y);
   originX_=x;
   originY_=y;
   return DEVICE_OK;
}

int XYStage::Home()
{
   PurgeComPort(port_.c_str());
   string cmd = "RUNTASK 1,HOMEX";
   int ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "");
   if (ret != DEVICE_OK)
      return ret;
   cmd = "RUNTASK 1,HOMEY";
   ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "");
   if (ret != DEVICE_OK)
       return ret;
   return DEVICE_OK;
}

int XYStage::Stop()
{
   PurgeComPort(port_.c_str());
   const char* cmd = "RAPIDSTOP(2)";
   int ret = SendSerialCommand(port_.c_str(), cmd, "");
   if (ret != DEVICE_OK)
      return ret;
   return DEVICE_OK;
}

/**
 * Returns the stage position limits in um.
 */
int XYStage::GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax)
{
    xMin = -57000;
    xMax = 57000;
    yMin = -62000;
    yMax = 64000;
   return DEVICE_OK;
}

int XYStage::GetStepLimits(long& xMin, long& xMax, long& yMin, long& yMax)
{
    xMin = -57000;
    xMax = 57000;
    yMin = -62000;
    yMax = 64000;
    return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Internal, helper methods
///////////////////////////////////////////////////////////////////////////////

/**
 * Sends a specified command to the controller
 */
int XYStage::ExecuteCommand(const string& cmd, string& response)
{
   // send command
   clearPort(*this, *GetCoreCallback(), port_.c_str());
   if (DEVICE_OK != SendSerialCommand(port_.c_str(), cmd.c_str(), "\r\n"))
      return DEVICE_SERIAL_COMMAND_FAILED;
   GetSerialAnswer(port_.c_str(), "\n", response);//这一步啥都没接到，或者online command line not ended over time
   /*
   unsigned char resp[255];
   unsigned long read = 0;
   int ret = ReadFromComPort(port_.c_str(), resp, 255, read);//返回字符没有超过255的，足够了
   if (ret != DEVICE_OK)
   {
      ostringstream os;
      os << "ReadFromComPort failed in XYStage::ExecuteCommand, error code:" << ret;
      this->LogMessage(os.str().c_str(), false);
      return false; // Error, let's pretend all is fine
   }
   response = (char*) resp;
   */
   if (response.length() < 1)
       return ERR_NO_ANSWER;
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
// Handle changes and updates to property values.
///////////////////////////////////////////////////////////////////////////////

int XYStage::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct)
{
    if (pAct == MM::BeforeGet)
    {
        pProp->Set(port_.c_str());
    }
    else if (pAct == MM::AfterSet)
    {
        if (initialized_)
        {
            pProp->Set(port_.c_str());
            return ERR_PORT_CHANGE_FORBIDDEN;
        }
        pProp->Get(port_);
    }
    return DEVICE_OK;
}

int XYStage::OnXPosition(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      string resp;
      int ret = ExecuteCommand("?DPOS(0)", resp);
      if (ret != DEVICE_OK)
         return ret;
      position_x = atof(resp.c_str())/resolution_;//resp字符串要变为double型，然后再除分辨率转为mm
      pProp->Set(position_x);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(position_x);
      ostringstream os;
      os << "base(0) moveabs(" << position_x << " * presx)";
      string cmd = os.str();
      int ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "");
      if (ret != DEVICE_OK)
         return ret;
   }
   return DEVICE_OK;
}

int XYStage::OnYPosition(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        string resp;
        int ret = ExecuteCommand("?DPOS(1)", resp);
        if (ret != DEVICE_OK)
            return ret;
        position_y = atof(resp.c_str())/resolution_;
        pProp->Set(position_y);
    }
    else if (eAct == MM::AfterSet)
    {
        pProp->Get(position_y);
        ostringstream os;
        os << "base(1) moveabs(" << position_y << " * presx)";
        string cmd = os.str();
        int ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "");
        if (ret != DEVICE_OK)
            return ret;
    }
    return DEVICE_OK;
}

int XYStage::OnRelativelyXPosition(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        string resp;
        int ret = ExecuteCommand("?DPOS(0)", resp);
        if (ret != DEVICE_OK)
            return ret;
        position_x = atof(resp.c_str()) / resolution_;//resp字符串要变为double型，然后再除分辨率转为mm
        relative_position_x = position_x - originX_;//相对坐标可以由绝对坐标减去自定义原点计算得到
        pProp->Set(relative_position_x);
    }
    else if (eAct == MM::AfterSet)
    {
        pProp->Get(relative_position_x);
        position_x = originX_ + relative_position_x;//自定义原点加相对坐标计算得绝对坐标
        ostringstream os;
        os << "base(0) moveabs(" << position_x << " * presx)";//再通过绝对坐标来控制
        string cmd = os.str();
        int ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "");
        if (ret != DEVICE_OK)
            return ret;
    }
    return DEVICE_OK;
}

int XYStage::OnRelativelyYPosition(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        string resp;
        int ret = ExecuteCommand("?DPOS(1)", resp);
        if (ret != DEVICE_OK)
            return ret;
        position_y = atof(resp.c_str()) / resolution_;
        relative_position_y = position_y - originY_;
        pProp->Set(relative_position_y);
    }
    else if (eAct == MM::AfterSet)
    {
        pProp->Get(relative_position_y);
        position_y = originY_ + relative_position_y;
        ostringstream os;
        os << "base(1) moveabs(" << position_y << " * presx)";
        string cmd = os.str();
        int ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "");
        if (ret != DEVICE_OK)
            return ret;
    }
    return DEVICE_OK;
}