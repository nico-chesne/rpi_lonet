/*
 * Sms.cpp
 *
 *  Created on: Apr 10, 2017
 *      Author: nico
 */

#include <Sms.h>

inline const char * helper_SmsStatusToString(Sms::SmsStatus_t s)
{
  switch (s)
  {
  case Sms::SMS_REC_UNREAD:
    return "REC UNREAD";
  case Sms::SMS_REC_READ:
    return "REC READ";
  case Sms::SMS_STO_UNSENT:
    return "STO UNSENT";
  case Sms::SMS_STO_SENT:
    return "STO SENT";
  case Sms::SMS_ALL:
    return "ALL";
  default:
    return "";
  }
}

inline Sms::SmsStatus_t helper_StringToSmsStatus(const char *statusStr)
{
  if (!strcmp(statusStr, "REC UNREAD")) {
    return Sms::SMS_REC_UNREAD;
  } else if (!strcmp(statusStr, "REC READ")) {
      return Sms::SMS_REC_READ;
  } else if (!strcmp(statusStr, "STO UNSENT")) {
    return Sms::SMS_STO_UNSENT;
  } else if (!strcmp(statusStr, "STO SENT")) {
    return Sms::SMS_STO_SENT;
  } else if (!strcmp(statusStr, "ALL")) {
    return Sms::SMS_ALL;
  }
  return Sms::SMS_ALL;
}

Sms::Sms(GsmLine &gsmRawData, GsmLine &gsmText)
{
  // char tmpSta1[16], tmpSta2[8];
  // char tmpFrom[24];
  char delims[] = {' ', ',', '"', '"',  '"', '"',  '"', '"',  '"', '"'};
  char *strs[] = {NULL, NULL, NULL, NULL, NULL};
  char *data = gsmRawData.getData();
  unsigned int t = 0, s = 0, i = 0;
  int err = 0;

  date = NULL;
  text = NULL;
  index = 0;
  status = SMS_ALL;

  // Basic test, for when I'll make this algo more generic (in its own func)
  if ((sizeof(delims) / sizeof(char)) / 2 == (sizeof(strs) / sizeof(char*)))
  {
    while (data[i]) {
      // Look for begining of next token
      while (data[i] && data[i] != delims[t])
        i++;
      // if end of str data: error
      if (!data[i])
      {
        err = 1;
        break;
      }
      // If no more room to store start of delims: err
      if (s >= sizeof(strs))
      {
        err = 1;
        break;
      }
      // Move to first char of the token
      i++;
      strs[s] = &data[i];
      s++;

      // Move to next delimitors to search for
      t++;
      // If no more token to look for: error
      if (t >= sizeof(delims))
      {
        err = 1;
        break;
      }
      // Now look for end of token
      while (data[i] && data[i] != delims[t])
      {
        // pc.putc(data[i]);
        i++;
      }
      // if no more data to read: parse error
      if (!data[i])
      {
        err = 1;
        break;
      }
      data[i] = '\0';
      // pc.putc('\r');
      // pc.putc('\n');
      t++;
      // If no more token to look for: we're good, exit
      if (t >= sizeof(delims))
        break;

      // Move to next char
      i++;
    }
    if (!err)
    {
      index = atoi(strs[0]);
      status = helper_StringToSmsStatus(strs[1]);
      from = strdup(strs[2]);
      date = strdup(strs[4]);
      text = strdup(gsmText.getData());
      // pc.printf("parsed %d/%s/%s/%s\r\n", index, strs[1], strs[2], strs[4]);
    }
  }
}


inline void Sms::setStatus(const char *statusStr) {
  status = helper_StringToSmsStatus(statusStr);
}

inline void Sms::display(std::ostream &s)
{
   if (date && text)
     s << "Sms #"<< index << "\tstatus: "<< helper_SmsStatusToString(status) << "\tdate:"<< date << "\tfrom:"<< from << CR << LF << "\tsays:'"<< text << "'"<< CR << LF;
 }
