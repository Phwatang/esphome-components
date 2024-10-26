#include "vl53l3cx_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vl53l3cx {

static const char *const TAG = "vl53l3cx";

VL53L3CXSensor::VL53L3CXSensor():
// Initialise callbacks for the driver to interface with
vl53lxInstance(
  // gpio0_high callback
  [this]() {
    if (this->enable_pin != nullptr) {
      this->enable_pin->digital_write(true);
      delayMicroseconds(100);
    }
    return;
  },
  // gpio0_low callback
  [this]() {
    if (this->enable_pin != nullptr) {
      this->enable_pin->digital_write(false);
      delayMicroseconds(100);
    }
    return;
  },
  // I2CWrite callback
  [this](uint8_t DeviceAddr, uint16_t RegisterAddr, uint8_t *pBuffer, uint16_t NumByteToWrite) {
    this->write_register16(
      RegisterAddr, 
      pBuffer, 
      NumByteToWrite
    );
    return VL53LX_ERROR_NONE;
  },
  // I2CRead callback
  [this](uint8_t DeviceAddr, uint16_t RegisterAddr, uint8_t *pBuffer, uint16_t NumByteToRead) {
    this->read_register16(
      RegisterAddr, 
      pBuffer, 
      NumByteToRead
    );
    return VL53LX_ERROR_NONE;
  }
)
{}


void VL53L3CXSensor::dump_config() {
  LOG_SENSOR("", "VL53L3CX", this);
  LOG_UPDATE_INTERVAL(this);
  LOG_I2C_DEVICE(this);
  if (this->enable_pin != nullptr) {
    LOG_PIN("  Enable Pin: ", this->enable_pin)
  }
  switch (this->mode) {
    case ReportMode::Closest:
      ESP_LOGCONFIG(TAG, "  Mode: Closest");
      break;
    case ReportMode::Furtherest:
      ESP_LOGCONFIG(TAG, "  Mode: Furtherest");
      break;
  }
}

void VL53L3CXSensor::setup() {
  ESP_LOGD(TAG, "'%s' - Setup BEGIN", this->name_.c_str());
  VL53LX_Error error = VL53LX_ERROR_NONE;

  // Initial bootup
  this->vl53lxInstance.VL53LX_On();

  // Slave address reprogramming
  uint8_t final_address = this->address_;
  this->set_i2c_address(0x29); // vl53l3cx default i2c address is 0x29
  error = this->vl53lxInstance.VL53LX_SetDeviceAddress(final_address);
  this->set_i2c_address(final_address);
  if (error != VL53LX_ERROR_NONE) {
    ESP_LOGW(TAG, "'%s' - Slave address reprogramming failed!", this->name_.c_str());
  }

  // Finalise bootup
  error = this->vl53lxInstance.VL53LX_WaitDeviceBooted();
  if (error != VL53LX_ERROR_NONE) {
    ESP_LOGW(TAG, "'%s' - Bootup failed!", this->name_.c_str());
  }

  // Perform device initialisation
  error = this->vl53lxInstance.VL53LX_DataInit();
  if (error != VL53LX_ERROR_NONE) {
    ESP_LOGW(TAG, "'%s' - Initialisation failed!", this->name_.c_str());
  }

  this->perform_calibration();

  this->vl53lxInstance.VL53LX_SetMeasurementTimingBudgetMicroSeconds(100000);

  this->vl53lxInstance.VL53LX_ClearInterruptAndStartMeasurement();
  this->timeSinceLastInterruptClear = millis();
  ESP_LOGD(TAG, "'%s' - Setup END", this->name_.c_str());
}

void VL53L3CXSensor::update() {
  // Upon each update poll, initiate a new reading
  switch (this->state) {
    case MeasurementState::Idle:
      // Initiate a new reading
      this->vl53lxInstance.VL53LX_ClearInterruptAndStartMeasurement();
      this->timeSinceLastInterruptClear = millis();
      this->state = MeasurementState::InitiatedRead;
      break;
    default:
      this->publish_state(NAN);
      ESP_LOGW(TAG, "'%s' - New update called however a valid reading has not been found since last update call", this->name_.c_str());
      break;
  }
}

