#ifndef COMMON_H
#define COMMON_H

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_ds.h"

typedef struct {
	size_t start;
	size_t end;
	char *source;
} Slice;

typedef struct ArenaBlock {
	size_t pos;
	size_t cap;
	max_align_t data[];
} ArenaBlock;

typedef struct {
	ArenaBlock **blocks; // stb_ds arraylist
	ArenaBlock *current;
	ptrdiff_t current_index;

} Arena;

typedef struct {
	size_t block_index;
	size_t pos;
} ArenaCheckpoint;

void *alloc(Arena *arena, size_t size);
void destroy_arena(Arena *arena);
void reset_arena(Arena *arena);
ArenaCheckpoint arena_save(Arena *arena);
void arena_restore(Arena *arena, ArenaCheckpoint cp);
int slice_equals(Slice a, Slice b);

#endif
