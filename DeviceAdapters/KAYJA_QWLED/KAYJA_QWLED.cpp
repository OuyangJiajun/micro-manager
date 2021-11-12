///////////////////////////////////////////////////////////////////////////////
// FILE:          QWLED.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   KAYJA QWLED
// NOTICE:        Update micro-manager and install rs232 driver
// AUTHOR:        Ouyang Jiajun 09/11/2021
//

#ifdef WIN32
   #include <windows.h>
#endif
#include "FixSnprintf.h"
#include "KAYJA_QWLED.h"
#include <string>
#include <math.h>
#include "ModuleInterface.h"
#include "MMDevice.h"
#include "DeviceUtils.h"
#include <sstream>
#include <iostream>
#include <fstream>

const char* g_QWLED = "KAYJA QWLED01";

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
    RegisterDevice(g_QWLED, MM::ShutterDevice, "KAYJA QWLED01 Four channel LED Driver");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
    // no name, no device
    if (deviceName == 0)	return 0;
    if (strcmp(deviceName, g_QWLED) == 0)
    {
        return new QWLED;
    }
    return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

// General utility function:
int ClearPort(MM::Device& device, MM::Core& core, std::string port)
{
   // Clear contents of serial port 
   const int bufSize = 255;
   unsigned char clear[bufSize];                      
   unsigned long read = bufSize;
   int ret;                                                                   
   while (read == (unsigned) bufSize) 
   {                                                                     
      ret = core.ReadFromSerial(&device, port.c_str(), clear, bufSize, read);
      if (ret != DEVICE_OK)                               
         return ret;                                               
   }
   return DEVICE_OK;                                                           
} 

/**
* Constructor.
*/
QWLED::QWLED() :
	port_("undefined"),
	initialized_(false),
	m_constCurrent{ 0,0,0,0 }
{
   InitializeDefaultErrorMessages();
   SetErrorText(ERR_PORT_CHANGE_FORBIDDEN, "You can't change the port after device has been initialized.");
   SetErrorText(ERR_ONOFF_CONTROL_FAIL, "Fail to control leds on/off.");
   SetErrorText(ERR_READ_CURRENT_FAIL, "Fail to read current.");
	
   for (int i = 0; i < NUM_LEDS; i++)
   {
	   m_constCurrent[i] = 0;
	   m_SingleLedOnoff[i]="OFF";
   }

   // create pre-initialization properties                                   
   // ------------------------------------                                   
                                                                             
   // Name                                                                   
   CreateProperty(MM::g_Keyword_Name, g_QWLED, MM::String, true); 
                                                                             
   // Description                                                            
   CreateProperty(MM::g_Keyword_Description, "KAYJA QWLED01", MM::String, true);
                                                                             
   // Port                                                               
   CPropertyAction* pAct = new CPropertyAction (this, &QWLED::OnPort);      
   CreateProperty(MM::g_Keyword_Port, "undefined", MM::String, false, pAct, true);  
}                                                                            

QWLED::~QWLED()                                                            
{                                                                            
	Shutdown();                                                               
} 

int QWLED::Initialize()
{
	ostringstream command;
	int nRet;
	std::ostringstream os1;

	if (initialized_)
      return DEVICE_OK;

	CPropertyActionEx* pActEx;
	for (long i = 0; i < NUM_LEDS; i++)
	{
		// set property list
		// -----------------
		// constant current
		pActEx = new CPropertyActionEx(this, &QWLED::OnConstantCurrent, i);
		os1 << "Constant Current LED-" << i + 1;
		nRet = CreateProperty(os1.str().c_str(), "0", MM::Integer, false, pActEx);
		if (nRet != DEVICE_OK) return nRet;
		SetPropertyLimits(os1.str().c_str(), 0, 1000);
		os1.str("");//clear data
		//Single LED ON/OFF control
		pActEx = new CPropertyActionEx(this, &QWLED::OnSingleLedOnoff, i);
		os1 << "ON/OFF LED-" << i + 1;
		nRet = CreateProperty(os1.str().c_str(), "OFF", MM::String, false, pActEx);
		if (nRet != DEVICE_OK) return nRet;
		AddAllowedValue(os1.str().c_str(), "ON");
		AddAllowedValue(os1.str().c_str(), "OFF");
		os1.str("");
	}

	// for safety
	LEDOnOff(false);

   initialized_ = true;

   // init message
   std::ostringstream log;
   log << "QWLED01 - initializied ";
   LogMessage(log.str().c_str());

   return DEVICE_OK;                                                         
}

int QWLED::Shutdown()
{
	if (initialized_)
	{
		LEDOnOff(false);
		initialized_ = false;
	}
	return DEVICE_OK;
}

void QWLED::GetName(char* Name) const
{
	CDeviceUtils::CopyLimitedString(Name, g_QWLED);
}

