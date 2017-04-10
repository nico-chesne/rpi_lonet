/*
 * GsmAnswer.h
 *
 *  Created on: Apr 10, 2017
 *      Author: nico
 */

#ifndef GSMANSWER_H_
#define GSMANSWER_H_

#include <ostream>
#include <GsmLine.h>

// Class to hold a full answer from a GSM module
class GsmAnswer
{
public:

	typedef enum {
	    GSM_OK,
	    GSM_ERROR
	} GsmStatus_t;

public:
  // Default ctor
  GsmAnswer():
	  status(GSM_OK),
	  line_number(0),
	  lines(0),
	  cmd(0)
	{ }

  // Ctor to assign a command
  GsmAnswer(const char *_cmd):
	  status(GSM_OK),
	  line_number(0),
	  lines(0)
  {
    if (_cmd)
    	cmd = strdup(_cmd);
    else
    	cmd = 0;
  }
  // Dtor
  ~GsmAnswer() {
    if (lines) delete lines;
    if (cmd)   free(cmd);
    line_number = 0;
  }

  inline void reset() {
    if (cmd) free(cmd);
    if (lines) delete lines;
    lines = NULL;
    line_number = 0;
    status = GSM_OK;
  }

  inline void reset(const char *_cmd) {
    if (cmd) free(cmd);
    if (lines) delete lines;
    lines = NULL;
    line_number = 0;
    status = GSM_OK;
    cmd = strdup(_cmd);
  }

  inline int setCmd(const char *_cmd) {
    if (cmd)
      return -1;
    cmd = strdup(_cmd);
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
    s << "Answer has "<< this->line_number << " lines. Status is '" << (this->status == GSM_OK ? "OK" : "ERROR") << "'" << CR << LF;
    if (line_number)
      lines->displayAll(s);
  }


private:
  char       *cmd;
  int         line_number;
  GsmLine    *lines;
  GsmStatus_t status;
};



#endif /* GSMANSWER_H_ */
