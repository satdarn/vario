#include "../shared/common.h"

void *alloc(Arena *arena, size_t size) {
	if (size == 0 || !arena)
		return NULL;
	size_t aligned = (size + sizeof(max_align_t) - 1) & ~(sizeof(max_align_t) - 1);

	if (arena->current && arena->current->pos + aligned <= arena->current->cap) {
		void *ptr = (char *) arena->current->data + arena->current->pos;
		arena->current->pos += aligned;
		return memset(ptr, 0, aligned);
	}
	int block_count = arrlen(arena->blocks);
	int next_index = arena->current ? arena->current_index + 1 : 0;
	if (next_index < block_count && arena->blocks[next_index]->cap >= aligned) {
		ArenaBlock *block = arena->blocks[next_index];
		block->pos = aligned;
		arena->current = block;
		arena->current_index = next_index;
		return memset(block->data, 0, aligned);
	}
	size_t new_cap = aligned > 4096 ? aligned : 4096;
	ArenaBlock *block = malloc(sizeof(ArenaBlock) + new_cap);
	if (!block)
		return NULL;
	block->cap = new_cap;
	block->pos = aligned;
	if (next_index < block_count) {
		//  A leftover block exists but was too small - splice the new
		//  one in ahead of it instead of appending, so array order keeps
		//  matching allocation order. The too-small block shifts right
		//  and stays available for reuse later.
		arrins(arena->blocks, next_index, block);
	} else {
		arrpush(arena->blocks, block);
	}
	arena->current_index = next_index;
	arena->current = block;
	return memset(block->data, 0, aligned);
}

void destroy_arena(Arena *arena) {
	if (!arena)
		return;
	size_t count = arrlen(arena->blocks);
	for (size_t i = 0; i < count; i++) {
		free(arena->blocks[i]);
	}
	arrfree(arena->blocks);
	arena->current = NULL;
	arena->current_index = 0;
}

void reset_arena(Arena *arena) {
	if (!arena)
		return;
	size_t block_count = arrlen(arena->blocks);
	for (size_t i = 0; i < block_count; i++) {
		arena->blocks[i]->pos = 0;
	}
	arena->current = arena->blocks[0];
	arena->current_index = 0;
}

ArenaCheckpoint arena_save(Arena *arena) {
	return (ArenaCheckpoint) {.block_index = arena->current_index,
							  .pos = arena->current ? arena->current->pos : 0};
}

void arena_restore(Arena *arena, ArenaCheckpoint cp) {
	if (!arena)
		return;
	size_t block_count = arrlen(arena->blocks);
	if (cp.block_index >= block_count)
		return;
	for (size_t i = cp.block_index; i < block_count; i++) {
		arena->blocks[i]->pos = 0;
	}
	arena->blocks[cp.block_index]->pos = cp.pos;
	arena->current = arena->blocks[cp.block_index];
	arena->current_index = cp.block_index;
}
