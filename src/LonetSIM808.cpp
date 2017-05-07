/*
 * LonetSIM808.cpp
 *
 *  Created on: May 5, 2017
 *      Author: nico
 */

#include <LonetSIM808.h>

LonetSIM808::LonetSIM808(const char *serial_port_name, Gpio::gpio_id_t gpio_pwr, Gpio::gpio_id_t gpio_ri):
	serial(serial_port_name, LONETSIM808_DEFAULT_SERIAL_SPEED),
	pwr(gpio_pwr, Gpio::GPIO_FUNC_OUT),
	ri(gpio_ri, Gpio::GPIO_FUNC_IN)
{
	ri.set_callback(LonetSIM808::riCallback, this, Gpio::GPIO_EDGE_FALLING, LONETSIM808_DEFAULT_RI_POLL_TIMEOUT);
	memset(pin, 0, sizeof (pin));
	memset(&battery_info, 0, sizeof(battery_info));
	sms_received_list = 0;
	sms_received_number = 0;
	sms_config = 0;
	is_initialized = false;
	memset(serial_number, 0, sizeof(serial_number));
}

LonetSIM808::~LonetSIM808()
{
	if (sms_received_list)
		for (int i = 0; i < sms_received_number; i++) delete sms_received_list[i];
	free(sms_received_list);
	sms_received_number = 0;
}


void LonetSIM808::riCallback(void *param)
{
	LonetSIM808 *lonet = (LonetSIM808 *)param;
	std::cout << "ri callback !" << endl;
	lonet->smsUpdateList();
	if (lonet->smsGetNumber() > 0)
		lonet->smsGetLast()->display(std::cout);
}


// Manage
bool   LonetSIM808::initialize()
{
	if (is_initialized) {
		return true;
	}
	// Send init sequence
	serial.flush(100);

	// Initiate connection
	GsmCommand cmd("AT", &serial);
	if (cmd.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || cmd.getStatus() != GsmCommand::GSM_OK) {
		std::cerr << "Unable to send command AT" << endl;
		return false;
	}

	// Echo off
	cmd.reset("ATE0");
	if (cmd.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || cmd.getStatus() != GsmCommand::GSM_OK) {
		std::cerr << "Unable to send command ATE0" << endl;
		return false;
	}

    // Request Manufacturer Identification
	cmd.reset("AT+CGMI");
	if (cmd.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || cmd.getStatus() != GsmCommand::GSM_OK || cmd.getLineNumber() < 1) {
		std::cerr << "Unable to send command AT+CGMI" << endl;
		return false;
	}
	if (strcmp(cmd.getLine()->getData(), LONETSIM808_MANUF_ID) != 0) {
		std::cerr << "This module is not a LONET SIM808 module!" << endl;
		return false;
	}

    // Request Model Identification
	cmd.reset("AT+CGMM");
	if (cmd.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || cmd.getStatus() != GsmCommand::GSM_OK || cmd.getLineNumber() < 1) {
		std::cerr << "Unable to send command AT+CGMM" << endl;
		return false;
	}
	if (strcmp(cmd.getLine()->getData(), LONETSIM808_MODEL_ID) != 0) {
		std::cerr << "This module is not a LONET SIM808 module (" << cmd.getLine()->getData() << ")!" << endl;
		return false;
	}

    // Request Model Identification
	cmd.reset("AT+CGSN");
	if (cmd.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || cmd.getStatus() != GsmCommand::GSM_OK || cmd.getLineNumber() < 1) {
		std::cerr << "Unable to send command AT+CGSN" << endl;
		return false;
	}
	strcpy(serial_number, cmd.getLine()->getData());

	is_initialized = true;

	return true;
}

bool   LonetSIM808::power(bool enable)
{
	// Procedure to power up and down is the same
	if ((enable && !isPowerUp()) || (!enable && isPowerUp())) {
		pwr.set(1);
		usleep(2500000);
		pwr.set(0);
	}
}

bool   LonetSIM808::isPowerUp()
{
	return ri.readData();
}

