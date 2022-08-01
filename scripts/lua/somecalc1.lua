--
-- Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
--
-- SPDX-License-Identifier: Apache-2.0
--

require("math")
require("io")

print("somecalc1.lua")
print(os.time())

-- define an empty list
mylist = {}

N = 1000
for i = 1, N do
    -- random number between 0 and 100
    rdm = math.random(0, 100)
    mylist[i] = rdm
    -- print(rdm)
end

sum = 0
for i = 1, N do
    sum = sum + mylist[i]
end

print(sum)
print(sum / N)
