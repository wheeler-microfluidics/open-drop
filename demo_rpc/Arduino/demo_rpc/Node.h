#ifndef ___NODE__H___
#define ___NODE__H___

#include <BaseNodeRpc.h>
#include <BaseNodeFixedBuffer.h>
#include <BaseNodeSerialHandler.h>
#include <BaseNodeI2cHandler.h>
#include <Array.h>
#include <I2cHandler.h>
#include <SerialHandler.h>
#include <pb_validate.h>
#include <pb_eeprom.h>
#include "demo_rpc_config_validate.h"
namespace demo_rpc {
#include "demo_rpc_config_pb.h"


const size_t FRAME_SIZE = (3 * sizeof(uint8_t)  // Frame boundary
                           - sizeof(uint16_t)  // UUID
                           - sizeof(uint16_t));  // Payload length


class Node :
  public BaseNode,
#ifndef DISABLE_SERIAL
  public BaseNodeSerialHandler,
#endif  // #ifndef DISABLE_SERIAL
  public BaseNodeI2cHandler, public BaseNodeFixedBuffer<128> {
public:
  typedef PacketParser<FixedPacket> parser_t;

  nanopb::EepromMessage<Config, NodeConfigValidator> config_;
  nanopb::Message<State, NodeStateValidator> state_;

  Node() : BaseNode(), config_(Config_fields), state_(State_fields) {}
  UInt8Array get_buffer() { return buffer(); }

  void begin() {
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

  uint32_t max_payload_size() { return PACKET_SIZE - FRAME_SIZE; }

  void load_config() { config_.load(0); }
  void save_config() { config_.save(0); }
  void reset_config() { config_.reset(); }
  UInt8Array serialize_config() { return config_.serialize(); }
  uint8_t update_config(UInt8Array serialized) {
    return config_.update(serialized);
  }

  void reset_state() { state_.reset(); }
  UInt8Array serialize_state() { return state_.serialize(); }
  uint8_t update_state(UInt8Array serialized) {
    return state_.update(serialized);
  }

  void set_serial_number(uint32_t value) { config_._.serial_number = value; }
  uint32_t serial_number() { return config_._.serial_number; }
  void set_i2c_address(uint8_t value) {
    // Validator expects `uint32_t` by reference.
    uint32_t address = value;
    /* Validate address and update the active `Wire` configuration if the
     * address is valid. */
    config_.validator_.i2c_address_(address, config_._.i2c_address);
    config_._.i2c_address = address;
  }
  uint16_t packet_crc(UInt8Array data) {
    FixedPacket to_send;
    to_send.type(Packet::packet_type::DATA);
    to_send.reset_buffer(data.length, data.data);
    to_send.payload_length_ = data.length;

    /* Set the CRC checksum of the packet based on the contents of the payload.
    * */
    to_send.compute_crc();
    return to_send.crc_;
  }

  UInt8Array test_parser(UInt8Array data) {
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

  uint32_t sizeof_node() { return sizeof(Node); }
  uint32_t sizeof_config() { return sizeof(Config); }
  uint32_t sizeof_state() { return sizeof(State); }
  uint32_t sizeof_parser() { return sizeof(parser_t); }
  uint32_t sizeof_packet_struct() { return sizeof(FixedPacket); }
  uint32_t sizeof_packet() { return PACKET_SIZE; }
};

}  // namespace demo_rpc


#endif  // #ifndef ___NODE__H___
