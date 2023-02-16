#include "auth.h"
#include "user.h"

#include <stddef.h>

#include <zephyr/kernel.h>

const struct user users_list[] = {
	{
		.name = "",
		.role = USER_ROLE_UNAUTHENTICATED,
	},
	{
		.name = "caniot-lucas-dev",
		.role = USER_ROLE_ADMIN,
	},
	{
		.name = "caniot-lucas-user",
		.role = USER_ROLE_USER,
	},
};

const size_t users_count = ARRAY_SIZE(users_list);