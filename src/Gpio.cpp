/*
 * Gpio.cpp
 *
 *  Created on: Apr 3, 2017
 *      Author: nico
 */

#include "Gpio.h"

void *Gpio::polling_task(void *g)
{
	Gpio *gpio = (Gpio*)g;
	while (1) {
		if (gpio->poll() == true) {
			if (gpio->callback != 0) {
				gpio->callback(gpio->callback_param);
			}
		}

		pthread_testcancel();
	}
	pthread_exit(NULL);
}

bool Gpio::create_polling_task()
{
	if (pthread_create(&polling_thread, NULL, &Gpio::polling_task, this) != 0) {
		return false;
	}
	return true;
}

bool Gpio::destroy_polling_task()
{
	if (polling_thread) {
		pthread_cancel(polling_thread);
		pthread_join(polling_thread, NULL);
		polling_thread = 0;
		return true;
	}
	return false;
}

