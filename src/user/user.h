#ifndef _USER_H_
#define _USER_H_

#include <stddef.h>

#define USER_NAME_MAX 32u

typedef enum user_role {
	USER_ROLE_UNAUTHENTICATED,
	USER_ROLE_USER,
	USER_ROLE_ADMIN,
} user_role_t;

struct user {
	char name[USER_NAME_MAX + 1u];
	user_role_t role;
};

const char *user_role_to_str(user_role_t role);

extern const struct user users_list[];
extern const size_t users_count;

#endif /* _USER_AUTH_INTERNAL_H_ */