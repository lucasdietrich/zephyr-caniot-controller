from pprint import pprint
from caniot.test import  TestClient, data_gen_zeros, ChunksGeneratorType

ip = "192.0.2.1"
ip = "192.168.10.240"

t = TestClient(ip, False)

# t.test_simultaneous(7, 50)

# t.test_session(3)

def simple_chunk_generator(size: int, chunks_size: int = 1024) -> ChunksGeneratorType:
    sent = 0
    while sent < size:
        tosend = min(chunks_size, size - sent)
        yield bytes([1 for _ in range(tosend)])
        sent += tosend

res = t.test_stream(simple_chunk_generator(128*1024, 2048))


# res = t.test_multipart(2048*1024, 4048, [1024, 10, 234, 342])

# t.test_route_args(11312, 234, 2)

# res = t.test_big_data(10*1024)

print(res)
pprint(res.json())