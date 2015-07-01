#ifndef ___NODE__H___
#define ___NODE__H___

#include <BaseNodeRpc.h>
#include <BaseNodeConfig.h>
#include <BaseNodeEeprom.h>
#include <BaseNodeI2c.h>
#include <BaseNodePb.h>
//#include <BaseNodeState.h>
#include <Array.h>
#include <I2cHandler.h>
#include <SerialHandler.h>
#include <pb_validate.h>
#include "demo_rpc_config_validate.h"
namespace demo_rpc {
#include "demo_rpc_config_pb.h"


template <typename Fields>
inline Config get_pb_default(Fields const &fields) {
  /* Use nanopb decode with `init_default` set to `true` as a workaround to
   * avoid needing to initialize the object (i.e., `_`) using the
   * `<message>_init_default` macro.
   *
   * This is necessary because the macro resolves to an initialization list,
   * which cannot be referenced in a memory efficient, generalizable way. */
  Config obj;
  UInt8Array null_array;
  null_array.length = 0;
  base_node_rpc::decode_from_array(null_array, fields, obj, true);
  return obj;
}


//class NodeConfig : public BaseNodePb {
class NodeConfig {
public:
  Config _;
  MessageValidator<2> validator_;
  SerialNumberValidator serial_number_validator_;
  I2cAddressValidator i2c_address_validator_;
  UInt8Array buffer_;
  const pb_field_t *fields_;

  NodeConfig(const pb_field_t *fields, size_t buffer_size, uint8_t *buffer)
    : fields_(fields) {
    buffer_.length = buffer_size;
    buffer_.data = buffer;
    validator_.register_validator(serial_number_validator_);
    validator_.register_validator(i2c_address_validator_);
  }

  NodeConfig(const pb_field_t *fields, UInt8Array buffer)
    : fields_(fields), buffer_(buffer) {
    validator_.register_validator(serial_number_validator_);
    validator_.register_validator(i2c_address_validator_);
  }
  void reset() {
    _ = get_pb_default(fields_);
  }
  uint8_t update(UInt8Array serialized) {
    Config &obj = *((Config *)buffer_.data);
    bool ok = base_node_rpc::decode_from_array(serialized,
                                               fields_,
                                               obj, true);  // init_default
    if (ok) {
      validator_.update(fields_, obj, _);
    }
    return ok;
  }
  UInt8Array serialize() {
    return base_node_rpc::serialize_to_array(_, fields_, buffer_);
  }
  void validate() {
    Config &obj = *((Config *)buffer_.data);
    obj = get_pb_default(fields_);
    /* Validate the active configuration structure (i.e., trigger the
     * validation callbacks). */
    validator_.update(fields_, _, obj);
  }
  void load(uint8_t address=0) {
    if (!base_node_rpc::decode_obj_from_eeprom(address, _, fields_,
                                               buffer_)) {
      /* Configuration could not be loaded from EEPROM; reset obj. */
      reset();
    }
    validate();
  }
  void save() {
    UInt8Array serialized = serialize();
    if (serialized.data != NULL) {
      base_node_rpc::array_to_eeprom(0, serialized);
    }
  }
};


#if 0
class NodeState : public BaseNodePb {
public:
  State state_;
  MessageValidator<2> state_validator_;
  FloatValueValidator float_value_validator_;
  IntegerValueValidator integer_value_validator_;

  void init_state() {
    state_validator_.register_validator(float_value_validator_);
    state_validator_.register_validator(integer_value_validator_);
  }
  void reset_state() { state_ = State_init_default; }
  uint8_t update_state(UInt8Array serialized_state) {
    State state = State_init_default;
    bool ok = base_node_rpc::decode_from_array(serialized_state,
                                               State_fields,
                                               state);
    if (ok) {
      state_validator_.update(State_fields, state, state_);
    }
    return ok;
  }
  UInt8Array serialize_state() {
    return serialize_obj(state_, State_fields);
  }
  void validate_state(UInt8Array buffer) {
    State &state = *((State *)buffer.data);
    state = State_init_default;
    /* Validate the active stateuration structure (i.e., trigger the
     * validation callbacks). */
    state_validator_.update(State_fields, state_, state);
  }
};
#endif


class Node :
  public BaseNode, public BaseNodeEeprom, public BaseNodeI2c {
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

  i2c_handler_t i2c_handler_;
#ifndef DISABLE_SERIAL
  serial_handler_t serial_handler_;
#endif  // #ifndef DISABLE_SERIAL

  NodeConfig config_;
  //NodeState state_;

  Node() : BaseNode(), config_(Config_fields, sizeof(output_buffer),
                               &output_buffer[0]) {
    /* Configuration must be loaded after `i2c_address_validator_` has been
     * registered, since i2c address validator sets the address of the `Wire`
     * interface. */
    config_.load();
    //config_.init_state();
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
  void load_config() { config_.load(); }
  void reset_config() { config_.reset(); }
  uint8_t update_config(UInt8Array serialized) {
    return config_.update(serialized);
  }
  UInt8Array serialize_config() { return config_.serialize(); }

  virtual UInt8Array get_buffer() {
    UInt8Array output;
    output.data = output_buffer;
    output.length = sizeof(output_buffer);
    return output;
  }
  void set_serial_number(uint32_t value) { config_._.serial_number = value; }
  uint32_t serial_number() { return config_._.serial_number; }
  void set_i2c_address(uint8_t value) {
    // Validator expects `uint32_t` by reference.
    uint32_t address = value;
    /* Validate address and update the active `Wire` configuration if the
     * address is valid. */
    config_.i2c_address_validator_(address, config_._.i2c_address);
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

  UInt8Array i2c_request(uint8_t address, UInt8Array data) {
    UInt8Array result = i2c_handler_.request(address, data);
    if (result.data == NULL) { result.data = get_buffer().data; }
    return result;
  }

  uint32_t sizeof_node_config() { return sizeof(NodeConfig); }
  //uint32_t sizeof_node_state() { return sizeof(NodeState); }
  uint32_t sizeof_config() { return sizeof(Config); }
  //uint32_t sizeof_state() { return sizeof(State); }
  uint32_t sizeof_parser() { return sizeof(parser_t); }
  uint32_t sizeof_packet_struct() { return sizeof(FixedPacket); }
  uint32_t sizeof_packet() { return PACKET_SIZE; }
  uint32_t sizeof_i2c_handler() { return sizeof(i2c_handler_t); }
#ifndef DISABLE_SERIAL
  uint32_t sizeof_serial_handler() { return sizeof(serial_handler_t); }
#endif  // #ifndef DISABLE_SERIAL
};

}  // namespace demo_rpc


#endif  // #ifndef ___NODE__H___
