from enum import IntEnum

class PrometheusType(IntEnum):
    Counter = 1
    Gauge = 2
    Histogram = 3
    Summary = 4

# BLE/CANIOT, EMB/EXT, BLE MAC/CANIOT DID, room name, f429/raspberry
device_measurement_tags = ["device_type", "sensor_type", "mac", "room", "collector"]

model = {
    "caniot": {
        "f429": {
            "net": {
                "tx_pkt": [PrometheusType.Counter, "Packets sent"],
                "rx_pkt": [PrometheusType.Counter, "Packets received"],
                "tx_byte": [PrometheusType.Counter, "Bytes sent"],
                "rx_byte": [PrometheusType.Counter, "Byte received"],
            },
            "cpu": {
                "usage": [PrometheusType.Gauge, "CPU Usage"],
                "uptime": [PrometheusType.Gauge, "CPU Uptime"],
            },
        }
    },
    "device": {
        "count": [PrometheusType.Gauge, "Device count"],
        "temperature": [PrometheusType.Gauge, "Device temperature (in °C)", device_measurement_tags],
        "humidity":  [PrometheusType.Gauge, "Device humidity (in %)", device_measurement_tags],
        "battery_voltage": [PrometheusType.Gauge, "Device battery voltage (in V)", device_measurement_tags],
        "measurements_last_timestamp": [PrometheusType.Gauge, "Device last measurement timestamp (UTC time)", device_measurement_tags],
    },
}


def generate_metrics(model: dict, prefix: str = "", mute: bool = False):
    for ns, val in model.items():
        if prefix != "":
            metric = "_".join((prefix, ns))
        else:
            metric = ns

        if isinstance(val, dict):
            yield from generate_metrics(val, metric, mute)
        elif isinstance(val, list):
            count = len(val)

            if count >= 2:
                if not mute:
                    type, desc = val[:2]
                    yield f"# HELP {metric} {desc}"
                    yield f"# TYPE {metric} {type.name.lower()}"
            else:
                raise Exception(f"Invalid metric definition: {val}")

            params = ""

            if count == 3:
                if isinstance(val[2], list) and len(val[2]) > 0:
                    params = "{" + ",".join(f'{param}="XXX"' for param in val[2]) + "}"
            
            yield f"{metric}{params} 0.0"
            
        else:
            raise Exception(f"Invalid model, ns = {metric}")

if __name__ == "__main__":
    content = "\n".join(generate_metrics(model, mute=True))

    print("length = ", len(content))

    print(content)