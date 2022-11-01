/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_CONTIG_ALLOC_H_
#define _UTILS_CONTIG_ALLOC_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/dlist.h>

#define Z_CONFIG_BUF_SYM(_name) \
	(_contig_buf ## _name)

struct contig {
	size_t allocated;
	size_t size;
	char *buf;
};

#define CONTIG_INIT(_buf, _size) \
	{ \
		.allocated = 0u, \
		.size = _size, \
		.buf = _buf, \
	}

#define CONTIG_BUF_DEFINE(_name, _size) \
	static __aligned(4u) char Z_CONFIG_BUF_SYM(_name)[_size];\
	struct contig _name = { \
		.allocated = 0u, \
		.size = _size, \
		.buf = Z_CONFIG_BUF_SYM(_name), \
	}

#define CONTIG_DEFINE(_name, _buf, _size) \
	struct contig name = CONTIG_INIT(_buf, _size)

int contig_init(struct contig *g, uint8_t *buffer, size_t size);

int contig_remaining(struct contig *g);

int contig_reset(struct contig *g);

void *contig_alloc(struct contig *g, size_t size);

struct kcontig {
	size_t allocated;
	size_t size;
	char *buf;
	sys_dlist_t dlist;
};

#define KCONTIG_DEFINE(_name, _buf, _size) \
	struct kcontig name = { \
		.allocated = 0u, \
		.size = _size, \
		.buf = _buf, \
		.dlist = SYS_DLIST_STATIC_INIT(&name.dlist), \
	}

#define KCONTIG_BUF_DEFINE(_name, _size) \
	static __aligned(4u) char Z_CONFIG_BUF_SYM(_name)[_size];\
	struct kcontig _name = { \
		.allocated = 0u, \
		.size = _size, \
		.buf = Z_CONFIG_BUF_SYM(_name), \
		.dlist = SYS_DLIST_STATIC_INIT(&_name.dlist), \
	}

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