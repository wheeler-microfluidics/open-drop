#include "Node.h"

namespace demo_rpc {

void Node::begin() {
  config_.set_buffer(get_buffer());
  state_.set_buffer(get_buffer());
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


uint16_t Node::packet_crc(UInt8Array data) {
  FixedPacket to_send;
  to_send.type(Packet::packet_type::DATA);
  to_send.reset_buffer(data.length, data.data);
  to_send.payload_length_ = data.length;

  /* Set the CRC checksum of the packet based on the contents of the payload.
  * */
  to_send.compute_crc();
  return to_send.crc_;
}


UInt8Array Node::test_parser(UInt8Array data) {
  UInt8Array output = get_buffer();
  parser_t parser;
  FixedPacket packet;
  packet.reset_buffer(output.length, &output.data[0]);
  parser.reset(&packet);
  for (int i = 0; i < data.length; i++) {
    parser.parse_byte(&data.data[i]);
  }
  output.data[0] = parser.message_completed_;
  output.data[1] = parser.parse_error_;
  output.length = 2;
  return output;
}

}  // namespace demo_rpc
