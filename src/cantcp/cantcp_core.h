#ifndef _CANTCP_CORE_H_
#define _CANTCP_CORE_H_

#include <kernel.h>

/*___________________________________________________________________________*/

typedef enum
{
	CONTROL_FRAME = 0,
	FILTER_FRAME = 1,
	DATA_FRAME = 2,
	ERROR_FRAME = 3,
} cantcp_frame_t;

typedef struct
{
	uint32_t frame_type : 2;
	uint32_t rtr : 1;
	uint32_t id_type : 1; 		/* standard or extended */
	uint32_t dlc : 4;	  	/* can data length */
} cantcp_header_t;

/*___________________________________________________________________________*/

#endif /* _CANTCP_CORE_H_ */