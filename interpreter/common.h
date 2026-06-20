#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>

#include "stb_ds.h"

typedef struct {
    size_t start;
    size_t end;
	char * source;
} Slice;

#endif 
