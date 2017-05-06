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

#define MY_LONET_SIM_PIN "1234"

int main(int argc, char **argv)
{
	if (argc <= 1)
	{
		printf("Missing parameter [on|off|read]\n");
		exit (-1);
	}
	LonetSIM808 lonet("/dev/ttyAMA0", 24, 23);

	if (!strcmp(argv[1], "on"))
	{
		if (!lonet.isPowerUp())
		{
			lonet.power(true);
			usleep(200 * 1000);
		}
		if (!lonet.initialize()) {
			std::cerr << "Could not initialize Lonet module" << endl;
			exit (1);
		}
		if (!lonet.isPinOk())
		{
			if (!lonet.pinSet(MY_LONET_SIM_PIN))
			{
				std::cerr << "Failed setting PIN code" << endl;
				exit (1);
			}
		}
		LonetSIM808::BatteryInfo_t bat;

		std::cout << "Lonet is switched " << (lonet.isPowerUp() ? "on" : "off") << endl;
		std::cout << "Lonet serial number is " << lonet.getSerialNumber() << endl;
		std::cout << "Lonet is connected to network: '" << lonet.getOperator() << "'" << endl;

		if (!lonet.batteryInfoGet(true, &bat)) {
			std:cerr << "Unable to get battery info" << endl;
			exit(1);
		}
		switch (bat.charge_status) {
		case LonetSIM808::BATTERY_CHARGING:
			std::cout << "Lonet battery status is charging (" << to_string (bat.capacity) << "%, " << to_string(bat.voltage) << "mV) " << endl;
			break;
		case LonetSIM808::BATTERY_FINISHED_CHARGING:
			std::cout << "Lonet battery status has finished charging (" << to_string (bat.capacity) << "%, " << to_string(bat.voltage) << "mV) " << endl;
			break;
		case LonetSIM808::BATTERY_NOT_CHARGING:
			std::cout << "Lonet battery status is discharging (" << to_string (bat.capacity) << "%, " << to_string(bat.voltage) << "mV) " << endl;
			break;
		default:
			std::cerr << "Invalid battery status" << endl;
			break;
		}
		exit (0);
	}
	if (!strcmp(argv[1], "off"))
	{
		lonet.power(false);
		usleep(100*1000);
		std::cout << "Lonet is switched " << (lonet.isPowerUp() ? "on" : "off") << endl;
		exit (1);
	}
	if (!strcmp(argv[1], "read_ri"))
	{
		LinuxGpio ri(23, Gpio::GPIO_FUNC_IN);
		if (ri.readData() == true)
			std::cout << "GPIO status is 1" << endl;
		else
			std::cout << "GPIO status is 0" << endl;
		return 0;
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

		GsmCommand *cmd;
		if (lonet.atCmdSend(argv[2], &cmd, 1000)) {
			cmd->display(std::cout);
		} else {
			std::cerr << "Unable to send command " << argv[2] << endl;
		}
		delete cmd;
		return 0;
	}

	return 0;
}
