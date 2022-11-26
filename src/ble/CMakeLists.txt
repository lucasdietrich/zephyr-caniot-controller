#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

SET(app_sources 
    ble.c
    observer.c
)

target_sources(app PRIVATE ${app_sources})

# target_sources_ifdef(CONFIG_APP_HTTP_TEST_SERVER app PRIVATE oberver_legacy.c)