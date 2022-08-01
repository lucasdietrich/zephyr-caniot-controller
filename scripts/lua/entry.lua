--
-- Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
--
-- SPDX-License-Identifier: Apache-2.0
--

require("math")
require("io")
require("dummy")

print("Lua entry point")

n, m = 1, 1

for i = 1, 100000 do
    n, m = m, n + m
end

print("n=" .. n .. " m=" .. m)

print(math.random(1, 10))

dummy.myhelloworld()
