--
-- Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
--
-- SPDX-License-Identifier: Apache-2.0
--

require("math")
require("io")

print("somecalc1.lua")
print(os.time())

mylist = {}

N = 2000
for i = 1, N do
    -- random number between 0 and 100
    rdm = math.random(0, 1000)
    mylist[i] = rdm
end

sum = 0
for i = 1, N do
    sum = sum + mylist[i]
end

print(sum / (1000*N))
