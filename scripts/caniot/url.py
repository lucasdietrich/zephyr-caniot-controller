from __future__ import annotations

from typing import List, Optional, Union

import urllib.parse


class URL:
    def __init__(self, hostname: str, path: Union[str, List[str]] = [], query: str = "", secure: bool = False):
        self.hostname = hostname

        if isinstance(path, list):
            path = "/".join(path)

        self.path = path
        self.query = query
        self.secure = secure
        self.proto = "https" if secure else "http"

    def get(self) -> str:
        return urllib.parse.urlunparse((
            self.proto,
            self.hostname,
            self.path,
            None,
            self.query,
            None
        ))

    def sub(self, path: str = "") -> URL:
        result = urllib.parse.urlparse(
            urllib.parse.urljoin(self.get() + "/", str(path)))

        return URL(self.hostname, result.path, result.query, secure=self.secure)

    def __add__(self, path: str) -> URL:
        assert isinstance(path, str)
        return self.sub(path)

    def project(self, **kwargs) -> str:
        return self.get().format(**kwargs)

    def __call__(self, **kwargs) -> str:
        return self.project(**kwargs)

    def __str__(self) -> str:
        return self.get()

    def __repr__(self) -> str:
        return self.__str__()

if __name__ == "__main__":
    url = URL("caniotctrl.local", ["config", "{section}"], "n={n}", True)
    res = url(section="network", n=18)
    print(res)