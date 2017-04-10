//============================================================================
// Name        : rpi_gsm.cpp
// Author      : 
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <unistd.h>
#include <LinuxGpio.h>
#include <GsmLine.h>
#include <GsmAnswer.h>
#include <Sms.h>

using namespace std;

int main() {
	cout << "Hello there !" << endl; // prints
	LinuxGpio pwr(24);
	usleep(50000);
	pwr.configure(Gpio::GPIO_FUNC_OUT);
	usleep(50000);
	pwr.set(1);
	usleep(2500000);
	pwr.set(0);

	return 0;
}
