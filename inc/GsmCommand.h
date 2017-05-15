/*
 * GsmCommand.h
 *
 *  Created on: Apr 10, 2017
 *      Author: nico
 */

#ifndef GSM_COMMAND_H_
#define GSM_COMMAND_H_

#include <ostream>
#include <GsmLine.h>

using namespace std;

// Class to hold a command and its full answer from a GSM module
class GsmCommand
{
public:

	typedef enum {
	    GSM_OK,
	    GSM_ERROR,
		GSM_NO_STATUS
	} GsmStatus_t;

public:
  // Default ctor
  GsmCommand():
	  status(GSM_NO_STATUS),
	  line_number(0),
	  lines(0),
	  cmd(0),
	  serial(0)
	{
	  sem_init(&lock, 0, 1);
	}

  // Ctor to assign a command
  GsmCommand(const char *_cmd, Serial *s):
	  status(GSM_NO_STATUS),
	  line_number(0),
	  lines(0),
	  serial(s)
  {
	sem_init(&lock, 0, 1);
    if (_cmd) {
    	cmd = strdup(_cmd);
    }
    else
    	cmd = 0;
  }

  // Copy ctor
  GsmCommand(GsmCommand *orig):
	  status(orig->getStatus()),
	  line_number(orig->getLineNumber()),
	  serial(orig->getSerial()),
	  lines(orig->getLine())
  {
	  sem_init(&lock, 0, 1);
	  cmd = strdup(orig->getCmd());
  }

  // Dtor
  ~GsmCommand() {
    if (lines) delete lines;
    if (cmd)   {
    	free(cmd);
    	cmd = 0;
    }
    line_number = 0;
    sem_destroy(&lock);
  }


  inline int acquireLock() {
	  return sem_wait(&lock);
  }

  inline int acquireLock(uint32_t timeout_ms) {
	  struct timespec ts;
	  ts.tv_sec = timeout_ms / 1000;
	  ts.tv_nsec = (timeout_ms % 1000) * 1000000;
	  return sem_timedwait(&lock, &ts);
  }

  inline int releaseLock() {
	  return sem_post(&lock);
  }

  inline void reset() {
    if (cmd) {
    	free(cmd);
    	cmd = 0;
    }
    if (lines) delete lines;
    lines = NULL;
    line_number = 0;
    status = GSM_NO_STATUS;
  }

  inline int reset(const char *_cmd) {
    reset();
    return setCmd(_cmd);
  }

  inline int process(int wait_before_read_us, int timeout_ms) {
	  setStatus(GSM_NO_STATUS);
	  if (sendCmdToSerial() < 0) {
		  std::cerr << "Unable to send data to serial" << endl;
		  return -1;
	  }
	  usleep(wait_before_read_us);
	  int n = readFromSerial(timeout_ms);
	  return n;
  }

  inline int sendCmdToSerial() {
	  if (!cmd || !serial)
		  return -1;
	  return serial->printfData("%s\r\n", cmd);
  }

  inline int sendRawBufToSerial(char *buf, int len) {
	  if (!buf || !len) return -1;
#if 0
	  if (len == 1)
		  std::cout << "Sending " << to_string(*buf) << endl;
	  else
		  std::cout << "Sending " << buf << endl;
#endif
	  return serial->put(buf, len);
  }

  inline int readFromSerial(int timeout_ms) {
	  GsmLine *l;

	  if (!serial) return -1;
	  if (lines) delete lines;
	  lines = NULL;
	  line_number = 0;
	  l = serial->readGsmLine(timeout_ms);
	  if (!l) {
		  //std::cout << __func__ << "(): Got 0 line" << endl;
		  return -1;
	  }
	  while (l) {
		  if (!strcmp(l->getData(), "OK")) {
			  //std::cout << "Got OK" << endl;
			  setStatus(GSM_OK);
		  }
		  else if (!strcmp(l->getData(), "ERROR")) {
			  //std::cout << "Got ERROR" << endl;
			  setStatus(GSM_ERROR);
		  }
		  else {
			  //std::cout << "Got line '" << l->getData() << "'" << endl;
			  addLine(l->getData());
		  }
		  l = l->getNext();
	  }
	  delete l;
	  if (status == GSM_NO_STATUS)
		  return -1;
	  return 0;
  }

  inline const char *getCmd() {
	  return cmd;
  }

  inline int setCmd(const char *_cmd) {
    if (cmd) {
    	free(cmd);
    }
    if (_cmd) {
    	cmd = strdup(_cmd);
    }
    return 0;
  }

  inline int addLine(GsmLine *l) {
	if (!lines) {
		lines = l;
	}
	else {
		lines->append(l);
	}
	line_number++;
	return 0;
  }

  inline int addLine(const char *l) {
    if (!lines) {
      lines = new GsmLine(l);
    }
    else {
      lines->append(l);
    }
    line_number++;
    return 0;
  }

  inline void setStatus(GsmStatus_t s) {
    status = s;
  }
  inline GsmStatus_t getStatus() {
    return status;
  }
  inline GsmLine *getLine() {
    return lines;
  }

  inline void setSerial(Serial *s) {
	  serial = s;
  }
  inline Serial *getSerial() {
	  return serial;
  }

  inline int getLineNumber() {
    return line_number;
  }

  // Display all lines and info about answer
  inline void display(ostream &s) {
    s << "Answer has "<< this->line_number << " lines. Status is '" << (this->status == GSM_OK ? "OK" : "ERROR") << "'" << LF;
    if (line_number)
      lines->displayAll(s);
  }

protected:

private:
  char       *cmd;
  int         line_number;
  GsmLine    *lines;
  GsmStatus_t status;
  Serial     *serial;
  sem_t       lock;
};



#endif /* GSM_COMMAND_H_ */
