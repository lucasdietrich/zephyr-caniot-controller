/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "auth.h"
#include "user.h"

#include <string.h>

#include <zephyr/kernel.h>

extern size_t strnlen(const char *, size_t);

const struct user *user_auth_verify(struct user_auth *auth)
{
	/* Unauthenticated user */
	size_t user_len;
	const struct user *user = user_get_unauthenticated_user();

	for (size_t i = 1u; i < users_count; i++) {
		user_len = strnlen(users_list[i].name, USER_NAME_MAX);
		if (strncmp(users_list[i].name, auth->username, user_len) == 0 &&
		    auth->username[user_len] == '\0') {
			user = &users_list[i];
			break;
		}
	}

	return user;
}

const struct user *user_get_unauthenticated_user(void)
{
	return &users_list[0u];
}