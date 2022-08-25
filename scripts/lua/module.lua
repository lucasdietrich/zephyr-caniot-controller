--
-- Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
--
-- SPDX-License-Identifier: Apache-2.0
--

require("math")
require("io")
require("dummy")

print("module.lua")

-- Custom module
res = dummy.hello("Lucas")
print(string.format("hello: %s", res))

res = dummy.add(1, 2, 3, 4)
print(string.format( "add: %s", res))

res = dummy.misc("hello", 2.3)
print(string.format( "misc: %s", res))

-- OS module

-- res = os.clock()
-- print(string.format( "clock: %s", res))

res = os.date()
print(string.format( "date: %s", res))

res = os.difftime(os.time(), 1661461268)
print(string.format( "difftime: %s", res))

-- res = os.execute()
-- print(string.format( "execute: %s", res))

-- res = os.exit(1)
-- print(string.format( "exit: %s", res))

-- res = os.getenv()
-- print(string.format( "getenv: %s", res))

-- res = os.remove()
-- print(string.format( "remove: %s", res))

-- res = os.rename()
-- print(string.format( "rename: %s", res))

-- res = os.setlocale()
-- print(string.format( "setlocale: %s", res))

res = os.time()
print(string.format( "time: %s", res))

-- res = os.tmpname()
-- print(string.format( "tmpname: %s", res))

-- BASE module
res = warn("Warning", "hell")
print(string.format( "warn: %s", res))
