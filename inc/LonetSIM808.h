/*
 * LonetSIM808.h
 *
 *  Created on: May 3, 2017
 *      Author: nico
 */

#ifndef LONETSIM808_H_
#define LONETSIM808_H_

#include <semaphore.h>

#include <LinuxGpio.h>
#include <Serial.h>
#include <GsmCommand.h>
#include <Sms.h>

using namespace std;

class LonetSIM808 {

public:
	typedef enum {
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
	bool   initialize(const char *pin_code);
	bool   power(bool enable);
	bool   isPowerUp();
	bool   pinSet(const char *pin_code);
	bool   isPinOk();
	string getOperator();
	bool   batteryInfoUpdate();
	bool   batteryInfoGet(bool force_update = false);

	// Generic
	bool atCmdSend(GsmCommand *command);

	// Sms
	bool      smsSetConfig(uint32_t config);
	bool      smsUpdateList();
	uint32_t  smsGetNumber();
	Sms      *smsGet(uint32_t index);
	Sms      *smsGetLast();
	bool      smsDelete(uint32_t index);
	bool      smsDeleteAll();
	bool      smsSend(const char *number, const char *message);

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
	sem_t         serial_sem;

	// Generic
	char          pin[4];

	// Battery
	BatteryInfo_t battery_info;

	// Sms
	Sms         **sms_received_list;
	uint32_t      sms_received_number;
	uint32_t      sms_config;
};

#endif /* LONETSIM808_H_ */
