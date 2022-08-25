--
-- Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
--
-- SPDX-License-Identifier: Apache-2.0
--

require("math")
require("io")
require("ha")

print("ha.lua")

-- build a structure in lua
local s = {}
s.num = 2
s.city_id = 234
s.area_id = 2345
s.name = "Hamburg"

print(s.area_id .. s.name)