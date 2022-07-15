from caniot.test import  TestClient

t = TestClient("192.0.2.1", False)

# t.test_simultaneous(7, 50)

# t.test_session(3)

# t.test_stream(128*1024, 4048, [1024, 10, 234, 342])

t.test_route_args(11312, 234, 2)

# t.test_big_data(12*1024)