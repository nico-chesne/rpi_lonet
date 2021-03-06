//============================================================================
// Name        : rpi_gsm.cpp
// Author      : 
// Version     :
// Copyright   : 
//============================================================================

#include <iostream>
#include <unistd.h>
#include <LinuxGpio.h>
#include <GsmLine.h>
#include <Sms.h>
#include <Serial.h>
#include <GsmCommand.h>
#include <LonetSIM808.h>
#include <ServerCommand.h>

using namespace std;

ServerCommand server;


void sms_callback(LonetSIM808 *lonet, Sms *sms)
{
	server.commandProcessFromSms(lonet, sms);
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

	// INIT
	// ------------------------
	if (!strcmp(argv[1], "on"))
	{
		if (!lonet.isPowerUp())
		{
			lonet.power(true);
			usleep(500 * 1000);
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

		lonet.smsSetConfig(Sms::SMS_CONFIG_MODE_TEXT);

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

	// SWITCH OFF
	// ------------------------
	if (!strcmp(argv[1], "off"))
	{
		lonet.power(false);
		usleep(100*1000);
		std::cout << "Lonet is switched " << (lonet.isPowerUp() ? "on" : "off") << endl;
		exit (1);
	}

	// READ STATUS PIN
	// ------------------------
	if (!strcmp(argv[1], "read_ri"))
	{
		LinuxGpio ri(23, Gpio::GPIO_FUNC_IN);
		if (ri.readData() == true)
			std::cout << "GPIO status is 1" << endl;
		else
			std::cout << "GPIO status is 0" << endl;
		return 0;
	}


	// SEND SMS <number> <message>
	// ------------------------
	if (!strcmp(argv[1], "sms"))
	{
		uint32_t sms_id = 0;
		if (argc < 3) {
			printf("Error: expected phone number\n");
			exit(1);
		}
		if (argc < 4) {
			printf("Error: expected msg text\n");
			exit(1);
		}

		printf("Sending msg '%s' to '%s'\n", argv[3], argv[2]);
		if (lonet.smsSend(argv[2], argv[3], &sms_id))
			printf("Got id '%d'\n", sms_id);
		lonet.smsUpdateList(false);
		lonet.smsDisplayAll(std::cout);
		exit (0);
	}

	// LIST SMS
	// ------------------------
	if (!strcmp(argv[1], "list_sms"))
	{
		lonet.smsUpdateList(false);
		lonet.smsDisplayAll(std::cout);
		return 0;
	}

	// RECEIVE SMS
	// ------------------------
	if (!strcmp(argv[1], "receive_sms"))
	{
		lonet.initialize();
		server.commandRegister("ping", &ServerCommand::commandPing);
		server.commandRegister("battery", &ServerCommand::commandBattery);
		server.commandRegister("location", &ServerCommand::commandLocation);
		server.commandRegister("status", &ServerCommand::commandTimelapseStatus);
		lonet.smsCallbackInstall(sms_callback);
		lonet.smsUpdateList(false);

		while (true) {
			std::cout << "Waiting for new sms" << endl;
			usleep(2000*1000);
		}
		return true;
	}


	// DELETE ONE SMS
	// ------------------------
	if (!strcmp(argv[1], "delete_sms")) {
		if (argc < 3) {
			cout << "Missing sms index\n";
			exit(1);
		}
		lonet.smsUpdateList(false);
		lonet.smsDelete(atoi(argv[2]));
		return 0;
	}

	// SEND AT CMD
	if (!strcmp(argv[1], "cmd")) {
		if (argc < 3) {
			printf("error: expected GSM command to send to device\n");
			exit(-1);
		}

		GsmCommand *cmd;
		if (lonet.atCmdSend(argv[2], 1000, &cmd)) {
			cmd->display(std::cout);
		} else {
			std::cerr << "Unable to send command " << argv[2] << endl;
		}
		delete cmd;
		return 0;
	}

	return 0;
}
