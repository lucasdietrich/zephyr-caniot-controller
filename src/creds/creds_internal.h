#ifndef _CREDS_INTERNAL_H
#define _CREDS_INTERNAL_H

#include "manager.h"

struct creds_api {
	int (*init)(void);
	int (*get)(cred_id_t id, struct cred *c);
	int (*copy_to)(cred_id_t id, char *buf, size_t *size);
	int (*iterate)(struct cred *c, void *user_data);
};

#define CRED_API_INIT(_init, _get, _copy_to, _iterate)                                   \
	{                                                                                \
		.init = _init, .get = _get, .copy_to = _copy_to, .iterate = _iterate,    \
	}

#define CREDS_API_DEFINE(_name, _init, _get, _copy_to, _iterate)                         \
	static STRUCT_SECTION_ITERABLE(creds_api, _name) =                               \
		CRED_API_INIT(_init, _get, _copy_to, _iterate)

#endif /* _CREDS_INTERNAL_H */