# Zephyr Application developpement

- [Devicetree HOWTOs - Set devicetree overlays](https://docs.zephyrproject.org/latest/build/dts/howtos.html#set-devicetree-overlays)

        If the file boards/<BOARD>.overlay exists, it will be used.

- [Setting Kconfig configuration values - The Initial Configuration](https://docs.zephyrproject.org/3.0.0/guides/build/kconfig/setting.html#initial-conf)

        The application configuration can come from the sources below. By default, prj.conf is used.
        Otherwise if CONF_FILE is set, and a single configuration file of the form prj_<build>.conf is used, then if file boards/<BOARD>_<build>.conf exists in same folder as file prj_<build>.conf, the result of merging prj_<build>.conf and boards/<BOARD>_<build>.conf is used.