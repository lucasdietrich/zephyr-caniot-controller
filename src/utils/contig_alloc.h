/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_CONTIG_ALLOC_H_
#define _UTILS_CONTIG_ALLOC_H_

#include <stddef.h>
#include <stdint.h>

#include <sys/dlist.h>

struct contig {
	size_t allocated;
	size_t size;
	uint8_t *buf;
};

int contig_init(struct contig *g, uint8_t *buffer, size_t size);

int contig_remaining(struct contig *g);

int contig_reset(struct contig *g);

void *contig_alloc(struct contig *g, size_t size);

struct kcontig {
	size_t allocated;
	size_t size;
	uint8_t *buf;
	sys_dlist_t dlist;
};

struct kcontig_block {
	sys_dnode_t handle;
	char buf[];
};

int kcontig_init(struct kcontig *g, uint8_t *buffer, size_t size);

int kcontig_remaining(struct kcontig *g);

int kcontig_reset(struct kcontig *g);

void *kcontig_alloc(struct kcontig *g, size_t size);

void *kcontig_remove(struct kcontig_block *b);

int kcontig_iterate(struct kcontig *g,
		    bool (*cb)(struct kcontig_block *b, void *),
		    void *user_data);

#endif /* _UTILS_CONTIG_ALLOC_H_ */