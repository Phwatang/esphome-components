# esphome-components
 Personal ESPHome external components

## Dev Notes
Everything below are notes from trying to develop an external component for ESPHome.

### Git Structure
ESPHome recommends a specific structure for the git repo for ESPHome component projects
 - https://esphome.io/components/external_components.html

### Dev Environment Setup
Install the ESPHome package with pip. This allows for ESPHome to be operated via command line.

General command to test out the component will be
```
python -m esphome run test_config.yaml
```

Within ``test_config.yaml`` we must tell ESPHome to look for the component on the PC
```yaml
# ...

external_components:
  - source:
      type: local
      path: /path/to/parent/dir/of/custom_component
```

Again, refer to the following for further details:
 - https://esphome.io/components/external_components.html

### Component Code Overview
ESPHome components consist of C++ code and python code.
 - The C++ files are responsible for acting as the driver code for your target device.
 - The Python code is responsible for defining:
    - The build environment of the C++ files
    - The schema of the yaml code associated with your device.

A simple general structure would be:
```
example_component_folder
 - __init__.py
 - driver_code.h
 - driver_code.cpp
```

If the component is a specific type of a pre-existing component then the python file ESPHome will look at will be different.
 - E.g if the component is a subtype of ``Sensor``, the ESPHome will look at ``sensor.py``

### Python
As mentioned before, the python file(s) have two key roles.

#### Build Environment
Build environment starts off with the Python import
```
import esphome.codegen as cg
```

Afterwards, namespaces in the C++ files can be encapsulated with Python code and necessary ESPHome header files can get included when the driver code gets compiled.

Easy to learn by example from looking at other ESPHome component.

#### Config Validation
Within the Python code, we can describe what the schema will look like of the yaml code that configures our final component.

This starts off with the Python import
```
import esphome.config_validation as cv
```

From describing the schema, the Python code also acts a validator to ensure the end user cannot compile with incorrect yaml code.

ESPHome achieves all of this using **voluptuous** as the backend. Voluptuous works off two main concepts:
 - **Schemas**
    - Describes formats of data
 - **Validators**
    - Callable functions that check if a piece of data fits a schema and raises an exception when it does
    - Also responsible for more advanced logic of deciding when a piece of data should be accepted

Many classes and functions from voluptuous are directly imported into config_validation.py. Thus its worth analysing the file directly to see what structures from voluputuous will be in the name space.
 - https://github.com/esphome/esphome/blob/dev/esphome/config_validation.py

### Actions
See the following for a general overview of actions:
 - https://esphome.io/automations/actions.html#triggers

Actions are defined in the C++ code with C++ templates.

The Python file is then responsible for ensuring action template is generated into appropriate C++ code during build time.

Look at other ESPHome components to see this clearly.

### See Also
Useful links:
 - https://github.com/esphome/esphome/blob/dev/esphome/components/climate/__init__.py
    - Registering custom actions
    - Defining custom states to be used in the configuration yaml
 - https://github.com/jesserockz/esphome-external-component-examples
