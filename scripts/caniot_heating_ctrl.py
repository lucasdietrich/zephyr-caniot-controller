from pprint import pprint
from caniot.controller import Controller

ip = "192.168.10.240"

ctrl = Controller(ip, False)

CANIOT_HEATER_NONE = 0
CANIOT_HEATER_CONFORT = 1
CANIOT_HEATER_CONFORT_MIN_1 = 2
CANIOT_HEATER_CONFORT_MIN_2 = 3
CANIOT_HEATER_ENERGY_SAVING = 4
CANIOT_HEATER_FROST_FREE = 5
CANIOT_HEATER_OFF = 6

payload = [CANIOT_HEATER_CONFORT_MIN_1]

did = 0x5 << 3

# ctrl.command(did, 0x0, payload)

# ctrl.can(0x741)

# ctrl.request_telemetry(did, 0x3)

ctrl.read_attribute(did, 0x3)