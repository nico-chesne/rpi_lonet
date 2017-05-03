/*
 * Gpio.h
 *
 *  Created on: Apr 3, 2017
 *      Author: nico
 */

#ifndef GPIO_H_
#define GPIO_H_


class Gpio {
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
	Gpio(gpio_id_t _id);
	Gpio(gpio_id_t _id, gpio_func_t func);
	virtual ~Gpio();

	gpio_id_t getId() { return id; }
	bool configure(gpio_func_t f);
	bool set(bool val);
	bool readData();
	void set_active_low(bool al);
	virtual bool set_callback(gpio_int_callback_t cb, void *param);
	virtual bool cancel_callback();


protected:
	gpio_id_t 			id;
	bool 				value;
	gpio_func_t 		function;
	bool 				active_low;
	gpio_int_callback_t callback;
	gpio_edge_t         edge;
	void 				*callback_param;

private:
	virtual bool update_config(gpio_func_t f) = 0;
	virtual bool update_edge() = 0;
	virtual bool update_edge(gpio_edge_t e) = 0;
	virtual bool update_value() = 0;
	virtual bool update_value(bool val) = 0;
	virtual bool update_active_low(bool val) = 0;
};

#endif /* GPIO_H_ */
