/*
 * LonetSIM808.h
 *
 *  Created on: May 3, 2017
 *      Author: nico
 */

#ifndef LONETSIM808_H_
#define LONETSIM808_H_

#include <LinuxGpio.h>
#include <Serial.h>
#include <GsmCommand.h>
#include <Sms.h>

using namespace std;

#define LONETSIM808_DEFAULT_SERIAL_SPEED 115200
#define LONETSIM808_DEFAULT_RI_POLL_TIMEOUT 100
#define LONETSIM808_MANUF_ID "SIMCOM_Ltd"
#define LONETSIM808_MODEL_ID "SIMCOM_SIM808"
#define LONETSIM808_DEFAULT_DELAY_BEFORE_READ_ANSWER_US 200
#define LONETSIM808_DELAY_AFTER_PIN_SET_MS 6000

class LonetSIM808 {

public:
	typedef enum {
		BATTERY_UNKNOWN_STATUS,
		BATTERY_NOT_CHARGING,
		BATTERY_CHARGING,
		BATTERY_FINISHED_CHARGING
	} ChargeStatus_t;

	typedef struct {
		ChargeStatus_t charge_status;
		uint32_t       capacity;
		uint32_t       voltage;
	} BatteryInfo_t;

public:
	LonetSIM808(const char *serial_port_name, Gpio::gpio_id_t gpio_pwr, Gpio::gpio_id_t gpio_ri);
	~LonetSIM808();

public:
	static void riCallback(void *param);

public:
	// Manage
	bool   initialize();
	bool   isInitialized() { return is_initialized; }
	char const * const getSerialNumber() { return serial_number; };
	bool   power(bool enable);
	bool   isPowerUp();
	bool   pinSet(const char *pin_code);
	bool   isPinOk();
	string getOperator();
	bool   batteryInfoUpdate();
	bool   batteryInfoGet(bool force_update, BatteryInfo_t *bat_info);

	// Generic send. Cmd must be properly initialized with a Serial line and a command
	bool atCmdSend(GsmCommand *cmd, int delay_before_read_answer_us);
	// Generic send. GsmCommand will be created and must be deleted after usage
	bool atCmdSend(const char *at_cmd, GsmCommand **command, int delay_before_read_answer_us);

	// Sms
	bool      smsSetConfig(uint32_t config);
	bool      smsUpdateList();
	uint32_t  smsGetNumber();
	Sms      *smsGet(uint32_t index);
	Sms      *smsGetLast();
	bool      smsDelete(uint32_t index);
	bool      smsDeleteAll();
	bool      smsSend(const char *number, const char *message);
	bool 	  smsDisplayAll(std::ostream &out);

	// GPS
	bool gpsOpen();
	bool gpsClose();
	bool gpsReset();
	bool gpsGetStatus(bool *gpsHasSignal);
	bool gpsReadInfo(char **location, uint32_t *length);

protected:
	// HW Resources
	Serial        serial;
	LinuxGpio     pwr;
	LinuxGpio     ri;

	// Generic
	char          pin[5];
	bool          is_initialized;
	char          serial_number[16];

	// Battery
	BatteryInfo_t battery_info;

	// Sms
	Sms         **sms_received_list;
	uint32_t      sms_received_number;
	uint32_t      sms_config;
};

#endif /* LONETSIM808_H_ */
