/*
* Serial.cpp
*
*  Created on: Apr 25, 2017
*      Author: nico
*/


#include "Serial.h"
#include <iostream>

using namespace std;

Serial::Serial(const char *portName, speed_t speed):
	tty_fd(-1),
	speed(115200),
	isBlocking(false)
{
	tty_fd = open(portName, O_RDWR | O_NOCTTY | O_NDELAY) ;
	if (tty_fd < 0) {
		perror("open");
	}
	this->initPort(speed) ;
}


int Serial::put(char c)
{
	return write(tty_fd, &c, 1) ;
}

int Serial::put(char *c, int n)
{
	return write(tty_fd, c, n) ;
}

int Serial::readData(char *buf, size_t n)
{
	return read(tty_fd, buf, n);
}

int Serial::selectData(unsigned int timeout_ms)
{
	int            n;
	fd_set         input;
	struct timeval timeout;

	/* Initialize the input set */
	FD_ZERO(&input);
	FD_SET(tty_fd, &input);

	/* Initialize the timeout structure */
	timeout.tv_sec  = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;

	/* Do the select */
	n = select(tty_fd + 1, &input, NULL, NULL, &timeout);

	/* See if there was an error */
	if (n < 0)
	{
	  perror("select failed");
	  return -1;
	}
	else
	{
	  return FD_ISSET(tty_fd, &input);
	}
}

int Serial::printfData(char *format, ...)
{
	va_list argList;
	char *buf;
	int n, res = -1;

	va_start(argList, format);
	n = vasprintf(&buf, format, argList);
	va_end(argList);
	if (n > 0)
	{
		res = put(buf, n);
		buf[strlen(buf) - 2] = 0;
		cout << "=> " << buf << endl;
		free(buf);
	}
	return res;
}

int Serial::flush(unsigned int timeout_ms)
{
	char c;
	while (selectData(timeout_ms) > 0)
	{
		readData(&c, 1);
	}
	return 0;
}

#define GSM_LINE_MAX_LENGTH 192
GsmLine *Serial::readGsmLine(unsigned int timeout_ms)
{
	GsmLine *res = NULL;
	char *tmp = NULL, c;
	int pos = 0;
	ReadStatus_t status = READ_GETTING_LINE;

	// Allocate temp buf;
	if ((tmp = (char*)malloc(GSM_LINE_MAX_LENGTH)) == 0)
	{
		printf("%s(): Cannot allocate memory\n", __func__);
		return NULL;
	}
	memset(tmp, 0, GSM_LINE_MAX_LENGTH);

	// Read all chars from serial
	while (selectData(timeout_ms) > 0)
	{
		readData(&c, 1);
		switch (status)
		{
		case READ_GETTING_LINE:
			// Getting a new char
			if (c == CR)
				status = READ_GOT_CR;
			else if (c != LF) {
				status = READ_GETTING_LINE;
				tmp[pos++] = c;
			} else {
				// if got LF: parse error
				printf("%s():%d Parse error: unexpected char LF without CR before\n", __func__, __LINE__);
			}
			break;

		case READ_GOT_CR:
			// End of line, we can add line to GsmAnswer or create it if NULL
			if (c == LF) {
				if (strlen(tmp) != 0) {
					if (res == NULL) {
						res = new GsmLine(tmp);
					}
					else {
						res->append(tmp);
					}
					status = READ_GOT_LF;
				} else {
					// We drop empty line and we go back to start of line
					//printf("%s():%d: Got empty line\n", __func__, __LINE__);
					status = READ_GETTING_LINE;
					break;
				}
				// Let's authorize a second CR after CR, Lonet seems to double it sometimes
			} else if (c != CR) {
				printf("%s():%d: Parse error. Unexpected char '%c' after CR\n", __func__, __LINE__, c);
			}
			break;

		case READ_GOT_LF:
			// Starting a new line
			pos = 0;
			memset(tmp, 0, GSM_LINE_MAX_LENGTH);
			status = READ_GETTING_LINE;

			// Getting a new char
			if (c == CR)
				status = READ_GOT_CR;
			else if (c != LF) {
				status = READ_GETTING_LINE;
				tmp[pos++] = c;
			} else {
				// if got LF: parse error
				printf("%s():%d Parse error: unexpected char LF at beginning of line\n", __func__, __LINE__);
			}
			break;

		default:
			printf("%s()%d unexpected parse status 0x%x\n", __func__, __LINE__, status);
			break;
		}
	}

	free(tmp);
	return res;
}

int Serial::closePort()
{
	if (close(tty_fd) < 0)
	{
		perror("close");
		return -1;
	}
	return 0;
}

int Serial::baud(speed_t sp)
{
	int s = intToSpeed(sp);
	int res = 0;

	cfsetospeed(&tio, s);
	cfsetispeed(&tio, s);
	if (tcsetattr(tty_fd, TCSANOW, &tio) < 0)
		return -1;
	speed = sp;
	return 0;
}

int Serial::intToSpeed(speed_t sp)
{
	switch(sp) {
	case 50:
		return B50;
	case 75:
		return B75;
	case 110:
		return B110;
	case 134:
		return B134;
	case B150:
		return B150;
	case 200:
		return B200;
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 1800:
		return B1800;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	default:
		return -1;
	}
}

int Serial::initPort(speed_t sp)
{
	int s = intToSpeed(sp);
	if (s < 0)
		return -1;
	speed = sp;
	tcgetattr(tty_fd, &tio);
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_cflag = s | CS8 | CREAD | CLOCAL ; // 8n1, see termios.h for more information
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;
	tcflush(tty_fd, TCIFLUSH);
	return tcsetattr(tty_fd, TCSANOW, &tio);
}

int Serial::setBlocking(bool b)
{
	if (b && !isBlocking)
	{
		if (fcntl(tty_fd, F_SETFL, 0) == 0)
		{
			isBlocking = true;
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else if (!b && isBlocking)
	{
		if (fcntl(tty_fd, F_SETFL, FNDELAY) == 0)
		{
			isBlocking = false;
			return 0;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}
