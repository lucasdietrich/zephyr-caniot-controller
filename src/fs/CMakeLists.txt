#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#


target_sources(app PRIVATE app_utils.c)
target_sources_ifdef(CONFIG_APP_FS_ASYNC_OPERATIONS app PRIVATE asyncrw.c)