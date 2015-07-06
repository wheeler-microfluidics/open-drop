#ifndef ___DEMO_RPC_CONFIG_VALIDATE___
#define ___DEMO_RPC_CONFIG_VALIDATE___

#ifdef __AVR__
#include <Wire.h>
#endif  // ifdef __AVR__
#include "pb_validate.h"

namespace dmf_control_rpc {

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


template <typename NodeT>
struct VoltageValidator : public ScalarFieldValidator<float, 1> {
  typedef ScalarFieldValidator<float, 1> base_type;

  NodeT *node_p_;
  VoltageValidator() : node_p_(NULL) { this->tags_[0] = 1; }

  void set_node(NodeT &node) { node_p_ = &node; }
  virtual bool operator()(float &source, float target) {
    if (node_p_ != NULL) { return node_p_->on_voltage_changed(target, source); }
    return false;
  }
};


template <typename NodeT>
struct FrequencyValidator : public ScalarFieldValidator<float, 1> {
  typedef ScalarFieldValidator<float, 1> base_type;

  NodeT *node_p_;
  FrequencyValidator() : node_p_(NULL) { this->tags_[0] = 2; }

  void set_node(NodeT &node) { node_p_ = &node; }

  virtual bool operator()(float &source, float target) {
    if (node_p_ != NULL) { return node_p_->on_frequency_changed(target, source); }
    return false;
  }
};


template <typename NodeT>
class NodeStateValidator : public MessageValidator<2> {
public:
  VoltageValidator<NodeT> voltage_;
  FrequencyValidator<NodeT> frequency_;

  NodeStateValidator() {
    register_validator(voltage_);
    register_validator(frequency_);
  }
};

}  // namespace dmf_control_rpc

#endif  // #ifndef ___DEMO_RPC_CONFIG_VALIDATE___
