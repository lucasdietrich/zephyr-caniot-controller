import socket

CANTCP_DEFAULT_PORT = 5555

class CanTcpServer:
    def __init__(self, port: int = CANTCP_DEFAULT_PORT) -> None:
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind(('', self.port))
        self.sock.listen(1)
        sock, addr = self.sock.accept()

        print(sock, addr)

        b = sock.recv(1024)
        sock.send(b)

        sock.close()


if __name__ == "__main__":
    srv = CanTcpServer()
