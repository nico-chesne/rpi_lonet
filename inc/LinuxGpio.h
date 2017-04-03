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

#define GPIO_SYSFS_BASE "/sys/class/gpio"

class LinuxGpio: public Gpio {
public:
	LinuxGpio(gpio_id_t _id):
		Gpio(_id)
	{
		string export_str = GPIO_SYSFS_BASE "/export";
		ofstream exportgpio(export_str.c_str());
		if (exportgpio < 0) {
		    cout << " OPERATION FAILED: Unable to export GPIO"<<  _id << endl;
		}
		exportgpio << _id << endl;;
		exportgpio.close();

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

	}
	~LinuxGpio()
	{
	    string unexport_str = GPIO_SYSFS_BASE "/unexport";
	    ofstream unexportgpio(unexport_str.c_str());
	    if (unexportgpio < 0){
	        cout << " OPERATION FAILED: Unable to unexport GPIO"<< this->id << endl;
	    } else {
	    	unexportgpio << this->id;;
	    	unexportgpio.close();
	    }
	}

protected:

	bool update_config(gpio_func_t f) {
		string setdir_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/direction";
		ofstream setdirgpio(setdir_str.c_str());
		if (setdirgpio < 0) {
			cout << " OPERATION FAILED: Unable to set direction of GPIO"<< this->id <<  endl;
		    return false;
		}
		if (f == GPIO_FUNC_OUT)
			setdirgpio << "out";
		else if (f == GPIO_FUNC_IN)
			setdirgpio << "in";
		else {
			cout << "Bad function (" << f << "). Configuration aborted" << endl;
			return false;
		}
        setdirgpio.close();
        function = f;
        return true;
	}


	bool update_value() {
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

	bool update_value(bool val) {
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

	bool update_active_low(bool val) {
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

	bool poll() {
		string setval_str = std::string(GPIO_SYSFS_BASE "/gpio") + std::to_string(this->id) + "/value";
	    int fd;
	    fd_set fds;
	    struct timeval tv;

	    if ((fd = open(setval_str.c_str(), O_RDONLY)) < 0) {
	    	cout << " OPERATION FAILED: Unable to open file " << setval_str << " for polling" << endl;
	    	return false;
	    }
	    FD_ZERO(&fds);
	    FD_SET(fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
	    if (select(fd+1, NULL, NULL, &fds, &tv) <= 0) {
	    	return false;
	    }

	    close(fd);
	    return true;
	}

};

#endif /* LINUXGPIO_H_ */
