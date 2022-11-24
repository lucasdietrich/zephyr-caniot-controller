#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

target_sources(app PRIVATE manager.c utils.c)

target_sources_ifdef(CONFIG_APP_CREDS_HARDCODED app PRIVATE hardcoded_creds.c hardcoded_creds_g.c)
target_sources_ifdef(CONFIG_APP_CREDS_FLASH app PRIVATE flash_creds.c)
target_sources_ifdef(CONFIG_APP_CREDS_FS app PRIVATE fs_creds.c)