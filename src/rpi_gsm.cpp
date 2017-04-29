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

	if (!strcmp(argv[1], "init"))
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

		cmd.setCmd("AT+CMGF=1");
		cmd.process(500, 200);
		cmd.display(std::cout);

		cmd.setCmd("AT+COPS?");
		cmd.process(200, 100);
		cmd.display(std::cout);

		cmd.setCmd("AT+CBC");
		cmd.process(500, 200);
		cmd.display(std::cout);

		exit(0);
	}


	if (!strcmp(argv[1], "sms"))
	{
		if (argc < 3) {
			printf("Error: expected phone number\n");
			exit(1);
		}
		if (argc < 4) {
			printf("Error: expected msg text\n");
			exit(1);
		}

		printf("Sending msg '%s' to '%s'\n", argv[3], argv[2]);
		Serial serial("/dev/ttyAMA0", 115200);
		GsmLine *lines;

		char tmp[32];
		snprintf(tmp, 32, "AT+CMGS=\"%s\"\r\n", argv[2]);
		serial.put(tmp, strlen(tmp));
		usleep(1000);
		lines = serial.readGsmLine(100);
		if (lines) {
			lines->displayAll(std::cout);
			delete lines;
		}
		serial.put(argv[3], strlen(argv[3]));
		serial.put(0x1A);

		lines = serial.readGsmLine(100);
		if (lines) {
			lines->displayAll(std::cout);
			delete lines;
		}

	}

	return 0;
}