bool   LonetSIM808::pinSet(const char *pin_code)
{
	char tmp[32];

	snprintf(tmp, sizeof (tmp), "AT+CPIN=%s", pin_code);
	GsmCommand cmd(tmp, &serial);
	if (cmd.process(LONETSIM808_DELAY_AFTER_PIN_SET_MS * 1000, 100) != 0) {
		std:cerr << "Unable to set PIN" << endl;
		return false;
	}
	if (cmd.getStatus() == GsmCommand::GSM_ERROR) {
		std::cerr << "Invalid PIN code " << pin_code << endl;
		return false;
	}
	snprintf(pin, sizeof(pin), "%s", pin_code);
	return true;
}

bool   LonetSIM808::isPinOk()
{
	GsmCommand cmd("AT+CPIN?", &serial);
	if (cmd.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) == 0 && cmd.getStatus() == GsmCommand::GSM_OK && cmd.getLineNumber() >= 1) {
		if (strcmp(cmd.getLine()->getData(), "+CPIN: READY") == 0) {
			return true;
		}
		std::cerr << "Pin not ready, got " << cmd.getLine()->getData() << endl;
		return false;
	}
	std::cerr << "Something got wrong when requesting AT+CPIN?" << endl;
	return false;
}

string LonetSIM808::getOperator()
{
	GsmCommand cmd("AT+COPS?", &serial);
	if (cmd.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) == 0 && cmd.getStatus() == GsmCommand::GSM_OK && cmd.getLineNumber() >= 1) {
		char tmp[32] = { 0 };
		char *answer = cmd.getLine()->getData();
		// Should retrieve something like +COPS: 0,0,"Orange F"
		int i;
		for (i = 0; answer[i] && answer[i] != '"'; i++)
			;
		i++;
		int start = i;
		for (i = start; answer[i] && answer[i] != '"'; i++)
			;
		int end = i;
		strncpy(tmp, answer+start, 32);
		tmp[end - start] = 0;
		return std::string(tmp);
	}
	std::cerr << "Could not retrieve Operator name" << endl;
	return std::string("");
}

bool   LonetSIM808::batteryInfoUpdate()
{
	int bcs, bcl, voltage;
	GsmCommand cmd("AT+CBC", &serial);
	if (cmd.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) == 0 && cmd.getStatus() == GsmCommand::GSM_OK && cmd.getLineNumber() >= 1) {
		if (3 == sscanf(cmd.getLine()->getData(), "+CBC: %d,%d,%d", &bcs, &bcl, &voltage)) {
			switch(bcs) {
				case 0:
					battery_info.charge_status = BATTERY_NOT_CHARGING;
					break;
				case 1:
					battery_info.charge_status = BATTERY_CHARGING;
					break;
				case 2:
					battery_info.charge_status = BATTERY_FINISHED_CHARGING;
					break;
				default:
					battery_info.charge_status = BATTERY_UNKNOWN_STATUS;
					break;
			}
			battery_info.capacity = bcl;
			battery_info.voltage = voltage;
			return true;
		} else {
			std::cerr << "Unable to parse battery status ()" << cmd.getLine()->getData() << endl;
			return false;
		}
	} else {
		std::cerr << "Unable to update battery status" << endl;
		return false;
	}
}

bool   LonetSIM808::batteryInfoGet(bool force_update, BatteryInfo_t *bat_info)
{
	if (!bat_info) return false;
	if (force_update) {
		if (batteryInfoUpdate() == false)
			return false;
	}
	memcpy(bat_info, &battery_info, sizeof(BatteryInfo_t));
	return true;
}


// Generic
bool LonetSIM808::atCmdSend(GsmCommand *command, int delay_before_read_answer_us)
{
	if (!command) return false;
	if (command->getStatus() != GsmCommand::GSM_NO_STATUS) return false;
	if (!strcmp(command->getCmd(), "")) return false;

	if (command->process(delay_before_read_answer_us, 100) != 0) {
		std::cerr << "Issue while processing command " << command->getCmd() << endl;
		return false;
	}
	return true;
}

