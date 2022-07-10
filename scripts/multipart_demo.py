import requests
from pprint import pprint
import hexdump

response = requests.post('http://httpbin.org/post', files=dict(foo='bar'), stream=True)
print(response.status_code)

pprint(response.json()['headers'])