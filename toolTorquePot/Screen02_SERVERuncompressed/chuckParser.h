#ifndef CHUCK_PARSER_H
#define CHUCK_PARSER_H

#ifndef CHUCK_PARSER_USE_SD
#include <FS.h>
#else
#include <SPI.h>
#include <SD.h>
#endif

#define NO_MODELS 2
#define NO_SIZES 10
#define NO_SHANKS 10
#define MODEL_STRLEN 25
#define PRE_STRLEN 10
#define SIZE_STRLEN 8 //VCK25 etc
#define SHANK_STRLEN 5

struct Chuck{
  int exists = 0;
  char model[MODEL_STRLEN];
  char prefix[PRE_STRLEN];
  char sizes[NO_SIZES][SIZE_STRLEN] = {{0}};
  char shanks[NO_SIZES][NO_SHANKS][SHANK_STRLEN] = {{{0}}};
  uint16_t torques[NO_SIZES][NO_SHANKS] = {{0}};
};

int readChuckFile(Chuck *, File *);
void printChuckData(Chuck *);
int readToCharCode(File *, char, int, char *);

#endif
