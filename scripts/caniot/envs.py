#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from dataclasses import dataclass

@dataclass
class Env:
    name: str
    ip: str
    ip_fwd: str = None

    def __repr__(self) -> str:
        fwd_str = "" if self.ip_fwd is None else f" fwd -> {self.ip_fwd}"
        c = [
            f"{self.name.ljust(15)} ({self.ip}{fwd_str})",
            "\t" + "discovery = on",
            "\t" + "http = on",
            "\t" + "rest = on"
        ]

        return "\n".join(c)


envs = [
    Env(
        name="nucleo_f429zi",
        ip="192.168.10.240"
    ),
    Env(
        name="qemu_x86",
        ip="192.0.2.1",
        ip_fwd="192.168.10.216"
    ),
]

if __name__ == "__main__":
    print("Environments:")
    for env in envs:
        print(env)
