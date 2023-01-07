/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LUA_LIBC_STUBS_H_
#define _LUA_LIBC_STUBS_H_

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <sys/times.h>

#define DUMMY_STDLIB_FUNCTIONS
#ifdef DUMMY_STDLIB_FUNCTIONS

/* https://man7.org/linux/man-pages/man2/times.2.html */
clock_t _times(struct tms *tms)
{
	__ASSERT(0, "_times not implemented");
	return (clock_t)-1;
}

/* https://man7.org/linux/man-pages/man2/unlink.2.html */
/* http://manpagesfr.free.fr/man/man2/unlink.2.html */
int _unlink(const char *pathname)
{
	__ASSERT(0, "_unlink not implemented");
	return -1;
}

/* https://man7.org/linux/man-pages/man2/link.2.html */
int _link(const char *oldpath, const char *newpath)
{
	__ASSERT(0, "_link not implemented");
	return -1;
}
#endif

#endif /* _LUA_LIBC_STUBS_H_ */