/*
 * Sms.h
 *
 *  Created on: Apr 10, 2017
 *      Author: nico
 */

#ifndef SMS_H_
#define SMS_H_

#include <GsmLine.h>
#include <ostream>


class Sms
{
public:
	typedef enum {
	  SMS_REC_UNREAD        = (1 << 0),
	  SMS_REC_READ          = (1 << 1),
	  SMS_STO_UNSENT        = (1 << 2),
	  SMS_STO_SENT          = (1 << 30),
	  SMS_ALL               = (SMS_REC_UNREAD|SMS_REC_READ|SMS_STO_UNSENT|SMS_STO_SENT)
	} SmsStatus_t;

	typedef enum {
		SMS_CONFIG_IMMEDIATE_DELIVER = 1 << 0,
		SMS_CONFIG_MODE_TEXT         = 1 << 1,
		SMS_CONFIG_TOTAL             = 1 << 2
	} SmsConfig_t;

public:
  Sms():
  date(0),
  text(0),
  from(0),
  index(0),
  status(SMS_ALL) {  }

  Sms(GsmLine &gsmRawData, GsmLine &gsmText);
  ~Sms() {
    if (date) free(date);
    if (text) free(text);
  }
  inline int getIndex() {
    return index;
  }
  inline void setIndex(int newIndex) {
    index = newIndex;
  }
  inline void setStatus(SmsStatus_t newStatus) {
    status = newStatus;
  }
  inline void setStatus(const char *statusStr);

  inline SmsStatus_t getStatus() {
    return status;
  }
  inline void setDate(const char *newDate) {
    if (date) free(date);
    date = strdup(date);
  }
  inline char *getDate() {
    return date;
  }
  inline char *getText() {
    return text;
  }
  inline char *getFrom() {
    return from;
  }
  inline void setText(const char* newText) {
    if (text) free(text);
    text = strdup(newText);
  }

  inline void setFrom(const char *newFrom) {
    if (from) free(from);
    from = strdup(newFrom);
  }

  void display(std::ostream &s);

private:
  int index;
  SmsStatus_t status;
  char *date;
  char *text;
  char *from;
};

inline const char * helper_SmsStatusToString(Sms::SmsStatus_t s);
inline Sms::SmsStatus_t helper_StringToSmsStatus(const char *statusStr);

#endif /* SMS_H_ */
