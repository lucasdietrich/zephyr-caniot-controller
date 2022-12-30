#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

target_sources(app PRIVATE usb.c)
target_sources_ifdef(CONFIG_USB_CDC_ACM app PRIVATE usb_cdc_acm.c)
target_sources_ifdef(CONFIG_USB_DEVICE_NETWORK app PRIVATE usb_net.c)