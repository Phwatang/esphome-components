#pragma once

#include "vl53lx_class.h"

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace vl53l3cx {

enum class MeasurementState {
  Idle,
  InitiatedRead,
  InterruptRaised,
};

enum class ReportMode {
  Furtherest,
  Closest,
};

// Wrapper to interface the vl53lx driver code to ESPHome
class VL53L3CXSensor : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice {
  protected:
    unsigned long timeSinceLastInterruptClear = millis();

    VL53LX vl53lxInstance;
    VL53LX_MultiRangingData_t multiRangingData;
    MeasurementState state{MeasurementState::Idle};
    ReportMode mode{ReportMode::Closest};
    GPIOPin *enable_pin{nullptr};

  public:
    VL53L3CXSensor();

    void set_enable_pin(GPIOPin *enable) { 
      this->enable_pin = enable; 
      this->enable_pin->setup();
      // Make sure device is off before setup (set XSHUT pin to low).
      // Ensures that i2c reprogramming occurs correctly at setup when working with
      // multiple vl53l3cx sensors.
      this->vl53lxInstance.VL53LX_Off();
    }
    void set_report_mode(ReportMode mode);

    // Returns the last updated set of ranging data from sensor
    VL53LX_MultiRangingData_t getMeasurements();
    // Outputs the lowest valid distance reading of all objects in the ranging data
    int16_t getLowestDistance();
    // Outputs the highest valid distance reading of all objects in the ranging data
    int16_t getHighestDistance();
    // Outputs number of "objects" found by the sensor
    uint8_t getNumberOfObjects();
    uint32_t getTimingBudget();
    // Perform calibration sequence on the sensor
    void perform_calibration();

    void setup() override;
    void loop() override;
    void update() override;

    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::DATA; }

    ~VL53L3CXSensor();
};

template<typename... Ts> class VL53L3CXCalibrateAction : public Action<Ts...> {
  public:
    explicit VL53L3CXCalibrateAction(VL53L3CXSensor* sensor) : sensor_(sensor) {}

    void play(Ts... x) override { this->sensor_->perform_calibration(); }
  
  protected:
    VL53L3CXSensor *sensor_;
};

} // namespace vl53l3cx
} // namespace esphome