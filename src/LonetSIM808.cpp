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
	ri(gpio_ri, Gpio::GPIO_FUNC_IN),
	gsm_command()
{
	ri.set_callback(LonetSIM808::riCallback, this, Gpio::GPIO_EDGE_FALLING, LONETSIM808_DEFAULT_RI_POLL_TIMEOUT);
	memset(pin, 0, sizeof (pin));
	memset(&battery_info, 0, sizeof(battery_info));
	sms_received_list = 0;
	sms_received_number = 0;
	sms_config = 0;
	is_initialized = false;
	memset(serial_number, 0, sizeof(serial_number));
	sem_init(&sms_monitoring, 0, 0);
	sms_monitoring_thread = 0;
	sms_callback = 0;
	gsm_command.setSerial(&serial);
}

LonetSIM808::~LonetSIM808()
{
	if (sms_received_list)
		for (int i = 0; i < sms_received_number; i++) delete sms_received_list[i];
	free(sms_received_list);
	sms_received_number = 0;
	if (sms_monitoring_thread) {
		pthread_cancel(sms_monitoring_thread);
		pthread_join(sms_monitoring_thread, NULL);
	}
	sem_destroy(&sms_monitoring);
}


void LonetSIM808::riCallback(void *param)
{
	LonetSIM808 *lonet = (LonetSIM808 *)param;

	//std::cout << "ri callback !" << endl;
	uint32_t sms_number = lonet->smsGetNumber();
	usleep(200 * 1000);
	lonet->smsUpdateList(true);
	lonet->smsGetLast()->display(std::cout);
	if (lonet->smsGetNumber() != sms_number) {
		usleep(1000 * 1000);
		std::cout << "posting sms_monitoring" << endl;
		sem_post(&lonet->sms_monitoring);
	} else {
		std::cout << "Something got wrong: no new sms received" << endl;
	}
}

void *LonetSIM808::sms_monitoring_task(void *g)
{
	LonetSIM808 *lonet = (LonetSIM808 *)g;

	while (1) {
		struct timespec tv;
		tv.tv_sec = 0;
		tv.tv_nsec = 1000 * 1000 * 100; //100ms timout
		if (sem_timedwait(&lonet->sms_monitoring, &tv) < 0) {
			pthread_testcancel();
			continue;
		}
		if (lonet->sms_callback != 0) {
			//std::cout << "Calling callback with " << to_string(lonet->smsGetLast()->getIndex()) << endl;
			lonet->sms_callback(lonet, lonet->smsGetLast());
		}
		pthread_testcancel();
	}
	pthread_exit(NULL);
}


bool	  LonetSIM808::smsCallbackInstall(lonetsim808_sms_callback_t cb)
{
	if (sms_callback != 0 || sms_monitoring_thread != 0) {
		std::cerr << "SMS Callback already installed";
		return false;
	}
	sms_callback = cb;
	if (pthread_create(&sms_monitoring_thread, NULL, &LonetSIM808::sms_monitoring_task, this) != 0) {
		std::cerr << "Unable to create monitoring thread!" << endl;
		sms_callback = 0;
		return false;
	}
	return true;
}

