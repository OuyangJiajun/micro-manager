///////////////////////////////////////////////////////////////////////////////
// FILE:          QWLED.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   KAYJA QWLED
//
// AUTHOR:        Ouyang Jiajun 09/11/2021

#pragma once

#ifndef _QWLED_PLUGIN_H_
#define _QWLED_PLUGIN_H_

#include "MMDevice.h"
#include "DeviceBase.h"
#include <string>
#include <map>
#include <time.h>

//////////////////////////////////////////////////////////////////////////////
// Error codes
#define ERR_PORT_CHANGE_FORBIDDEN	101
#define ERR_ONOFF_CONTROL_FAIL		102
#define ERR_READ_CURRENT_FAIL		103

class QWLED : public CShutterBase<QWLED>
{
public:
	QWLED();
	~QWLED();

	// Device API
	// ----------
	int Initialize();
	int Shutdown();
	void GetName(char* pszName) const;
	bool Busy() { return false; }

	// Shutter API
	// ---------
	int SetOpen(bool open);
	int GetOpen(bool& open) { return DEVICE_UNSUPPORTED_COMMAND; }
	int Fire(double /*interval*/) { return DEVICE_UNSUPPORTED_COMMAND; }

	// action interface
	// ----------------
	int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnConstantCurrent(MM::PropertyBase* pProp, MM::ActionType eAct, long index);
	int OnSingleLedOnoff(MM::PropertyBase* pProp, MM::ActionType eAct, long index);

private:
	//Turn all of the LED OFF(0), otherwise ON
	int LEDOnOff(int);
	// MMCore name of serial port
	std::string port_;
	// close (0) or open (1)
	bool initialized_;

	static int const NUM_LEDS = 4;
	long m_constCurrent[NUM_LEDS];
	std::string m_SingleLedOnoff[NUM_LEDS];
};
#endif