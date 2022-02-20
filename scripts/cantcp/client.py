import socket

CANTCP_DEFAULT_PORT = 5555

class CanTcpClient:
    def __init__(self, host: str, port: int = CANTCP_DEFAULT_PORT):
        self.host = host
        self.port = port

        self.rx_callback: None

    def connect(self):
        self.sock = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_TCP)
        self.sock.connect((self.host, self.port))

    def close(self):
        self.sock.close()

    def attach_rx_callback(self, callback):
        self.rx_callback = callback

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


if __name__ == "__main__":
    with CanTcpClient("192.168.10.240") as tunnel:
        pass