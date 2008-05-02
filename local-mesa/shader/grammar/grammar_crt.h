#ifndef GRAMMAR_CRT_H
#define GRAMMAR_CRT_H


#include <stdlib.h>
#ifndef FIRTREE
#include <malloc.h>
#endif
#include <string.h>


typedef unsigned long grammar;
typedef unsigned char byte;


#define GRAMMAR_PORT_INCLUDE 1
#include "grammar.h"
#undef GRAMMAR_PORT_INCLUDE


#endif

