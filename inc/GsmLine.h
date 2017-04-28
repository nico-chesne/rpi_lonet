/*
 * GsmLine.h
 *
 *  Created on: Apr 10, 2017
 *      Author: nico
 */

#ifndef GSMLINE_H_
#define GSMLINE_H_

#include <ostream>
#include <string.h>

#define CR '\r'
#define LF '\n'

class GsmLine;

// Define a line, part of an Answer from GSM module
// has a linked-list structure
class GsmLine
{
public:
  // Basic Ctor
  GsmLine() {
    line = NULL;
    length = -1;
    next = NULL;
  }
  // Ctor which assign a content
  GsmLine(const char *new_line) {
    if (new_line) {
      line = strdup(new_line);
      length = strlen(new_line);
      next = NULL;
    };
  }
  // Ctor to assign a content plus rest of linked list
  // Used to append an element in HEAD of list
  GsmLine(const char *new_line, GsmLine *_next) {
    if (new_line) {
      line = strdup(new_line);
      length = strlen(new_line);
      next = NULL;
    };
    next = _next;
  }
  // Dtor
  ~GsmLine() {
    if (line)  free(line);
    if (next)  delete next;
  }
  // Assign a string char* directly with '=' operator
  GsmLine &operator=(const char *new_line) {
    if (line) {
      free(line);
      line = NULL;
      length = 0;
    }
    if (new_line) {
      line = strdup(new_line);
      length = strlen(new_line);
    }
    return *this;
  }

  // Apend _next to end of linked list
  inline int append(GsmLine *_next) {
    GsmLine *t = this;
    while (t->next)
      t = t->getNext();
    t->setNext(_next);
    return 0;
  }
// Append an new elt at pos 'pos'
  inline int append(GsmLine *_next, int pos) {
    GsmLine *t = this, *tt;
    if (pos <= 0)
      return -1;
    while (t->getNext() && --pos) t = t->getNext();
    tt = t->getNext();
    t->setNext(_next);
    _next->setNext(tt);
    return 0;
  }
// Append a new line to the linked list, with char* as input
  inline int append(const char *_line) {
    if (line == NULL)
      return setData(_line);
    else
      return this->append(new GsmLine(_line));
  };

  // Return length of the line
  inline int strLength() {
    return length;
  }
  // Return length of all the lines of the linked list.
  inline int fullLength(bool withCRLF = false) {
    int l = 0;
    GsmLine *t = this;
    while (t) {
      l += this->strLength();
      l += (withCRLF ? 2 : 0);
      t = t->getNext();
    }
    return l;
  }
// Return a pointer to the line containing data
  inline char *getData() {
    return this->line;
  }

  // Return a pointer to the next line of the list
  inline GsmLine *getNext() {
    return this->next;
  }

  // Return a pointer to the next line of the list
  inline void setNext(GsmLine *next) {
    this->next = next;
  }

//Return the last line of the linked list
  char *getLastLine() {
    GsmLine *t = this;
    while (t->getNext()) t = t->getNext();
    return t->getData();
  }

  // Return a pointer to the next line of the list
  int setData(const char *data, bool erase = false) {
    if (line && !erase) return -1;
    if (line) {
      free(line);
      length = 0;
    }
    if (data) {
      line = strdup(data);
      length = strlen(data);
    }
    return 0;
  };

  // Return all the lines concatenated together, separated by <CR><LF>
  char *concatenateAll() {
    int len = this->fullLength(true) + 1;
    char *res = (char*)malloc(len * sizeof (char));
    GsmLine *t = this;
    memset(res, 0, len * sizeof(char));
    if (!res)  return NULL;
    while (t) {
      strcat(res, t->getData());
      res[strlen(res)] = LF;
      t = t->getNext();
    }
    return res;
  }

  // Display all lines
  inline void displayAll(std::ostream &s) {
    GsmLine *t = this;
    while (t) {
      if (t->strLength())
        s << t->getData() << LF;
      t = t->getNext();
    }
  }

  inline void reset() {
    delete line;
    line = 0;
    length = 0;
  }

private:
  // hold the text data
  char *line;
  // Length of the text data
  int length;
  // Ptr to the next line
  GsmLine *next;
};


#endif /* GSMLINE_H_ */
