/*
 * LinuxGpio.cpp
 *
 *  Created on: Apr 3, 2017
 *      Author: nico
 */

#include "LinuxGpio.h"
#include <pthread.h>
#include <string.h>

LinuxGpio::LinuxGpio(gpio_id_t _id):
	Gpio(_id),
	poll_timeout_ms(0),
	polling_thread(0),
	value_fd(-1)
{
	string export_str = GPIO_SYSFS_BASE "/export";
	ofstream exportgpio(export_str.c_str());
	if (exportgpio < 0) {
		cout << " OPERATION FAILED: Unable to export GPIO"<<  _id << endl;
	}
	exportgpio << _id << endl;;
	exportgpio.close();

	update_config();
	update_edge();
}

LinuxGpio::LinuxGpio(gpio_id_t _id, gpio_func_t func):
	Gpio(_id),
	poll_timeout_ms(0),
	polling_thread(0),
	value_fd(-1)
{
	exportGpio();
	update_config(func);

}

LinuxGpio::~LinuxGpio()
{
	cancel_callback();
	unexportGpio();
}


bool LinuxGpio::cancel_callback()
{
	if (callback) {
		destroy_polling_task();
		return Gpio::cancel_callback();
	}
	return true;
}

bool LinuxGpio::set_callback(Gpio::gpio_int_callback_t cb, void *param, gpio_edge_t e, int timeout_ms)
{
	if (Gpio::set_callback(cb, param) != true) {
		return false;
	}
	if (!update_edge(e)) {
		printf("Unable to set edge to %d\n", e);
		return false;
	}
	poll_timeout_ms = timeout_ms <= 0 ? DEFAULT_POLL_TIMEOUT_MS : timeout_ms;
	return create_polling_task();
}

bool LinuxGpio::exportGpio()
{
	string export_str = GPIO_SYSFS_BASE "/export";
	ofstream exportgpio(export_str.c_str());
	if (exportgpio < 0) {
		cout << " OPERATION FAILED: Unable to export GPIO"<<  this->id << endl;
		return false;
	}
	exportgpio << this->id << endl;;
	exportgpio.close();
	return true;
}

bool LinuxGpio::unexportGpio()
{
    string unexport_str = GPIO_SYSFS_BASE "/unexport";
    ofstream unexportgpio(unexport_str.c_str());
    if (unexportgpio < 0){
        cout << " OPERATION FAILED: Unable to unexport GPIO"<< this->id << endl;
        return false;
    } else {
    	unexportgpio << this->id;;
    	unexportgpio.close();
    }
    return true;
}

bool LinuxGpio::update_edge()
{
	string getval_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/edge";
	string val;
	ifstream getvalgpio(getval_str.c_str());

	if (getvalgpio < 0){
		cout << " OPERATION FAILED: Unable to open direction for GPIO"<< this->id << endl;
	} else {
		getvalgpio >> val;
		if (val == "none")
			edge = GPIO_EDGE_NONE;
		else if (val == "rising")
			edge = GPIO_EDGE_RISING;
		else if (val == "falling")
			edge = GPIO_EDGE_FALLING;
		else if (val == "both")
			edge = GPIO_EDGE_BOTH;
		getvalgpio.close();
	}
    return true;
}

bool LinuxGpio::update_edge(gpio_edge_t e)
{
	string setdir_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/edge";
	ofstream setdirgpio(setdir_str.c_str());
	if (setdirgpio < 0) {
		cout << " OPERATION FAILED: Unable to set edge of GPIO"<< this->id <<  endl;
	    return false;
	}
	switch(e)
	{
	case GPIO_EDGE_NONE:
		setdirgpio << "edge";
		break;
	case GPIO_EDGE_RISING:
		setdirgpio << "rising";
		break;
	case GPIO_EDGE_FALLING:
		setdirgpio << "falling";
		break;
	case GPIO_EDGE_BOTH:
		setdirgpio << "both";
		break;
	default:
		cout << "Bad edge (" << e << "). Configuration aborted" << endl;
		return false;
	}
    setdirgpio.close();
    edge = e;
    return true;
}

