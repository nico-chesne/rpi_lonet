/*
 * Gpio.h
 *
 *  Created on: Apr 3, 2017
 *      Author: nico
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <pthread.h>

class Gpio {
public:
public:
	enum gpio_func_t {
		GPIO_FUNC_IN,
		GPIO_FUNC_OUT,
		GPIO_FUNC_ALT
	};

	enum gpio_edge_t {
		GPIO_EDGE_NONE,
		GPIO_EDGE_RISING,
		GPIO_EDGE_FALLING,
		GPIO_EDGE_BOTH
	};

	enum gpio_active_t {
		GPIO_ACTIVE_LOW,
		GPIO_ACTIVE_HIGH
	};
	typedef void (*gpio_int_callback_t)(void*);
	typedef unsigned int gpio_id_t;

public:
	Gpio(gpio_id_t _id):
		id(_id) {
		value = false;
		function = GPIO_FUNC_IN;
		edge = GPIO_EDGE_NONE;
		active_low = false;
		callback = 0;
		callback_param = 0;
		polling_thread = 0;
	}
	Gpio(gpio_id_t _id, gpio_func_t func):
		id(_id)
	{
		value = false;
		edge = GPIO_EDGE_NONE;
		active_low = false;
		callback = 0;
		callback_param = 0;
		polling_thread = 0;
		configure(func);
	}
	~Gpio() {
		if (polling_thread != 0) {
			destroy_polling_task();
		}
	}

	bool configure(gpio_func_t f) {
		if (function == GPIO_FUNC_IN
				&& f != GPIO_FUNC_IN
				&& polling_thread != 0) {
			destroy_polling_task();
		}
		return update_config(f);
	}

	bool set(bool val) {
		if (function == GPIO_FUNC_OUT) {
			return update_value(val);
		}
		return false;
	}

	bool read() {
		if (function == GPIO_FUNC_IN)
			update_value();
			return value;
		return false;
	};

	void set_active_low(bool al) {
		if (update_active_low(al)) {
			active_low = al;
		}
	}

	bool set_callback(gpio_int_callback_t cb, void *param) {
		if (function != GPIO_FUNC_IN) {
			return false;
		}
		callback = cb;
		callback_param = param;
		return create_polling_task();
	};


protected:
	gpio_id_t 			id;
	bool 				value;
	gpio_func_t 		function;
	gpio_edge_t 		edge;
	bool 				active_low;
	gpio_int_callback_t callback;
	void 				*callback_param;

private:
	pthread_t polling_thread;

private:
	static void *polling_task(void *g);
	bool create_polling_task();
	bool destroy_polling_task();

	virtual bool poll() = 0;
	virtual bool update_config(gpio_func_t f) = 0;
	virtual bool update_value() = 0;
	virtual bool update_value(bool val) = 0;
	virtual bool update_active_low(bool val) = 0;
};

#endif /* GPIO_H_ */
