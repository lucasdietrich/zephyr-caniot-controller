#include "user.h"

#include <zephyr/kernel.h>

const char *user_role_to_str(user_role_t role)
{
	switch (role) {
	case USER_ROLE_UNAUTHENTICATED:
		return "Unauthenticated";
	case USER_ROLE_USER:
		return "User";
	case USER_ROLE_ADMIN:
		return "Admin";
	default:
		return "<unknown role>";
	}
}