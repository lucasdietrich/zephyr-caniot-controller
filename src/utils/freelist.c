#include "freelist.h"

int free_list_init(sys_slist_t *fl,
		   void *const mem,
		   size_t block_size,
		   size_t block_count)
{
	/* check if list is aligned */
	if (!IS_PTR_ALIGNED(mem, sizeof(void *))) {
		return -EINVAL;
	}

	if (!fl || !mem || !block_count || !block_size || (block_size % 4u)) {
		return -EINVAL;
	}

	/* Init structure */
	sys_slist_init(fl);

	/* Add all blocks to the free list */
	for (uint32_t i = 0; i < block_count; i++) {
		uintptr_t block = (uintptr_t)mem + (i * block_size);
		sys_slist_append(fl, (void *)block);
	}

	return 0u;
}

void *free_list_alloc(void *fl)
{
	return (void *)sys_slist_get(fl);
}

void free_list_free(void *fl, void *block)
{
	if (!fl || !block) {
		return;
	}

	sys_slist_append(fl, block);
}


int free_list_count(void *fl)
{
	int count = 0;
	sys_snode_t *node;
	SYS_SLIST_FOR_EACH_NODE(fl, node) {
		count++;
	}
	return count;
}

void sys_free_lists_init(void)
{
	STRUCT_SECTION_FOREACH(_flist_info, finfo) {
		free_list_init(finfo->free_list, finfo->mem,
			       finfo->block_size, finfo->block_count);
	}
}