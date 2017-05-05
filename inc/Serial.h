/*
 * Serial.cpp
 *
 *  Created on: Apr 25, 2017
 *      Author: nico
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <stdarg.h>
#include <semaphore.h>


#include "GsmLine.h"

using namespace std;

class Serial
{
public :

	Serial(const char *portName, speed_t sp);
	~Serial();
	int baud(speed_t speed);
	ssize_t put(char c);
	ssize_t put(char *c, int n);
	ssize_t readData(char *buf, size_t n);
	int selectData(unsigned int timeout_ms);
	int flush(unsigned int timeout_ms);
	int printfData(char *format, ...);
	GsmLine *readGsmLine(unsigned int timeout_ms);
	int setBlocking(bool b);
	int closePort();

protected :
	int initPort(speed_t sp);
	int intToSpeed(speed_t sp);

private:
	enum ReadStatus_t {
		READ_GETTING_LINE,
		READ_GOT_CR,
		READ_GOT_LF,
	};

	struct termios 	tio;
	int 			tty_fd;
	speed_t         speed;
	bool            isBlocking;
	sem_t           sem_access;
};

#endif // _SERIAL_H_

