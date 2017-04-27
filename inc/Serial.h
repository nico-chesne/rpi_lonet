/*
 * Serial.cpp
 *
 *  Created on: Apr 25, 2017
 *      Author: nico
 */

#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <stdarg.h>

using namespace std;

class Serial
{
public :

	Serial(const char *portName, speed_t sp);
	int baud(speed_t speed);
	int put(char c);
	int put(char *c, int n);
	int readData(char *buf, size_t n);
	int selectData(unsigned int timeout_ms);
	int printf(char *format, ...);
	int setBlocking(bool b);
	int closePort();

protected :
	int initPort(speed_t sp);
	int intToSpeed(speed_t sp);

private:
	struct termios 	tio;
	int 			tty_fd;
	speed_t         speed;
	bool            isBlocking;
};
