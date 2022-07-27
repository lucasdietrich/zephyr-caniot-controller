--
-- Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
--
-- SPDX-License-Identifier: Apache-2.0
--

require("math")
require("io")

-- pseudo random number generator
for i=1,10 do
    io.write(math.random( 0, 100 ) .. " ")
end

print("ok")

-- calling another lua script
dofile("/RAM:/lua/helloworld.lua")
dofile("/RAM:/lua/math.lua")