void VL53L3CXSensor::loop() {
  // Upon each loop poll, try to complete and report the initiated reading
  switch (this->state) {
    case MeasurementState::Idle:
      break;
    case MeasurementState::InitiatedRead: {
      // Check reading readiness
      VL53LX_Error error = VL53LX_ERROR_NONE;
      uint8_t NewDataReady = 0;
      error = this->vl53lxInstance.VL53LX_GetMeasurementDataReady(&NewDataReady);
      if (error != VL53LX_ERROR_NONE || NewDataReady == 0) {
        return;
      }
      this->state = MeasurementState::InterruptRaised;
      break;
    }
    case MeasurementState::InterruptRaised:
      // A reading is ready
      this->vl53lxInstance.VL53LX_GetMultiRangingData(&(this->multiRangingData));
      // Calculate appropriate reading
      int16_t dist = INT16_MAX;
      switch (this->mode) {
        case ReportMode::Closest:
          dist = this->getLowestDistance();
          break;
        case ReportMode::Furtherest:
          dist = this->getHighestDistance();
          break;
      }
      // If reading is erroneous 
      if (dist == INT16_MAX || dist == INT16_MIN) {
        // Initiate another reading
        this->vl53lxInstance.VL53LX_ClearInterruptAndStartMeasurement();
        this->timeSinceLastInterruptClear = millis();
        this->state = MeasurementState::InitiatedRead;
        break;
      }
      // Publish reading
      float reading = dist / 1000.0f;
      ESP_LOGD(TAG, "'%s' - Got distance %.3f m", this->name_.c_str(), reading);
      this->publish_state(reading);
      this->state = MeasurementState::Idle;
      break;
  }
}

VL53LX_MultiRangingData_t VL53L3CXSensor::getMeasurements() {
  return this->multiRangingData;
}

int16_t VL53L3CXSensor::getLowestDistance() {
  // Note: It's assumed that if n objects are found, then then the data will be
  //       in the first n indexes of RangeData. Not sure if this is always true...
  auto minDist = INT16_MAX;
  for(int i = 0; i < this->multiRangingData.NumberOfObjectsFound; i++) {
    if(this->multiRangingData.RangeData[i].RangeStatus != 0) {
      continue;
    }
    auto dist = this->multiRangingData.RangeData[i].RangeMilliMeter;
    if(dist < minDist) {
      minDist = dist;
    }
  }
  return minDist;
}

int16_t VL53L3CXSensor::getHighestDistance() {
  // Note: It's assumed that if n objects are found, then then the data will be
  //       in the first n indexes of RangeData. Not sure if this is always true...
  auto maxDist = INT16_MIN;
  for(int i = 0; i < this->multiRangingData.NumberOfObjectsFound; i++) {
    if(this->multiRangingData.RangeData[i].RangeStatus != 0) {
      continue;
    }
    auto dist = this->multiRangingData.RangeData[i].RangeMilliMeter;
    if(dist > maxDist) {
      maxDist = dist;
    }
  }
  return maxDist;
}

uint8_t VL53L3CXSensor::getNumberOfObjects() {
  return this->multiRangingData.NumberOfObjectsFound;
}

uint32_t VL53L3CXSensor::getTimingBudget() {
  uint32_t time = 0;
  this->vl53lxInstance.VL53LX_GetMeasurementTimingBudgetMicroSeconds(&time);
  return time;
}

void VL53L3CXSensor::set_report_mode(ReportMode mode) {
  this->mode = mode;
}

void VL53L3CXSensor::perform_calibration() {
  // Perform calibration steps specified in user manual:
  // https://www.st.com/resource/en/user_manual/um2778-vl53l3cx-timeofflight-ranging-module-with-multi-object-detection-stmicroelectronics.pdf
  ESP_LOGD(TAG, "'%s' - Calibration BEGIN", this->name_.c_str());
  this->vl53lxInstance.VL53LX_SetTuningParameter(VL53LX_TUNINGPARM_PHASECAL_PATCH_POWER, 2);
  this->vl53lxInstance.VL53LX_PerformRefSpadManagement();
  this->vl53lxInstance.VL53LX_PerformXTalkCalibration();
  ESP_LOGD(TAG, "'%s' - Calibration END", this->name_.c_str());
}

VL53L3CXSensor::~VL53L3CXSensor() {}


}  // namespace vl53l3cx
}  // namespace esphome
