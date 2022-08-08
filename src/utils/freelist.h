#ifndef _UTILS_FREE_LIST_H_
#define _UTILS_FREE_LIST_H_

#include <stdio.h>

#include <zephyr.h>
#include <sys/slist.h>

struct _flist_info {
	sys_slist_t *free_list;
	void *mem;
	size_t block_size;
	size_t block_count;
};

#define Z_FREE_LIST_BUF_SYM(_name) \
	_free_list_buf_ ## _name

#define Z_FREE_LIST_INFO_SYM(_name) \
	_free_list_info_ ## _name

#define FREE_LIST_DECLARE(_name, _type, _count) \
	Z_DECL_ALIGN(sys_slist_t) _name = SYS_SLIST_STATIC_INIT(&_name); \
	BUILD_ASSERT(sizeof(_type) % 4 == 0); \
	BUILD_ASSERT(_count > 0); \
	static Z_DECL_ALIGN(_type) Z_FREE_LIST_BUF_SYM(_name)[_count]; \
	static STRUCT_SECTION_ITERABLE(_flist_info, Z_FREE_LIST_INFO_SYM(_name)) = { \
		.free_list = &_name, \
		.mem = Z_FREE_LIST_BUF_SYM(_name), \
		.block_size = sizeof(_type), \
		.block_count = _count \
	};

#define FREE_LIST_DECLARE_STATIC(_name, _type, _count) \
	static FREE_LIST_DECLARE(_name, _type, _count)

/**
 * @brief Initialize a free list in @param mem 
 * 
 * @param mem Memory to initialize
 * @param block_size Size of each block in the free list,
 *   should be a multiple of sizeof(void*)
 * @param block_count Number of blocks in the free list
 * @return int 
 */
int free_list_init(sys_slist_t *fl,
		   void *const mem,
		   size_t block_size,
		   size_t block_count);

/**
 * @brief Allocate a block from the free list
 * 
 * @param fl 
 * @return void* 
 */
void *free_list_alloc(void *fl);

/**
 * @brief Free a block to the free list
 * 
 * @param fl 
 * @param block 
 */
void free_list_free(void *fl, void *block);

/**
 * @brief Count the number of free blocks in the free list
 * 
 * @param fl 
 * @return int 
 */
int free_list_count(void *fl);

/**
 * @brief Initialize all free lists in the system
 */
void sys_free_lists_init(void);

#endif /* _UTILS_FREE_LIST_H_ */