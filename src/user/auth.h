/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USER_AUTH_H_
#define _USER_AUTH_H_

#include "user.h"

struct user_auth {
    char username[USER_NAME_MAX + 1u];
};

const struct user *user_auth_verify(struct user_auth *auth);

const struct user* user_get_unauthenticated_user(void);

#endif /* _USER_AUTH_H_ */