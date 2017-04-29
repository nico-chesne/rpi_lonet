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
#include <Sms.h>
#include <Serial.h>
#include "../inc/GsmCommand.h"

using namespace std;

#define SERIAL_BUF_SIZE 4096

int main(int argc, char **argv)
{
	if (argc <= 1)
	{
		printf("Missing parameter [on|off|read]\n");
		exit (-1);
	}

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "off"))
	{
		LinuxGpio pwr(24);
		usleep(50000);
		pwr.configure(Gpio::GPIO_FUNC_OUT);
		usleep(50000);
		pwr.set(1);
		usleep(2500000);
		pwr.set(0);
		return 0;
	}

	if (!strcmp(argv[1], "read"))
	{
		Serial serial("/dev/ttyAMA0", 115200);
		GsmCommand cmd("AT", &serial);

		serial.flush(100);

		cmd.process(200, 100);
		cmd.display(std::cout);

		cmd.setCmd("ATE0");
		cmd.process(200, 100);
		cmd.display(std::cout);

		cmd.setCmd("AT+CGMI");
		cmd.process(200, 100);
		cmd.display(std::cout);

		cmd.setCmd("AT+CPIN=1234");
		cmd.process(6000*1000, 100);
		cmd.display(std::cout);

		cmd.setCmd("AT+COPS?");
		cmd.process(200, 100);
		cmd.display(std::cout);

		cmd.setCmd("AT+CBC");
		cmd.process(500, 200);
		cmd.display(std::cout);
	}


	return 0;
}
