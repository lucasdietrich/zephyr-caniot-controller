from caniot.discovery import DiscoveryClient

client = DiscoveryClient()

addr, raw, parsed = client.lookup()

print(f"Address {addr} responded with {len(raw)} bytes : parse = {parsed}")