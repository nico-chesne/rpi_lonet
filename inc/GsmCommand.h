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

	}

  // Ctor to assign a command
  GsmCommand(const char *_cmd, Serial *s):
	  status(GSM_NO_STATUS),
	  line_number(0),
	  lines(0),
	  serial(s)
  {
    if (_cmd)
    	cmd = strdup(_cmd);
    else
    	cmd = 0;
  }
  // Dtor
  ~GsmCommand() {
    if (lines) delete lines;
    if (cmd)   free(cmd);
    line_number = 0;
  }

  inline void reset() {
    if (cmd) free(cmd);
    if (lines) delete lines;
    lines = NULL;
    line_number = 0;
    status = GSM_NO_STATUS;
  }

  inline void reset(const char *_cmd) {
    if (cmd) free(cmd);
    if (lines) delete lines;
    lines = NULL;
    line_number = 0;
    status = GSM_NO_STATUS;
    cmd = strdup(_cmd);
  }

  inline int process(int wait_before_read_us, int timeout_ms) {
	  setStatus(GSM_NO_STATUS);
	  if (sendCmdToSerial() < 0)
		  return -1;
	  usleep(wait_before_read_us);
	  return readFromSerial(timeout_ms);
  }

  inline int sendCmdToSerial() {
	  if (!cmd || !serial)
		  return -1;
	  return serial->printfData("%s\r\n", cmd);
  }

  inline int readFromSerial(int timeout_ms) {
	  GsmLine *l;

	  if (!serial) return -1;
	  if (lines) delete lines;
	  lines = NULL;
	  line_number = 0;
	  l = serial->readGsmLine(timeout_ms);
	  if (!l) {
		  return -1;
	  }
	  while (l) {
		  if (!strcmp(l->getData(), "OK"))
			  setStatus(GSM_OK);
		  else if (!strcmp(l->getData(), "ERROR"))
			  setStatus(GSM_ERROR);
		  else
			  addLine(l->getData());
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
    if (cmd)
      free(cmd);
    cmd = strdup(_cmd);
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

  inline int getLineNumber() {
    return line_number;
  }

  // Display all lines and info about answer
  inline void display(ostream &s) {
    s << "Answer has "<< this->line_number << " lines. Status is '" << (this->status == GSM_OK ? "OK" : "ERROR") << "'" << LF;
    if (line_number)
      lines->displayAll(s);
  }

private:
  char       *cmd;
  int         line_number;
  GsmLine    *lines;
  GsmStatus_t status;
  Serial     *serial;
};



#endif /* GSM_COMMAND_H_ */
