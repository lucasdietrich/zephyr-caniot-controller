#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

target_sources(app PRIVATE cloud.c backoff.c utils.c mqttc.c cloud_app.c)
target_sources_ifdef(CONFIG_AWS app PRIVATE aws.c)