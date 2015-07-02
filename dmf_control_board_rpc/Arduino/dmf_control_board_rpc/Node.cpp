#include "Node.h"

namespace dmf_control_board_rpc {

void Node::begin() {
  config_.set_buffer(get_buffer());
  config_.load();
  // Start Serial after loading config to set baud rate.
#if !defined(DISABLE_SERIAL)
  Serial.begin(config_._.baud_rate);
#endif  // #ifndef DISABLE_SERIAL
  // Set i2c clock-rate to 400kHz.
  TWBR = 12;
}


void Node::set_i2c_address(uint8_t value) {
  // Validator expects `uint32_t` by reference.
  uint32_t address = value;
  /* Validate address and update the active `Wire` configuration if the
    * address is valid. */
  config_.validator_.i2c_address_(address, config_._.i2c_address);
  config_._.i2c_address = address;
}

}  // namespace dmf_control_board_rpc
