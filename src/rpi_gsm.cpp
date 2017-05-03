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
#include <GsmCommand.h>
#include <LonetSIM808.h>

using namespace std;

#define SERIAL_BUF_SIZE 4096


void ri_callback(void *param)
{
	LinuxGpio *gpio = (LinuxGpio*)param;
	if (param != NULL)
		cout << "RI int triggered on Gpio "<< gpio->getId() << " !\n";
	else
		cout << "RI int triggered on unknown gpio !\n";

	Serial serial("/dev/ttyAMA0", 115200);

	GsmCommand cmd("AT+CMGL=\"ALL\"", &serial);
	cmd.process(100*1000, 200);
	cmd.display(std::cout);
}


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

		// Send PIN code to SIM
		cmd.setCmd("AT+CPIN=1234");
		cmd.process(6000*1000, 100);
		cmd.display(std::cout);

		// Sms mode text = true
		cmd.setCmd("AT+CMGF=1");
		cmd.process(500, 200);
		cmd.display(std::cout);

		// SMS are delivered when received
		cmd.setCmd("AT+CNMI=0,0,0,0,0");
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
		serial.flush(100);
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
		exit (0);
	}

	if (!strcmp(argv[1], "receive"))
	{
		Serial serial("/dev/ttyAMA0", 115200);
		serial.flush(100);

		LinuxGpio ri(23, Gpio::GPIO_FUNC_IN);
		ri.set_callback(&ri_callback, &ri, Gpio::GPIO_EDGE_FALLING, 1000);
		// Display all current sms got so far
		GsmCommand cmd("AT+CMGL=\"ALL\"", &serial);
		cmd.process(100*1000, 200);
		cmd.display(std::cout);

		// Start waiting for a new one
		while (1) {
			cout << "Waiting for ri to trigger\n";
			usleep(5000*1000);
		}
		return 0;
	}

	if (!strcmp(argv[1], "delete_sms")) {
		if (argc < 3) {
			cout << "Missing sms index\n";
			exit(1);
		}
		Serial serial("/dev/ttyAMA0", 115200);
		char tmp[32];
		snprintf(tmp, 32, "AT+CMGD=%s\r\n", argv[2]);
		GsmCommand cmd(tmp, &serial);
		cmd.process(500, 200);
		cmd.display(std::cout);
		return 0;
	}

	if (!strcmp(argv[1], "cmd")) {
		if (argc < 3) {
			printf("error: expected GSM command to send to device\n");
			exit(-1);
		}

		Serial serial("/dev/ttyAMA0", 115200);
		serial.flush(100);
		GsmCommand cmd(argv[2], &serial);
		cmd.process(500, 200);
		cmd.display(std::cout);
		return 0;
	}

	return 0;
}
