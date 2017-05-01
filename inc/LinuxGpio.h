/*
 * LinuxGpio.h
 *
 *  Created on: Apr 3, 2017
 *      Author: nico
 */

#ifndef LINUXGPIO_H_
#define LINUXGPIO_H_

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

#include <Gpio.h>

using namespace std;

#define DEFAULT_POLL_TIMEOUT_MS 10
#define GPIO_SYSFS_BASE "/sys/class/gpio"

class LinuxGpio: public Gpio {
public:
	LinuxGpio(gpio_id_t _id);
	LinuxGpio(gpio_id_t _id, gpio_func_t func);
	~LinuxGpio();
	bool set_callback(Gpio::gpio_int_callback_t cb, void *param, gpio_edge_t e, int timeout_ms);
	bool cancel_callback();

protected:
	bool exportGpio();
	bool unexportGpio();
	bool update_config(gpio_func_t f);
	bool update_edge();
	bool update_edge(gpio_edge_t e);
	bool update_config();
	bool update_value();
	bool update_value(bool val);
	bool update_active_low(bool val);
	bool poll();
	static void *polling_task(void *g);
	bool create_polling_task();
	bool destroy_polling_task();
	void set_poll_timeout(int timeout_ms) {
		poll_timeout_ms = timeout_ms;
	}
	int get_poll_timeout() {
		return poll_timeout_ms;
	}

private:
	pthread_t 	polling_thread;
	int			poll_timeout_ms;
};

#endif /* LINUXGPIO_H_ */
