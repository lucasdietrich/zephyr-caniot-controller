#ifndef _UTILS_FREE_LIST_H_
#define _UTILS_FREE_LIST_H_

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

struct _flist_info {
	sys_slist_t *free_list;
	void *mem;
	size_t block_size;
	size_t block_count;
};

#define Z_FREE_LIST_BUF_SYM(_name) _free_list_buf_##_name

#define Z_FREE_LIST_INFO_SYM(_name) _free_list_info_##_name

/**
 * @brief Define a buffer and a free list.
 *
 * Example:
 *   FREE_LIST_DEFINE(fl, struct my_type, 10u);
 */
#define FREE_LIST_DEFINE(_name, _type, _count)                                           \
	Z_DECL_ALIGN(sys_slist_t) _name = SYS_SLIST_STATIC_INIT(&_name);                 \
	BUILD_ASSERT(sizeof(_type) % 4 == 0);                                            \
	BUILD_ASSERT(_count > 0);                                                        \
	static Z_DECL_ALIGN(_type) Z_FREE_LIST_BUF_SYM(_name)[_count];                   \
	static STRUCT_SECTION_ITERABLE(_flist_info, Z_FREE_LIST_INFO_SYM(_name)) = {     \
		.free_list   = &_name,                                                   \
		.mem	     = Z_FREE_LIST_BUF_SYM(_name),                               \
		.block_size  = sizeof(_type),                                            \
		.block_count = _count};

/**
 * @brief Declare a buffer and define a free list for list
 *
 * Example:
 *   uint8_t buf[10u*sizeof(struct my_type)];
 *   FREE_LIST_DECLARE(fl, struct my_type, buf, 10u);
 */
#define FREE_LIST_DECLARE(_name, _type, _buf, _count)                                    \
	Z_DECL_ALIGN(sys_slist_t) _name = SYS_SLIST_STATIC_INIT(&_name);                 \
	BUILD_ASSERT(sizeof(_type) % 4 == 0);                                            \
	BUILD_ASSERT(_count > 0);                                                        \
	static STRUCT_SECTION_ITERABLE(_flist_info, Z_FREE_LIST_INFO_SYM(_name)) = {     \
		.free_list   = &_name,                                                   \
		.mem	     = (void *)_buf,                                             \
		.block_size  = sizeof(_type),                                            \
		.block_count = _count};

/**
 * @brief Declare an array and define a free list for list
 *
 * Example:
 *   struct my_type array[10u];
 *   FREE_LIST_DECLARE(fl, array);
 */
#define FREE_LIST_DECLARE_TYPED_ARRAY(_name, _arr)                                       \
	Z_DECL_ALIGN(sys_slist_t) _name = SYS_SLIST_STATIC_INIT(&_name);                 \
	BUILD_ASSERT(sizeof(_arr[0]) % 4 == 0);                                          \
	static STRUCT_SECTION_ITERABLE(_flist_info, Z_FREE_LIST_INFO_SYM(_name)) = {     \
		.free_list   = &_name,                                                   \
		.mem	     = (void *)_arr,                                             \
		.block_size  = sizeof(_arr[0]),                                          \
		.block_count = ARRAY_SIZE(_arr)};

#define FREE_LIST_DEFINE_STATIC(_name, _type, _count)                                    \
	static FREE_LIST_DEFINE(_name, _type, _count)

#define FREE_LIST_DECLARE_STATIC(_name, _type, _buf, _count)                             \
	static FREE_LIST_DECLARE(_name, _type, _buf, _count)

#define FREE_LIST_DECLARE_TYPED_ARRAY_STATIC(_name, _arr)                                \
	static FREE_LIST_DECLARE_TYPED_ARRAY(_name, _arr)

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