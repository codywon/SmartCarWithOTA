/*
 * ExampleCommand.h
 *
 */

#ifndef SMINGCORE_EXAMPLE_COMMAND_H_
#define SMINGCORE_EXAMPLE_COMMAND_H_

#include <SmingCore/SmingCore.h>
#include "WString.h"
#include "../Services/CommandProcessing/CommandProcessingIncludes.h"

//https://www.gitbook.com/book/smartarduino/user-mannual-for-esp-12e-motor-shield/details

typedef Delegate<void()> carMotorDelegate;

enum CarDirection {
	FORWARD,
	BACK,
	LEFT,
	RIGHT,
	FF_LEFT,
	FF_RIGHT,
	BA_LEFT,
	BA_RIGHT
};

class CarCommand
{
public:
	CarCommand(int leftMotorPWM, int rightMotorPWM, int leftMotorDir, int rightMotorDir);
	virtual ~CarCommand();
	void initCommand();

private:
	CarDirection direction = FORWARD;
	int leftMotorPWM;
	int rightMotorPWM;
	int leftMotorDir;
	int rightMotorDir;
	bool stopped = false;

	//--target Speed
	int spdTargetA=1023;

	//--current speed
	int spdCurrentA=0;

	//--target Speed
	int spdTargetB = 1023;

	//--current speed
	int spdCurrentB = 0;
	bool stopFlag =true;
	long lastActionTime;
	Timer motorTimer;

	void processCarCommands(String commandLine, CommandOutput* commandOutput);
	void handleMotorTimer();
	void enableMovement(bool state);
};


#endif /* SMINGCORE_DEBUG_H_ */