bool	  LonetSIM808::smsCallbackUninstall()
{
	if (sms_monitoring_thread) {
		pthread_cancel(sms_monitoring_thread);
		pthread_join(sms_monitoring_thread, NULL);
	}
	sms_monitoring_thread = 0;
	sms_callback = 0;
	return true;
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
	gsm_command.acquireLock();
	gsm_command.reset("AT");
	if (gsm_command.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || gsm_command.getStatus() != GsmCommand::GSM_OK) {
		std::cerr << "Unable to send command AT" << endl;
		gsm_command.releaseLock();
		return false;
	}
	gsm_command.releaseLock();

	// Echo off
	gsm_command.reset("ATE0");
	gsm_command.acquireLock();
	if (gsm_command.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || gsm_command.getStatus() != GsmCommand::GSM_OK) {
		std::cerr << "Unable to send command ATE0" << endl;
		gsm_command.releaseLock();
		return false;
	}
	gsm_command.releaseLock();

    // Request Manufacturer Identification
	gsm_command.acquireLock();
	gsm_command.reset("AT+CGMI");
	if (gsm_command.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || gsm_command.getStatus() != GsmCommand::GSM_OK || gsm_command.getLineNumber() < 1) {
		std::cerr << "Unable to send command AT+CGMI" << endl;
		gsm_command.releaseLock();
		return false;
	}
	if (strcmp(gsm_command.getLine()->getData(), LONETSIM808_MANUF_ID) != 0) {
		std::cerr << "This module is not a LONET SIM808 module!" << endl;
		return false;
	}
	gsm_command.releaseLock();

    // Request Model Identification
	gsm_command.acquireLock();
	gsm_command.reset("AT+CGMM");
	if (gsm_command.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || gsm_command.getStatus() != GsmCommand::GSM_OK || gsm_command.getLineNumber() < 1) {
		std::cerr << "Unable to send command AT+CGMM" << endl;
		gsm_command.releaseLock();
		return false;
	}
	if (strcmp(gsm_command.getLine()->getData(), LONETSIM808_MODEL_ID) != 0) {
		std::cerr << "This module is not a LONET SIM808 module (" << gsm_command.getLine()->getData() << ")!" << endl;
		gsm_command.releaseLock();
		return false;
	}
	gsm_command.releaseLock();

    // Request Model Identification
	gsm_command.acquireLock();
	gsm_command.reset("AT+CGSN");
	if (gsm_command.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) != 0 || gsm_command.getStatus() != GsmCommand::GSM_OK || gsm_command.getLineNumber() < 1) {
		std::cerr << "Unable to send command AT+CGSN" << endl;
		gsm_command.releaseLock();
		return false;
	}
	strcpy(serial_number, gsm_command.getLine()->getData());
	gsm_command.releaseLock();

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
	return true;
}

bool   LonetSIM808::isPowerUp()
{
	return ri.readData();
}

bool   LonetSIM808::pinSet(const char *pin_code)
{
	char tmp[32];

	snprintf(tmp, sizeof (tmp), "AT+CPIN=%s", pin_code);
	gsm_command.acquireLock();
	gsm_command.reset(tmp);
	if (gsm_command.process(LONETSIM808_DELAY_AFTER_PIN_SET_MS * 1000, 100) != 0) {
		std:cerr << "Unable to set PIN" << endl;
		gsm_command.releaseLock();
		return false;
	}
	if (gsm_command.getStatus() == GsmCommand::GSM_ERROR) {
		std::cerr << "Invalid PIN code " << pin_code << endl;
		gsm_command.releaseLock();
		return false;
	}
	gsm_command.releaseLock();
	snprintf(pin, sizeof(pin), "%s", pin_code);
	return true;
}

bool   LonetSIM808::isPinOk()
{
	gsm_command.acquireLock();
	gsm_command.reset("AT+CPIN?");
	if (gsm_command.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) == 0 && gsm_command.getStatus() == GsmCommand::GSM_OK && gsm_command.getLineNumber() >= 1) {
		if (strcmp(gsm_command.getLine()->getData(), "+CPIN: READY") == 0) {
			gsm_command.releaseLock();
			return true;
		}
		std::cerr << "Pin not ready, got " << gsm_command.getLine()->getData() << endl;
		gsm_command.releaseLock();
		return false;
	}
	std::cerr << "Something got wrong when requesting AT+CPIN?" << endl;
	gsm_command.releaseLock();
	return false;
}

string LonetSIM808::getOperator()
{
	gsm_command.acquireLock();
	gsm_command.reset("AT+COPS?");
	if (gsm_command.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) == 0 && gsm_command.getStatus() == GsmCommand::GSM_OK && gsm_command.getLineNumber() >= 1) {
		char tmp[32] = { 0 };
		char *answer = gsm_command.getLine()->getData();
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
		gsm_command.releaseLock();
		return std::string(tmp);
	}
	std::cerr << "Could not retrieve Operator name" << endl;
	gsm_command.releaseLock();
	return std::string("");
}