bool LonetSIM808::atCmdSend(const char *at_cmd, GsmCommand **command, int delay_before_read_answer_us)
{
	if (!at_cmd) return false;
	if (!command) return false;
	*command = new GsmCommand(at_cmd, &serial);

	if ((*command)->process(delay_before_read_answer_us, 100) != 0) {
		std::cerr << "Issue while processing command " << (*command)->getCmd() << endl;
		return false;
	}
	return true;
}

// Sms
bool      LonetSIM808::smsSetConfig(uint32_t config)
{
	bool res = true;
	char *at_command;
	GsmCommand *gsm_command;

	// AT+CNMI=<mode,mt, bm, ds, bfr>
	if ((config & Sms::SMS_CONFIG_IMMEDIATE_DELIVER) && !(sms_config & Sms::SMS_CONFIG_IMMEDIATE_DELIVER)) {
		at_command = "AT+CNMI=2,2,0,0,0";
		bool cmd_res = true;

		cmd_res = atCmdSend(at_command, &gsm_command, 1000);
		if (!cmd_res) {
			std::cerr << "Unable to send command '" << at_command << "' " << endl;
			res = res && false;
		} else if (gsm_command->getStatus() != GsmCommand::GSM_OK) {

		} else {
			sms_config |= Sms::SMS_CONFIG_IMMEDIATE_DELIVER;
		}
		delete gsm_command;
	}
	else if (!(config & Sms::SMS_CONFIG_IMMEDIATE_DELIVER) && (sms_config & Sms::SMS_CONFIG_IMMEDIATE_DELIVER)) {
		at_command = "AT+CNMI=0,0,0,0,0";
		bool cmd_res = true;

		cmd_res = atCmdSend(at_command, &gsm_command, 1000);
		if (!cmd_res) {
			std::cerr << "Unable to send command '" << at_command << "' " << endl;
			res = res && false;
		} else if (gsm_command->getStatus() != GsmCommand::GSM_OK) {

		} else {
			sms_config &= ~Sms::SMS_CONFIG_IMMEDIATE_DELIVER;
		}
		delete gsm_command;
	}

	// AT+CMGF=<mode>
	// 0: PDU
	// 1: Text
	if ((config & Sms::SMS_CONFIG_MODE_TEXT) && !(sms_config & Sms::SMS_CONFIG_MODE_TEXT)) {
		at_command = "AT+CMGF=1";
		bool cmd_res = true;

		cmd_res = atCmdSend(at_command, &gsm_command, 1000);
		if (!cmd_res) {
			std::cerr << "Unable to send command '" << at_command << "' " << endl;
			res = res && false;
		} else if (gsm_command->getStatus() != GsmCommand::GSM_OK) {

		} else {
			sms_config |= Sms::SMS_CONFIG_MODE_TEXT;
		}
		delete gsm_command;
	}
	else if (!(config & Sms::SMS_CONFIG_MODE_TEXT) && (sms_config & Sms::SMS_CONFIG_MODE_TEXT)) {
		at_command = "AT+CMGF=0";
		bool cmd_res = true;

		cmd_res = atCmdSend(at_command, &gsm_command, 1000);
		if (!cmd_res) {
			std::cerr << "Unable to send command '" << at_command << "' " << endl;
			res = res && false;
		} else if (gsm_command->getStatus() != GsmCommand::GSM_OK) {

		} else {
			sms_config &= ~Sms::SMS_CONFIG_MODE_TEXT;
		}
		delete gsm_command;
	}

}

