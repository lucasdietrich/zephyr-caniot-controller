# Parse configurations and generate a table of configuration options

import re
from pprint import pprint
from typing import Dict, List, Tuple, Any, OrderedDict, Set
from collections import OrderedDict
from copy import copy, deepcopy


def parse_config_file(prjconf: str, config: Dict = None) -> Dict:
    rec_cfg_opt = re.compile(r'^CONFIG_(?P<option>[A-Z0-9_]+)=(?P<value>.*)\s*(#.*)?$')
    rec_cfg_val_int = re.compile(r'^(?P<int>-?[0-9]+)\s*(#.*)?$')
    rec_cfg_val_hex = re.compile(r'^(?P<hex>-?0x[0-9a-fA-F]+)\s*(#.*)?$')
    rec_cfg_val_yn = re.compile(r'^(?P<bool>y|n)\s*(#.*)?$')
    rec_cfg_val_str = re.compile(r'^"(?P<str>.*)"\s*(#.*)?$')

    if config is None:
        config = {}

    with open(prjconf, 'r') as f:
        for line in f.readlines():
            m = rec_cfg_opt.match(line)
            if m:
                option = m.group("option")
                value = m.group("value")

                m = rec_cfg_val_int.match(value)
                if m:
                    value = int(m.group("int"))
                else:
                    m = rec_cfg_val_hex.match(value)
                    if m:
                        value = int(m.group("hex"), 16)
                    else:
                        m = rec_cfg_val_yn.match(value)
                        if m:
                            value = m.group("bool") == "y"
                        else:
                            m = rec_cfg_val_str.match(value)
                            if m:
                                value = m.group("str")
                            else:
                                raise ValueError(
                                    "Invalid value for config option: " + line)

                config[option] = value
    
    return config

def parse_config_files(prjconfs: List[str], config: Dict = None) -> Dict:
    if config is None:
        config = {}

    for prjconf in prjconfs:
        parse_config_file(prjconf, config)

    return config

def gather_configurations(configurations: Dict) -> OrderedDict:
    configs = OrderedDict()

    for board, config in configurations.items():
        for option, value in config.items():
            if option not in configs:
                configs[option] = []
            configs[option].append((board, value))

    return dict(sorted(configs.items()))

def gconfig_remove_identical(gconfig: OrderedDict) -> OrderedDict:
    out = OrderedDict()

    for option, values in gconfig.items():
        if len(values) > 1:
            first = values[0][1]
            for board, value in values:
                if value != first:
                    out[option] = deepcopy(values)
                    break
        elif len(values) == 1:
            out[option] = deepcopy(values)

    return dict(sorted(out.items()))

def gconfig_list_boards(gconfig: OrderedDict) -> List[str]:
    boards = set()

    for option, values in gconfig.items():
        for board, value in values:
            boards.add(board)

    return list(boards)

def gconfig_filter_app_opt(gconfig: OrderedDict) -> OrderedDict:
    out = OrderedDict()

    for option, values in gconfig.items():
        if option.startswith("APP_"):
            out[option] = deepcopy(values)

    return dict(sorted(out.items()))

def gconfig_export_to_csv(gconfig: OrderedDict, filename: str, boards_names_order: List = None) -> None:
    if boards_names_order is None:
        boards_names_order = []

    boards_names = gconfig_list_boards(gconfig)
    boards_names = sorted(
        boards_names,
        key=lambda x: boards_names_order.index(x)
        if x in boards_names_order
        else len(boards_names_order))

    with open(filename, 'w') as f:
        f.write("option," + ",".join(boards_names) + "\n")
        
        for option, values in gconfig.items():
            line = "CONFIG_" + option + ","
            for board in boards_names:
                for b, value in values:
                    if b == board:
                        if isinstance(value, str):
                            value_len = len(value)
                            if value_len > 20:
                                value = value[:10] + "..."
                            value = '"' + value + '"'
                        line += str(value)
                    else:
                        line += ""
                line += ","
            line = line[:-1] + "\n"
            f.write(line)
            

if __name__ == "__main__":
    prj = "prj.conf"

    boards = {
        "nucleo_f429zi": [
            prj,
            "boards/nucleo_f429zi.conf",
            "overlays/nucleo_f429zi_spi_disk.conf",
            "overlays/nucleo_f429zi_mcuboot.conf",
        ],
        "mps2_an385": [
            prj,
            "boards/mps2_an385.conf",
            "overlays/mps2_an385_slip.conf",
        ],
        "qemu_x86": [
            prj,
            "boards/qemu_x86.conf",
        ],
    }

    configurations = {}

    boards_names_order = [
        "nucleo_f429zi",
        "mps2_an385",
        "qemu_x86",
    ]

    for board, conf_files in boards.items():
        configurations[board] = parse_config_files(conf_files)

    gconf_all = gather_configurations(configurations)

    gconf_diff = gconfig_remove_identical(gconf_all)
    gconfig_export_to_csv(
        gconf_diff,
        "./tmp/configurations.csv", 
        boards_names_order)
    print("Saved DIFF configurations to ./tmp/configurations.csv")

    gconf_app = gconfig_filter_app_opt(gconf_all)
    gconfig_export_to_csv(
        gconf_app,
        "./tmp/configurations_app.csv",
        boards_names_order)
    print("Saved APP configurations to ./tmp/configurations_app.csv")

    # pprint(gconfigurations)