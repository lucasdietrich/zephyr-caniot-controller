import time
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

CANIOT_SHUTTER_CMD_NONE = 0xff

def build_payload(h1=CANIOT_HEATER_NONE,
                  h2=CANIOT_HEATER_NONE,
                  h3=CANIOT_HEATER_NONE,
                  h4=CANIOT_HEATER_NONE,
                  s1=CANIOT_SHUTTER_CMD_NONE,
                  s2=CANIOT_SHUTTER_CMD_NONE,
                  s3=CANIOT_SHUTTER_CMD_NONE,
                  s4=CANIOT_SHUTTER_CMD_NONE):
    return [
        h1 & 0xF | (h2 & 0xF) << 4,
        h3 & 0xF | (h4 & 0xF) << 4,
        s1,
        s2,
        s3,
        s4,
    ]

did = 0x5 << 3

def cmd_shutter(**kwargs):
    ctrl.command(did, 0, build_payload(**kwargs))
    

def reset():
    cmd_shutter(s1 = 0, s2 = 0)

# step 75 25 to 25 100
def case1():
    cmd_shutter(s1 = 75, s2 = 25)
    time.sleep(2)
    cmd_shutter(s1 = 25, s2 = 100)
    time.sleep(2)
    reset()

def case2():
    cmd_shutter(s1 = 100)
    time.sleep(0.75)
    cmd_shutter(s1 = 0, s2 = 75)
    time.sleep(0.5)
    cmd_shutter(s1 = 25)
    time.sleep(1)
    cmd_shutter(s1 = 0, s2 = 100)
    time.sleep(1.5)
    reset()


# payload = build_payload(s1 = 0, s2 = 100)
# ctrl.command(did, 0x0, payload)

# ctrl.request_telemetry(did, 0)

# ctrl.can(0x741)


# ctrl.read_attribute(did, 0x3)