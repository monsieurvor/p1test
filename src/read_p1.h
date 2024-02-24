#ifndef _READ_P1_H_
#define _READ_P1_H_

#include "settings.h"

unsigned int crc16(unsigned int crc, unsigned char *buf, int len);
bool isNumber(char *res, int len);
int findCharInArrayRev(char array[], char c, int len);
long getValue(char *buffer, int maxlen, char startchar, char endchar);
/**
 *  Decodes the telegram PER line. Not the complete message. 
 */
bool decodeTelegram(int len);
bool readP1Serial(void);
void setupDataReadout(void);

struct TelegramDecodedObject
{
  String name;
  long value;
  char code[16];
  char startChar = '(';
  char endChar = ')';
};

#endif // _READ_P1_H_