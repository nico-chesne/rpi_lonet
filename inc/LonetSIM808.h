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

	typedef struct {
		float longitude;
		float latitude;
		float altitude;
		float utc_time; // THe format is yyyymmddHHMMSS
		int   ttff;     // time to first fix (in seconds)
		int   num;      // Satellites in view for fix
		float speed;    // speed over ground
		float course;  // course over ground
	} GpsInfo_t;

	typedef enum {
		GPS_MODE_0   = 0,
		GPS_MODE_GGA = 1 << 1,
		GPS_MODE_GLL = 1 << 2,
		GPS_MODE_GSA = 1 << 3,
		GPS_MODE_GSV = 1 << 4,
		GPS_MODE_RMC = 1 << 5,
		GPS_MODE_VTG = 1 << 6,
		GPS_MODE_ZDA = 1 << 7,
	} GpsMode_t;

	typedef void (lonetsim808_sms_callback_t)(LonetSIM808 *lonet, Sms *sms);

public:
	LonetSIM808(const char *serial_port_name, Gpio::gpio_id_t gpio_pwr, Gpio::gpio_id_t gpio_ri);
	~LonetSIM808();

public:
	static void riCallback(void *param);

public:
	// Manage
	bool   initialize();
	inline bool   isInitialized() { return is_initialized; }
	char const * const getSerialNumber() { return serial_number; };
	bool   power(bool enable);
	bool   isPowerUp();
	bool   pinSet(const char *pin_code);
	bool   isPinOk();
	string getOperator();
	bool   batteryInfoUpdate();
	bool   batteryInfoGet(bool force_update, BatteryInfo_t *bat_info);
	inline Serial &getSerial() { return serial; }
	bool   getDateTime(struct timeval &tv);

	// Generic send. Cmd must be properly initialized with a Serial line and a command
	bool atCmdSend(const char * at_cmd, int delay_before_read_answer_us);
	// Generic send. GsmCommand will be created and must be deleted after usage
	bool atCmdSend(const char *at_cmd, int delay_before_read_answer_us, GsmCommand **command);

	// Sms
	bool      smsSetConfig(uint32_t config);
	bool      smsUpdateList(bool unread_only);
	uint32_t  smsGetNumber();
	Sms      *smsGet(uint32_t index);
	Sms      *smsGetLast();
	bool      smsDelete(uint32_t te_index);
	bool      smsDeleteFromTabIndex(uint32_t tab_index);
	bool      smsDeleteAll();
	bool      smsSend(const char *number, const char *message, uint32_t *sms_id);
	bool 	  smsDisplayAll(std::ostream &out);
	bool	  smsCallbackInstall(lonetsim808_sms_callback_t cb);
	bool	  smsCallbackUninstall();

	// GPS
	bool gpsOpen();
	bool gpsClose();
	bool gpsReset(bool hot);
	bool gpsGetStatus(bool *gpsHasSignal);
	bool gpsReadInfo(char **answer, LonetSIM808::GpsMode_t mode);

protected:
	static void *sms_monitoring_task(void *g);
	bool gpsTogglePwr(bool toggle);


protected:
	// HW Resources
	Serial        serial;
	LinuxGpio     pwr;
	LinuxGpio     ri;

	// Generic
	GsmCommand    gsm_command;
	char          pin[5];
	bool          is_initialized;
	char          serial_number[16];

	// Battery
	BatteryInfo_t battery_info;

	// Sms
	Sms         **sms_received_list;
	uint32_t      sms_received_number;
	uint32_t      sms_config;
	lonetsim808_sms_callback_t *sms_callback;
	pthread_t 	  sms_monitoring_thread;
public:
	sem_t		  sms_monitoring;
};

#endif /* LONETSIM808_H_ */
