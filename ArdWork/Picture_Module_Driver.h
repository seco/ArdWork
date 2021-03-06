// Picture_Module_Driver.h

#ifndef _PICTURE_MODULE_DRIVER_h
#define _PICTURE_MODULE_DRIVER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Module_Driver.h"

class Picture_Module_Driver : public Module_Driver
{
public:
	Picture_Module_Driver(uint8_t priority = THREAD_PRIORITY_NORMAL);

private:
	uint8 device_count;

protected:
	void DoBeforeSuspend();
	void DoBeforeShutdown();
	void DoAfterInit();
	void DoModuleMessage(Int_Thread_Msg message);
	void DoUpdate(uint32_t deltaTime);
};

#endif