bool LinuxGpio::update_config(gpio_func_t f)
{
	string setdir_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/direction";
	ofstream setdirgpio(setdir_str.c_str());
	if (setdirgpio < 0) {
		cout << " OPERATION FAILED: Unable to set direction of GPIO"<< this->id <<  endl;
	    return false;
	}
	switch(f)
	{
	case GPIO_FUNC_OUT:
		setdirgpio << "out";
		break;
	case GPIO_FUNC_IN:
		setdirgpio << "in";
		break;
	default:
		cout << "Bad function (" << f << "). Configuration aborted" << endl;
		return false;
	}
    setdirgpio.close();
    function = f;
    return true;
}

bool LinuxGpio::update_config()
{
	string getval_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/direction";
	string val;
	ifstream getvalgpio(getval_str.c_str());

	if (getvalgpio < 0){
		cout << " OPERATION FAILED: Unable to open direction for GPIO"<< this->id << endl;
	} else {
		getvalgpio >> val;
		if (val == "out")
			function = GPIO_FUNC_OUT;
		else
			function = GPIO_FUNC_IN;
		getvalgpio.close();
	}
    return true;
}

bool LinuxGpio::update_value() {
	string getval_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/value";
	string val;
	ifstream getvalgpio(getval_str.c_str());

	if (getvalgpio < 0){
		cout << " OPERATION FAILED: Unable to get value of GPIO"<< this->id << endl;
		return false;
	}
	getvalgpio >> val;
	if (val == "0")
		value = false;
	else
		value = true;
	getvalgpio.close();
	return true;
}

bool LinuxGpio::update_value(bool val) {
	string setval_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/value";
	ofstream setvalgpio(setval_str.c_str());
	if (setvalgpio < 0) {
		cout << " OPERATION FAILED: Unable to set the value of GPIO"<< this->id << endl;
		return false;
	}
	cout << "Successfully opened " << setval_str.c_str() << ". Setting to " << (val ? "1" : "0") << endl;
	setvalgpio << (val ? "1" : "0");
	setvalgpio.close();
	value = val;
	return true;
}

bool LinuxGpio::update_active_low(bool val) {
	string setval_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/active_low";
	ofstream setvalgpio(setval_str.c_str());
	if (setvalgpio < 0){
		cout << " OPERATION FAILED: Unable to set the active_low of GPIO"<< this->id << endl;
		return false;
	}
	setvalgpio << val;
	setvalgpio.close();
	active_low = val;
	return true;
}

bool LinuxGpio::pollData() {
    int fd, n;

    if (value_fd < 0) {
    	cout << "Invalid value_fd field ! Aborting" << endl;
    	return false;
    }
    // stuff the poll structure
    struct pollfd pollfdset = {
    		.fd = value_fd,
    		.events = POLLPRI | POLLERR,
			.revents = 0
    };
    // Dummy read
    uint8_t dummy;
    if (read(value_fd, &dummy, 1) < 0) {
        return false;
    }
    if ((n = poll(&pollfdset, 1, poll_timeout_ms) > 0) && (pollfdset.revents & POLLPRI)) {
        // Set cursor back to start, for next dummy read
        lseek(value_fd, 0, SEEK_SET);
        return true;
    }

    // Set cursor back to start, for next dummy read
    lseek(value_fd, 0, SEEK_SET);
    return false;
}

void *LinuxGpio::polling_task(void *g)
{
	LinuxGpio *gpio = (LinuxGpio*)g;
	while (1) {
		if (gpio->pollData() == true) {
			if (gpio->callback != 0) {
				gpio->callback(gpio->callback_param);
			}
		}

		pthread_testcancel();
	}
	pthread_exit(NULL);
}

bool LinuxGpio::create_polling_task()
{
	string setval_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/value";
	if ((value_fd = open(setval_str.c_str(), O_RDONLY)) < 0) {
		cout << " OPERATION FAILED: Unable to open file " << setval_str << " for polling" << endl;
		return false;
	}
	if (pthread_create(&polling_thread, NULL, &LinuxGpio::polling_task, this) != 0) {
		return false;
	}
	return true;
}

bool LinuxGpio::destroy_polling_task()
{
	if (polling_thread) {
		pthread_cancel(polling_thread);
		pthread_join(polling_thread, NULL);
		polling_thread = 0;
		close(value_fd);
		value_fd = -1;
		return true;
	}
	return false;
}