bool   LonetSIM808::batteryInfoUpdate()
{
	int bcs, bcl, voltage;

	gsm_command.acquireLock();
	gsm_command.reset("AT+CBC");
	if (gsm_command.process(LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US, 100) == 0 && gsm_command.getStatus() == GsmCommand::GSM_OK && gsm_command.getLineNumber() >= 1) {
		if (3 == sscanf(gsm_command.getLine()->getData(), "+CBC: %d,%d,%d", &bcs, &bcl, &voltage)) {
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
			gsm_command.releaseLock();
			return true;
		} else {
			std::cerr << "Unable to parse battery status ()" << gsm_command.getLine()->getData() << endl;
			gsm_command.releaseLock();
			return false;
		}
	} else {
		std::cerr << "Unable to update battery status" << endl;
		gsm_command.releaseLock();
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
bool LonetSIM808::atCmdSend(const char * at_cmd, int delay_before_read_answer_us)
{
	if (!at_cmd) return false;

	gsm_command.acquireLock();
	gsm_command.reset(at_cmd);
	if (gsm_command.getStatus() != GsmCommand::GSM_NO_STATUS) return false;

	if (gsm_command.process(delay_before_read_answer_us, 100) != 0) {
		std::cerr << "Issue while processing command " << gsm_command.getCmd() << endl;
		gsm_command.releaseLock();
		return false;
	}
	gsm_command.releaseLock();
	return true;
}

bool LonetSIM808::atCmdSend(const char *at_cmd, int delay_before_read_answer_us, GsmCommand **command)
{
	if (!at_cmd) return false;
	if (!command) return false;
	*command = 0;
	gsm_command.acquireLock();
	gsm_command.reset(at_cmd);
	if (gsm_command.process(delay_before_read_answer_us, 100) != 0) {
		std::cerr << "Issue while processing command " << gsm_command.getCmd() << endl;
		gsm_command.releaseLock();
		return false;
	}
	*command = new GsmCommand(this->gsm_command);
	gsm_command.releaseLock();
	return true;
}

// Sms
bool      LonetSIM808::smsSetConfig(uint32_t config)
{
	bool res = true;
	char *at_command;

	// AT+CNMI=<mode,mt, bm, ds, bfr>
	if ((config & Sms::SMS_CONFIG_IMMEDIATE_DELIVER)) { //&& !(sms_config & Sms::SMS_CONFIG_IMMEDIATE_DELIVER)) {
		at_command = "AT+CNMI=2,2,0,0,0";
		bool cmd_res = true;

		cmd_res = atCmdSend(at_command, 1000);
		if (!cmd_res) {
			std::cerr << "Unable to send command '" << at_command << "' " << endl;
			res = res && false;
		} else if (gsm_command.getStatus() != GsmCommand::GSM_OK) {
			std::cerr << "Command '" << at_command << "' returned GSM_ERROR" << endl;
			res = res && false;
		} else {
			sms_config |= Sms::SMS_CONFIG_IMMEDIATE_DELIVER;
		}
	}
	else if (!(config & Sms::SMS_CONFIG_IMMEDIATE_DELIVER)) { // && (sms_config & Sms::SMS_CONFIG_IMMEDIATE_DELIVER)) {
		at_command = "AT+CNMI=0,0,0,0,0";
		bool cmd_res = true;

		cmd_res = atCmdSend(at_command, 1000);
		if (!cmd_res) {
			std::cerr << "Unable to send command '" << at_command << "' " << endl;
			res = res && false;
		} else if (gsm_command.getStatus() != GsmCommand::GSM_OK) {

		} else {
			sms_config &= ~Sms::SMS_CONFIG_IMMEDIATE_DELIVER;
		}
	}

	// AT+CMGF=<mode>
	// 0: PDU
	// 1: Text
	if ((config & Sms::SMS_CONFIG_MODE_TEXT)) { // && !(sms_config & Sms::SMS_CONFIG_MODE_TEXT)) {
		at_command = "AT+CMGF=1";
		bool cmd_res = true;

		cmd_res = atCmdSend(at_command, 1000);
		if (!cmd_res) {
			std::cerr << "Unable to send command '" << at_command << "' " << endl;
			res = res && false;
		} else if (gsm_command.getStatus() != GsmCommand::GSM_OK) {

		} else {
			sms_config |= Sms::SMS_CONFIG_MODE_TEXT;
		}
	}
	else if (!(config & Sms::SMS_CONFIG_MODE_TEXT)) { // && (sms_config & Sms::SMS_CONFIG_MODE_TEXT)) {
		at_command = "AT+CMGF=0";
		bool cmd_res = true;

		cmd_res = atCmdSend(at_command, 1000);
		if (!cmd_res) {
			std::cerr << "Unable to send command '" << at_command << "' " << endl;
			res = res && false;
		} else if (gsm_command.getStatus() != GsmCommand::GSM_OK) {

		} else {
			sms_config &= ~Sms::SMS_CONFIG_MODE_TEXT;
		}
	}
	return res;
}

bool      LonetSIM808::smsUpdateList(bool unread_only)
{

	int i;
	GsmLine *l;

	// If we want to force fetching of ALL sms, clean up current list
	if (!unread_only) {
		// Clean up the current tab
		for (i = 0; i < sms_received_number; i++) delete sms_received_list[i];
		if (sms_received_number > 0) {
			free(sms_received_list);
			sms_received_list = NULL;
			sms_received_number = 0;
		}
	}

	// Retrieve all sms from the module
	gsm_command.acquireLock();
	if (unread_only) {
		gsm_command.reset("AT+CMGL=\"REC UNREAD\"");
	} else {
		gsm_command.reset("AT+CMGL=\"ALL\"");
	}
	gsm_command.process(200*1000, 400);

	// Lock answer again coz we need it before someone else send a command"
	if (gsm_command.getStatus() != GsmCommand::GSM_OK) {
		// Problem with execution of command
		std::cerr << "Unable to get sms list from TE: " << to_string(gsm_command.getStatus()) << endl;
		gsm_command.releaseLock();
		return false;
	}

	// Check that we have an even number of sms, since one SMS takes two lines:
	//
	// +CMGL: 9,"REC UNREAD","XXXXXXXXX","","14/10/16,21:40:08+08"
    // sms text content
	//
	if (gsm_command.getLineNumber() % 2 != 0) {
		// Problem with answer: we should have a number of lines multiple of 2
		std::cerr << "Line number received is not even (" << to_string(gsm_command.getLineNumber()) << ")" <<endl;
		l = gsm_command.getLine();
		while (l != NULL) {
			std::cout << l->getData() << endl;
			l = l->getNext();
		}
		gsm_command.releaseLock();
		return false;
	}
	i = gsm_command.getLineNumber() / 2;

	// No new sms received
	if (i == 0)
	{
		std::cerr << "No new sms received" << endl;
		gsm_command.releaseLock();
		return true;
	}

	// If we only want an update, we need to allocate a new table and copy old sms to this new table
	if (unread_only) {
		// Allocate new table to hold old sms + new sms
		Sms **table_temp = (Sms **)malloc(sizeof (Sms*) * (i + sms_received_number));
		for (i = 0; i < sms_received_number; i++)
			table_temp[i] = sms_received_list[i];
		free(sms_received_list);
		sms_received_list = table_temp;
		// We keep i = last index, to add new sms at end of table
	} else {
		sms_received_list = (Sms **)malloc(sizeof (Sms*) * i);
		i = 0;
	}


	// Now, create new Sms objects at end of list, with content of what is read
	l = gsm_command.getLine();
	while (l != NULL)
	{
		if (l->getNext() == NULL) {
			// FATAL: how could we reach this point ??
			std::cerr << "This is odd... Should not have a NULL next pointer" << endl;
			gsm_command.releaseLock();
			return false;
		}
		// Create the new Sms entry with the super clever Sms Ctor
		sms_received_list[i] = new Sms(*l, *l->getNext());

		// Fill up next Sms object
		l = l->getNext()->getNext();;
		sms_received_number++;
		i++;
	}

	gsm_command.releaseLock();
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

bool      LonetSIM808::smsDelete(uint32_t te_index)
{
	int i;

	for (i = 0; i < sms_received_number; i++) {
		if (sms_received_list[i]->getIndex() == te_index)
			break;
	}
	if (!smsDeleteFromTabIndex(i)) {
		std::cerr << "Unable to delete sms #" << to_string(te_index) << endl;
		return false;
	}

	if (smsUpdateList(true) == false) {
		std::cerr << "Unable to update sms list after deletion" << endl;
		return false;
	}
	return true;
}

bool      LonetSIM808::smsDeleteFromTabIndex(uint32_t tab_index)
{
	char at_cmd[16];

	if (tab_index >= sms_received_number) {
		std::cerr << "TE index not found than sms received number (" << to_string(sms_received_number) << ")" << endl;
		return false;
	}
	snprintf(at_cmd, 16, "AT+CMGD=%d", sms_received_list[tab_index]->getIndex());
	gsm_command.acquireLock();
	gsm_command.reset(at_cmd);
	if (gsm_command.process(20*1000, 50) < 0) {
		std::cerr << "Unable to process command " << at_cmd << endl;
		gsm_command.releaseLock();
		return false;
	}
	if (gsm_command.getStatus() != GsmCommand::GSM_OK) {
		std::cerr << "Error returned by command " << at_cmd << endl;
		gsm_command.releaseLock();
		return false;
	}
	gsm_command.releaseLock();
	return true;
}


bool      LonetSIM808::smsDeleteAll()
{
	gsm_command.acquireLock();
	for (int i = 0; i < sms_received_number; i++) {
		char at_cmd[16];
		GsmCommand *gsm_cmd;

		snprintf(at_cmd, 16, "AT+CMGD=%d", sms_received_list[i]->getIndex());
		gsm_command.reset(at_cmd);
		if (gsm_cmd->process(1000, 50) < 0) {
			// Only warning here...
			std::cerr << "Unable to process command " << at_cmd << endl;
		} else if (gsm_cmd->getStatus() != GsmCommand::GSM_OK) {
			// Only warning here...
			std::cerr << "Error returned by command " << at_cmd << endl;
		}
		delete sms_received_list[i];
	}
	free(sms_received_list);
	sms_received_number = 0;
	gsm_command.releaseLock();
	return true;
}

bool LonetSIM808::smsDisplayAll(std::ostream &out)
{
	for (int i = 0; i < sms_received_number; i++)
		sms_received_list[i]->display(out);
	return true;
}

bool      LonetSIM808::smsSend(const char *number, const char *message, uint32_t *sms_id)
{
	if (!number || !message) {
		return false;
	}
	GsmLine *lines;
	char cmd[32], c = 0x1A;
	int len = strlen(message);

	snprintf(cmd, 32, "AT+CMGS=\"%s\"", number);
	gsm_command.acquireLock();
	gsm_command.reset(cmd);
	if (gsm_command.process(1000, 50) < 0) {
		// Only warning here...
		//std::cerr << "nothing returned " << cmd << endl;
	}
	gsm_command.reset();
	usleep(50*1000);
	if (gsm_command.sendRawBufToSerial((char *)message, strlen(message)) < 0) {
		std::cerr << "Unable send message to TE" << endl;
		gsm_command.releaseLock();
		return false;
	}
	if (gsm_command.sendRawBufToSerial(&c, 1) < 0) {
		std::cerr << "Unable final char to TE" << endl;
		gsm_command.releaseLock();
		return false;
	}
	int trials = 0;
	while (gsm_command.getLineNumber() == 0 && trials < 50) {
		gsm_command.readFromSerial(100);
		usleep(200*1000);
		trials++;
	}
	if (trials < 50 && !strncmp("+CMGS:", gsm_command.getLine()->getData(), 6)) {
		std::cout << "SMS Delivered (id: " << gsm_command.getLine()->getData()+7 << ")" << endl;
		if (sms_id)
			*sms_id = atoi(gsm_command.getLine()->getData() + 7);
		gsm_command.releaseLock();
		return true;
	} else {
		std::cout << "Not sure sms was delivered... waited 10s, but got no answer from TE" << endl;
		gsm_command.releaseLock();
		return false;
	}
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



