/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>

#include "contig_alloc.h"

int contig_block_init(struct contig *g, uint8_t *buffer, size_t size)
{
	if (!g || !buffer || !size) {
		return -EINVAL;
	}
	
	g->allocated = 0;
	g->size = size;
	g->buf = buffer;
	return 0;
}

int contig_remaining(struct contig *g)
{
	if (!g) {
		return -EINVAL;
	}
	return g->size - g->allocated;
}

int contig_block_reset(struct contig *g)
{
	g->allocated = 0;
	return 0;
}

void *contig_alloc(struct contig *g, size_t size)
{
	if (!g || !size) {
		return NULL;
	}
	if (g->allocated + size > g->size) {
		return NULL;
	}
	void *section = g->buf + g->allocated;
	g->allocated += size;
	return section;
}

int kcontig_init(struct kcontig *g, uint8_t *buffer, size_t size)
{
	if (!g || !buffer || !size) {
		return -EINVAL;
	}
	// check if buffer is aligned to 4 bytes
	if (((uintptr_t)buffer & 0x3) != 0) {
		return -EINVAL;
	}
	g->allocated = 0;
	g->size = size;
	g->buf = buffer;
	sys_dlist_init(&g->dlist);
	return 0;
}

int kcontig_remaining(struct kcontig *g)
{
	if (!g) {
		return -EINVAL;
	}
	return g->size - g->allocated;
}

int kcontig_reset(struct kcontig *g)
{
	g->allocated = 0;
	sys_dlist_init(&g->dlist);
	return 0;
}

void *kcontig_alloc(struct kcontig *g, size_t size)
{
	if (!g || !size) {
		return NULL;
	}

	/* Round up to upper 4 bytes */
	size = (size + 3) & ~0x3;

	/* Add required size for the dlist */
	size += sizeof(struct kcontig_block);

	if (g->allocated + size > g->size) {
		return NULL;
	}
	struct kcontig_block *block = (struct kcontig_block *) g->buf + g->allocated;
	sys_dlist_append(&g->dlist, &block->handle);
	g->allocated += size;
	return block;
}

void *kcontig_remove(struct kcontig_block *b)
{
	if (!b) {
		return NULL;
	}
	sys_dlist_remove(&b->handle);
	return b;
}

int kcontig_iterate(struct kcontig *g,
		    bool (*cb)(struct kcontig_block *b, void *),
		    void *user_data)
{
	if (!g || !cb) {
		return -EINVAL;
	}
	int ret = 0;
	struct kcontig_block *_dnode, *block;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&g->dlist, block, _dnode, handle)
	{
		if (!cb(block, user_data)) {
			break;
		}
		ret++;
	}
	return ret;
}