bool      LonetSIM808::smsUpdateList()
{

  int i;
  GsmLine *l;
  GsmCommand *cmd;

  // Clean up the current tab
  for (i = 0; i < sms_received_number; i++) delete sms_received_list[i];
  free(sms_received_list);
  sms_received_list = NULL;
  sms_received_number = 0;

  // Retrieve all sms from the module
  cmd = new GsmCommand("AT+CMGL=\"ALL\"", &serial);
  cmd->process(50*1000, 100);

  // Lock answer again coz we need it before someone else send a command"
  if (cmd->getStatus() != GsmCommand::GSM_OK) {
	  // Problem with execution of command
	  std::cerr << "Unable to get sms list from TE" << endl;
	  delete cmd;
	  return false;
  }

  if (cmd->getLineNumber() % 2 != 0) {
	// Problem with answer: we should have a number of lines multiple of 2
	  std::cerr << "Line number received is not even (" << to_string(cmd->getLineNumber()) << ")" <<endl;
	  delete cmd;
	  return false;
  }
  i = cmd->getLineNumber() / 2;
  if (i == 0)
  {
	  std::cerr << "No sms received" << endl;
	  delete cmd;
	  return true;
  }
  sms_received_list = (Sms **)malloc(sizeof (Sms*) * i);

  // Should no be NULL
  i = 0;
  l = cmd->getLine();
  while (l != NULL)
  {
	if (l->getNext() == NULL) {
		// FATAL: how could we reach this point ??
		std::cerr << "This is odd... Should have a NULL next pointer" << endl;
		delete cmd;
		return false;
	}
	// LONET_DEBUG(this, "%s: adding sms %d with following data:\r\n%s\r\n%s\r\n",
	//             __FUNCTION__, i, l->getData(), l->getNext()->getData());
	// Create the new Sms entry with the super clever Sms Ctor
	sms_received_list[i] = new Sms(*l, *l->getNext());

	// Fill up next Sms object
	l = l->getNext()->getNext();;
	sms_received_number++;
	i++;
  }

  return true;
}

uint32_t  LonetSIM808::smsGetNumber()
{
	return sms_received_number;
}

Sms      *LonetSIM808::smsGet(uint32_t index)
{
	if (sms_received_list && index < sms_received_number) {
		return sms_received_list[index];
	}
	return NULL;
}

Sms      *LonetSIM808::smsGetLast()
{
	if (sms_received_number)
		return sms_received_list[sms_received_number - 1];
	return NULL;
}

bool      LonetSIM808::smsDelete(uint32_t index)
{
	char at_cmd[16];
	GsmCommand *gsm_cmd;

	if (smsUpdateList() == false) {
		std::cerr << "Unable to update sms list before deletion" << endl;
		return false;
	}
	if (index >= sms_received_number) {
		std::cerr << "Index higher than sms received number (" << to_string(sms_received_number) << ")" << endl;
		return false;
	}
	snprintf(at_cmd, 16, "AT+CMGD=%d", index);
	gsm_cmd = new GsmCommand(at_cmd, &serial);
	if (!gsm_cmd->process(1000, 50)) {
		std::cerr << "Unable to process command " << at_cmd << endl;
		delete gsm_cmd;
		return false;
	}
	if (gsm_cmd->getStatus() != GsmCommand::GSM_OK) {
		std::cerr << "Error returned by command " << at_cmd << endl;
		delete gsm_cmd;
		return false;
	}
	if (smsUpdateList() == false) {
		std::cerr << "Unable to update sms list after deletion" << endl;
		return false;
	}
	return true;
}

bool      LonetSIM808::smsDeleteAll()
{
	for (int i = 0; i < sms_received_number; i++)
		delete sms_received_list[i];
	free(sms_received_list);
	sms_received_number = 0;
	return true;
}

bool LonetSIM808::smsDisplayAll(std::ostream &out)
{
	for (int i = 0; i < sms_received_number; i++)
		sms_received_list[i]->display(out);
	return true;
}

bool      LonetSIM808::smsSend(const char *number, const char *message)
{
	if (!number || !message) {
		return false;
	}
	GsmLine *lines;
	char tmp[32];

	snprintf(tmp, 32, "AT+CMGS=\"%s\"\r\n", number);
	//std::cout << "Sending '" << tmp << "'" << endl;
	serial.put(tmp, strlen(tmp));
	usleep(1000);
	lines = serial.readGsmLine(50);
	if (lines) {
		delete lines;
	}

	serial.printfData("%s", message);
	serial.put((char)0x1A);
	lines = serial.readGsmLine(100);
	if (lines) {
		delete lines;
	}
	return true;
}


// GPS
bool LonetSIM808::gpsOpen()
{

}

bool LonetSIM808::gpsClose()
{

}

bool LonetSIM808::gpsReset()
{

}

bool LonetSIM808::gpsGetStatus(bool *gpsHasSignal)
{

}

bool LonetSIM808::gpsReadInfo(char **location, uint32_t *length)
{

}



