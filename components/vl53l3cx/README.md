# vl53l3cx
An ESPHome external component for the vl53l3cx TOF sensor.

```yaml
# example configuration:

sensor:
  - platform: vl53l3cx
    name: "Friendly name"
```

Configuration variables:
------------------------

- **update_interval** (*Optional*, time): The interval to initiate new readings 
  on the sensor. Defaults to ``60s``.
- **address** (*Optional*, int): Manually specify the i2c address of the sensor. Defaults to ``0x29``.
  If an address other the ``0x29`` is specified, the sensor will be dynamically re-addressed at startup.
  A dynamic re-address of sensor requires the ``enable_pin`` configuration variable to be assigned.
- **enable_pin** (*Optional*, Pin Schema): The pin connected to XSHUT
  on vl53l3cx to enable/disable sensor. **Required** if not using address ``0x29`` which is the cause if you
  have multiple VL53L0X on the same i2c bus. In this case you have to assign a different pin to each VL53L0X.
- **mode** (*Optional*, string): The vl53l3cx can detect the distance to multiple "objects"
  in a single reading. This variable defines which distance is chosen to be
  reported. One of ``Closest``, ``Furtherest``. Defaults to ``Closest``.

Actions:
--------

``vl53l3cx.calibrate`` Action
-------------------------------

At startup of the sensor, a calibration sequence is performed (as recommended by the manufacturer).

If needed, user can perform the calibration again after startup.

```yaml
# Example configuration entry
sensor:
  - platform: vl53l3cx
    name: "Friendly name"
    id: vl53l3cx_id1
    # ...

# in some trigger
on_...:
  - vl53l3cx_id1.calibrate
```

See Also
--------

General project structure and code structure was heavily inspired from the [vl53l0x component](https://github.com/esphome/esphome/tree/dev/esphome/components/vl53l0x).
 
