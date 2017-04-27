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
#include <Serial.h>

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
		char c;
		int has_data = 0;
		while (serial.selectData(100) > 0)
		{
			serial.readData(&c, 1);
			printf("%c", c);
		}

		serial.printf("AT\r\n");
		usleep(1000);
		while (serial.selectData(100) > 0)
		{
			serial.readData(&c, 1);
			printf("%c", c);
		}
		serial.printf("ATE0\r\n");
		usleep(1000);
		while (serial.selectData(100) > 0)
		{
			serial.readData(&c, 1);
			printf("%c", c);
		}

		serial.printf("AT+CGMI\r\n");
		usleep(1000);
		while (serial.selectData(100) > 0)
		{
			serial.readData(&c, 1);
			printf("%c", c);
		}
		serial.printf("AT+CPIN=1234\r\n");
		usleep(6000*1000);
		while (serial.selectData(100) > 0)
		{
			serial.readData(&c, 1);
			printf("%c", c);
		}
		serial.printf("AT+COPS?\r\n");
		usleep(6000*1000);
		while (serial.selectData(100) > 0)
		{
			serial.readData(&c, 1);
			printf("%c", c);
		}

	}

	return 0;
}
