///////////////////////////////////////////////////////////////////////////////
// FILE:          AOTF.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   
//
// AUTHOR:        Lukas Kapitein / Erwin Peterman 24/08/2009


#ifndef _AOTF_H_
#define _AOTF_H_

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include <string>
#include <map>


//////////////////////////////////////////////////////////////////////////////
// Error codes
#define ERR_PORT_CHANGE_FORBIDDEN	101
#define ERR_ONOFF_CONTROL_FAIL		102
#define ERR_READ_CURRENT_FAIL		103

class AOTF : public CShutterBase<AOTF>
{
public:
   AOTF();
   ~AOTF();
  
   // Device API
   // ----------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const;
   bool Busy();

   // Shutter API
   // ---------
   int SetOpen(bool open);
   int GetOpen(bool& open);
   int Fire(double /*interval*/) {return DEVICE_UNSUPPORTED_COMMAND; }//这个作用上来讲可以去掉，直接分号


   // action interface
   int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct);
   /*下面这些被我注释掉
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnIntensity(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnMaxintensity(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnVersion(MM::PropertyBase* pProp, MM::ActionType eAct);
   */
   //下面这个由索雷博的来，用于查询恒定电流
   int OnConstantCurrent(MM::PropertyBase* pProp, MM::ActionType eAct, long index);

private:

   int LEDOnOff(int);
	bool dynErrlist_free		(void);
	bool dynErrlist_lookup	(int err, std::string* descr);
	bool dynErrlist_add		(int err, std::string descr);
   /*
   int SetIntensity(double intensity);
   int SetShutterPosition(bool state);
   //int GetVersion();

   // MMCore name of serial port
   std::string port_;
   // Command exchange with MMCore                                           
   std::string command_;           
   // close (0) or open (1)
   int state_;
   bool initialized_;
   // channel that we are currently working on 
   std::string activeChannel_;
   //intensity
   double intensity_;
   int maxintensity_; 
   */

   static int const NUM_LEDS = 4;
	const char* m_devName;
	std::string 	m_port;
	std::string 	m_LEDOn;
	long 				m_constCurrent[NUM_LEDS];//设置各通道恒定电流
	bool 				m_initialized;
   // dynamic error list
	std::vector<DynError*>	m_dynErrlist;
};

#endif //_AOTF_H_
