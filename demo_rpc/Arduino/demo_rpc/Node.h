#ifndef ___NODE__H___
#define ___NODE__H___

#include <BaseNodeRpc.h>
#include <BaseNodeConfig.h>
#include <BaseNodeEeprom.h>
#include <BaseNodeI2c.h>
#include <BaseNodePb.h>
#include <Array.h>
#include <I2cHandler.h>
#include <SerialHandler.h>
#include <pb_validate.h>
#include "demo_rpc_config_validate.h"
namespace demo_rpc {
#include "demo_rpc_config_pb.h"


class NodeConfig : public BaseNodeConfig<Config, MessageValidator<2> > {
public:
  typedef BaseNodeConfig<Config, MessageValidator<2> > base_type;

  NodeConfig() { reset_config(); }

  const pb_field_t *get_config_fields() { return Config_fields; }
  Config get_config_default() {
    Config result = Config_init_default;
    return result;
  }
};


class Node :
  public BaseNode, public BaseNodeEeprom, public BaseNodeI2c,
  public NodeConfig {
public:
  typedef PacketParser<FixedPacket> parser_t;
  typedef base_node_rpc::I2cReceiver<parser_t> i2c_receiver_t;
  typedef base_node_rpc::I2cHandler<i2c_receiver_t, I2C_PACKET_SIZE>
    i2c_handler_t;
#ifndef DISABLE_SERIAL
  typedef base_node_rpc::SerialReceiver<parser_t> serial_receiver_t;
  typedef base_node_rpc::Handler<serial_receiver_t, PACKET_SIZE>
    serial_handler_t;
#endif  // #ifndef DISABLE_SERIAL

  uint8_t output_buffer[128];
  State state_;

  SerialNumberValidator serial_number_validator_;
  I2cAddressValidator i2c_address_validator_;

  MessageValidator<2> state_validator_;
  FloatValueValidator float_value_validator_;
  IntegerValueValidator integer_value_validator_;

  i2c_handler_t i2c_handler_;
#ifndef DISABLE_SERIAL
  serial_handler_t serial_handler_;
#endif  // #ifndef DISABLE_SERIAL

  Node() : BaseNode() {
    config_validator_.register_validator(serial_number_validator_);
    config_validator_.register_validator(i2c_address_validator_);
    state_validator_.register_validator(float_value_validator_);
    state_validator_.register_validator(integer_value_validator_);
    state_validator_.register_validator(integer_value_validator_);
    /* Configuration must be loaded after `i2c_address_validator_` has been
     * registered, since i2c address validator sets the address of the `Wire`
     * interface. */
    load_config();
  }
  uint32_t max_payload_size() {
    return (PACKET_SIZE
            - 3 * sizeof(uint8_t)  // Frame boundary
            - sizeof(uint16_t)  // UUID
            - sizeof(uint16_t));  // Payload length
  }
  void begin() {
#if !defined(DISABLE_SERIAL)
    Serial.begin(115200);
#endif  // #ifndef DISABLE_SERIAL
    // Set i2c clock-rate to 400kHz.
    TWBR = 12;
  }
  virtual UInt8Array get_buffer() {
    UInt8Array output;
    output.data = output_buffer;
    output.length = sizeof(output_buffer);
    return output;
  }
  void reset_state() { state_ = State_init_default; }
  uint8_t update_state(UInt8Array serialized_state) {
    State state;
    bool ok = base_node_rpc::decode_from_array(serialized_state, State_fields,
                                               state);
    if (ok) {
      state_validator_.update(State_fields, state, state_);
    }
    return ok;
  }
  UInt8Array serialize_state() {
    return serialize_obj(state_, State_fields);
  }
  void set_serial_number(uint32_t value) { config_.serial_number = value; }
  uint32_t serial_number() { return config_.serial_number; }
  void set_i2c_address(uint8_t value) {
    // Validator expects `uint32_t` by reference.
    uint32_t address = value;
    /* Validate address and update the active `Wire` configuration if the
     * address is valid. */
    i2c_address_validator_(address, config_.i2c_address);
    config_.i2c_address = address;
  }
  uint32_t sizeof_parser() { return sizeof(PacketParser<FixedPacket>); }

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

  UInt8Array i2c_request(uint8_t address, UInt8Array data) {
    UInt8Array result = i2c_handler_.request(address, data);
    if (result.data == NULL) { result.data = get_buffer().data; }
    return result;
  }

  uint32_t sizeof_i2c_handler() { return sizeof(i2c_handler_t); }
};

}  // namespace demo_rpc


#endif  // #ifndef ___NODE__H___
