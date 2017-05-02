/*
 * Gpio.cpp
 *
 *  Created on: Apr 3, 2017
 *      Author: nico
 */

#include "Gpio.h"


Gpio::Gpio(gpio_id_t _id):
	id(_id),
	value(false),
	function(GPIO_FUNC_IN),
	edge(GPIO_EDGE_NONE),
	active_low(false),
	callback(0),
	callback_param(0)
{	}

Gpio::Gpio(gpio_id_t _id, gpio_func_t func):
	id(_id),
	value(false),
	edge(GPIO_EDGE_NONE),
	active_low(false),
	callback(0),
	callback_param(0)
{
	configure(func);
}

Gpio::~Gpio() {
	cancel_callback();
}

bool Gpio::configure(gpio_func_t f) {
	// If we move from a func IN to sth else, reset the callback
	if (function == GPIO_FUNC_IN
			&& f != GPIO_FUNC_IN
			&& callback != 0) {
		cancel_callback();
	}
	return update_config(f);
}

bool Gpio::set(bool val) {
	if (function == GPIO_FUNC_OUT) {
		return update_value(val);
	}
	return false;
}

bool Gpio::readData() {
	if (function == GPIO_FUNC_IN)
		update_value();
		return value;
	return false;
};

void Gpio::set_active_low(bool al) {
	if (update_active_low(al)) {
		active_low = al;
	}
}

bool Gpio::set_callback(gpio_int_callback_t cb, void *param) {
	if (function != GPIO_FUNC_IN) {
		return false;
	}
	cancel_callback();
	callback = cb;
	callback_param = param;
	return true;
};


bool Gpio::cancel_callback()
{
	if (callback != 0) {
		edge = GPIO_EDGE_NONE;
		callback = 0;
		callback_param = 0;
		return true;
	}
	return false;
}
