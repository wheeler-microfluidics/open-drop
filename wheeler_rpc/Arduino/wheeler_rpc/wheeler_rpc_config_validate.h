#ifndef ___DEMO_RPC_CONFIG_VALIDATE___
#define ___DEMO_RPC_CONFIG_VALIDATE___

#ifdef __AVR__
#include <Wire.h>
#endif  // ifdef __AVR__
#include "pb_validate.h"


struct SerialNumberValidator : public ScalarFieldValidator<uint32_t, 1> {
    using ScalarFieldValidator<uint32_t, 1>::tags_;

    SerialNumberValidator() { this->tags_[0] = 1; }

    virtual bool operator()(uint32_t &source, uint32_t target) {
        LOG("  current serial number=%d, proposed serial number=%d\n", a, b);
        // Serial number must be greater than zero.
        return (source > 0);
    }
};


struct BaudRateValidator : public ScalarFieldValidator<uint32_t, 1> {
    using ScalarFieldValidator<uint32_t, 1>::tags_;

    BaudRateValidator() { this->tags_[0] = 2; }

    virtual bool operator()(uint32_t &source, uint32_t target) {
      LOG("  current baud rate=%d, proposed baud rate=%d\n", a, b);
      // Only certain baud rates are valid.
      switch (source) {
        case 300:
        case 600:
        case 1200:
        case 2400:
        case 4800:
        case 9600:
        case 14400:
        case 19200:
        case 28800:
        case 31250:
        case 38400:
        case 57600:
        case 115200:
          return true;
        default:
          return false;
      };
    }
};


struct I2cAddressValidator : public ScalarFieldValidator<uint32_t, 1> {
  using ScalarFieldValidator<uint32_t, 1>::tags_;

  I2cAddressValidator() { this->tags_[0] = 3; }

  virtual bool operator()(uint32_t &source, uint32_t target) {
    LOG("  current i2c address=%d, proposed i2c address=%d\n", a, b);
    // I2C addresses must be in the range 10-120, according to the
    // specification.
    if ((source > 0x07) && (source < 0x78)) {
#ifdef __AVR__
      Wire.begin(static_cast<uint8_t>(source));
#endif  // #ifdef __AVR__
      return true;
    } else {
      // Invalid i2c address was specified, so set address to original value.
      source = target;
      return false;
    }
  }
};


struct FloatValueValidator : public ScalarFieldValidator<float, 1> {
    typedef float value_t;
    using ScalarFieldValidator<value_t, 1>::tags_;

    FloatValueValidator() { this->tags_[0] = 1; }

    virtual bool operator()(value_t &source, value_t target) {
        LOG("  current float value=%.2f, proposed float value=%.2f\n", a, b);
        return (source > 3.14);
    }
};


struct IntegerValueValidator : public ScalarFieldValidator<int32_t, 1> {
    typedef int32_t value_t;
    using ScalarFieldValidator<value_t, 1>::tags_;

    IntegerValueValidator() { this->tags_[0] = 2; }

    virtual bool operator()(value_t &source, value_t target) {
        LOG("  current integer value=%d, proposed integer value=%d\n", a, b);
        return (source > 5) && (source < 1024);
    }
};


class NodeConfigValidator : public MessageValidator<3> {
public:
  SerialNumberValidator serial_number_;
  BaudRateValidator baud_rate_;
  I2cAddressValidator i2c_address_;

  NodeConfigValidator() {
    register_validator(serial_number_);
    register_validator(baud_rate_);
    register_validator(i2c_address_);
  }
};


class NodeStateValidator : public MessageValidator<2> {
public:
  FloatValueValidator float_value_;
  IntegerValueValidator integer_value_;

  NodeStateValidator() {
    register_validator(float_value_);
    register_validator(integer_value_);
  }
};

#endif  // #ifndef ___DEMO_RPC_CONFIG_VALIDATE___