/*---------------------------------------------------------------------------
 This function sets the LED output.
---------------------------------------------------------------------------*/
int QWLED::SetOpen(bool open)
{
	int val = 0;
	(open) ? val = 1 : val = 0;
	return LEDOnOff(val);
}

/*---------------------------------------------------------------------------
 This function sets the LED output on or off.
---------------------------------------------------------------------------*/
int QWLED::LEDOnOff(int onoff)
{
	int ret = DEVICE_OK;
	std::string answer;
	std::ostringstream command;

	if (onoff == 0)
	{
		command << "@AEFFF";
	}
	else
	{
		command << "@AEFFT";
	}

	ret = SendSerialCommand(port_.c_str(), command.str().c_str(), "\n");
	if (ret != DEVICE_OK) return ret;
	ret = GetSerialAnswer(port_.c_str(), "\n", answer);
	if (ret != DEVICE_OK) return ret;
	if (answer == "@ERR")
	{
		return ERR_ONOFF_CONTROL_FAIL;
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
 //Sets the Serial Port to be used.
 //Should be called before initialization
int QWLED::OnPort(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(port_.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      if (initialized_)
      {
         // revert
         pProp->Set(port_.c_str());
         return ERR_PORT_CHANGE_FORBIDDEN;
      }
                                                                             
      pProp->Get(port_);                                                     
   }                                                                         
   return DEVICE_OK;     
}

/*---------------------------------------------------------------------------
 This function is the callback function for the "Constant Current" property.
---------------------------------------------------------------------------*/
int QWLED::OnConstantCurrent(MM::PropertyBase* pProp, MM::ActionType eAct, long index)
{
	int ret = DEVICE_OK;
	std::string answer;
	std::ostringstream command;

	if (eAct == MM::BeforeGet)
	{
		command << "@AR0" << index + 1;
		ret = SendSerialCommand(port_.c_str(), command.str().c_str(), "\n");
		if (ret != DEVICE_OK) return ret;
		ret = GetSerialAnswer(port_.c_str(), "\n@OK\n", answer);//接收返回信息："@AAAA"或"@ERR"
		if (ret != DEVICE_OK) return ret;
		if (answer == "@ERR")
		{
			return ERR_READ_CURRENT_FAIL;
		}
		answer.erase(0, 1);//删除“@”
		std::istringstream iss(answer);//用answer新建一个istringstream对象iss
		iss >> m_constCurrent[index];//把iss的值（即answer）赋值给m_constCurrent[index]，注意这里的值是QWLED01.h头文件定义的，Micro-Manager并不知道m_constCurrent[index]
		pProp->Set(m_constCurrent[index]);//Micro-Manager同步记录恒定电流值
	}
	else if (eAct == MM::AfterSet)
	{
		// get property
		pProp->Get(m_constCurrent[index]);//从Micro-Manager中获取恒定电流赋值给m_constCurrent[index]
		// prepare command
		std::stringstream ss;
		ss << std::setw(4) << std::setfill('0') << m_constCurrent[index];//把long m_constCurrent[index]转变为四位的字符流
		std::string str;
		ss >> str;         //将字符流传给 str
		command << "@AW0" << index + 1 << str;
		// send command
		ret = SendSerialCommand(port_.c_str(), command.str().c_str(), "\n");
		if (ret != DEVICE_OK) return ret;
		ret = GetSerialAnswer(port_.c_str(), "\n", answer);//接收返回信息："@OK"或"@ERR"
		if (ret != DEVICE_OK) return ret;
		if (answer == "@ERR")
		{
			return ERR_READ_CURRENT_FAIL;
		}
	}

	return ret;
}

/*---------------------------------------------------------------------------
 This function is the callback function for the "ON/OFF LED" property.
---------------------------------------------------------------------------*/
int QWLED::OnSingleLedOnoff(MM::PropertyBase* pProp, MM::ActionType eAct, long index)
{
	int ret = DEVICE_OK;
	char a;
	std::string answer;
	std::ostringstream command;

	if (eAct == MM::BeforeGet)
	{
		pProp->Set(m_SingleLedOnoff[index].c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		// get property
		pProp->Get(m_SingleLedOnoff[index]);
		// prepare command
		if (m_SingleLedOnoff[index] == "ON")
			a='T';
		else 
			a = 'F';
		command << "@AE0" << index + 1 << a;
		// send command
		ret = SendSerialCommand(port_.c_str(), command.str().c_str(), "\n");
		if (ret != DEVICE_OK) return ret;
		ret = GetSerialAnswer(port_.c_str(), "\n", answer);//接收返回信息："@OK"或"@ERR"
		if (ret != DEVICE_OK) return ret;
		if (answer == "@ERR")
		{
			return ERR_READ_CURRENT_FAIL;
		}
	}

	return ret;
}