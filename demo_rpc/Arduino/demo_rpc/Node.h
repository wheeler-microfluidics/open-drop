#ifndef ___NODE__H___
#define ___NODE__H___

#include <BaseNode.h>
#include <Array.h>
#include "pb_validate.h"
#include "demo_rpc_config_validate.h"
namespace demo_rpc {
#include "demo_rpc_config_pb.h"
}  // namespace demo_rpc


namespace i2c_buffer {


template <typename Parser>
struct I2cReceiver {
  Parser &parser_;
  uint8_t source_addr_;

  I2cReceiver(Parser &parser) : parser_(parser) { reset(); }

  void operator()(int16_t byte_count) {
    // Interpret first byte of each I2C message as source address of message.
    uint8_t source_addr = Wire.read();
    byte_count -= 1;
    /* Received message from a new source address.
     *
     * TODO
     * ====
     *
     * Strategies for dealing with this situation:
     *  1. Discard messages that do not match current source address until
     *     receiver is reset.
     *  2. Reset parser and start parsing data from new source.
     *  3. Maintain separate parser for each source address? */
    if (source_addr_ == 0) { source_addr_ = source_addr; }

    for (int i = 0; i < byte_count; i++) {
      uint8_t value = Wire.read();
      if (source_addr == source_addr_) { parser_.parse_byte(&value); }
    }
  }

  void reset() {
    parser_.reset();
    source_addr_ = 0;
  }

  bool packet_ready() { return parser_.message_completed_; }
  uint8_t packet_error() {
    if (parser_.parse_error_) { return 'e'; }
    return 0;
  }

  UInt8Array packet_read() {
    UInt8Array output;
    output.data = NULL;
    output.length = 0;
    if (packet_ready()) {
      output.data = parser_.packet_->payload_buffer_;
      output.length = parser_.packet_->payload_length_;
    }
    return output;
  }
};

}  // namespace i2c_handler


class Node : public BaseNode {
public:
  using BaseNode::output_buffer;
  using BaseNode::RETURN_CODE_;
  demo_rpc::Config config_;
  demo_rpc::State state_;

  MessageValidator<2> config_validator_;
  SerialNumberValidator serial_number_validator_;
  I2cAddressValidator i2c_address_validator_;

  MessageValidator<2> state_validator_;
  FloatValueValidator float_value_validator_;
  IntegerValueValidator integer_value_validator_;
  typedef PacketParser<FixedPacket> parser_t;
  parser_t i2c_parser_;
  FixedPacket i2c_packet_;
  i2c_buffer::I2cReceiver<parser_t> i2c_receiver_;

  Node() : BaseNode(), i2c_receiver_(i2c_parser_) {
    config_validator_.register_validator(serial_number_validator_);
    config_validator_.register_validator(i2c_address_validator_);
    state_validator_.register_validator(float_value_validator_);
    state_validator_.register_validator(integer_value_validator_);
    state_validator_.register_validator(integer_value_validator_);
    i2c_packet_.reset_buffer(I2C_PACKET_SIZE, &i2c_packet_buffer[0]);
    i2c_parser_.reset(&i2c_packet_);
    /* Configuration must be loaded after `i2c_address_validator_` has been
     * registered, since i2c address validator sets the address of the `Wire`
     * interface. */
    load_config();
    Wire.onReceive(i2c_receive_event);
  }
  void reset_config() { config_ = Config_init_default; }
  uint8_t update_config(UInt8Array serialized_config) {
    demo_rpc::Config config = Config_init_default;
    bool ok = base_node_rpc::decode_from_array(serialized_config,
                                               demo_rpc::Config_fields,
                                               config);
    if (ok) {
      config_validator_.update(demo_rpc::Config_fields, config, config_);
    }
    return ok;
  }
  void save_config() {
    UInt8Array serialized = serialize_config();
    if (serialized.data != NULL) {
      eeprom_update_block((void*)&serialized.length, (void*)0,
                          sizeof(uint16_t));
      update_eeprom_block(sizeof(uint16_t), serialized);
    }
  }
  void reset_state() { state_ = State_init_default; }
  uint8_t update_state(UInt8Array serialized_state) {
    demo_rpc::State state;
    bool ok = base_node_rpc::decode_from_array(serialized_state,
                                               demo_rpc::State_fields,
                                               state);
    if (ok) {
      state_validator_.update(demo_rpc::State_fields, state, state_);
    }
    return ok;
  }
  UInt8Array serialize_state() {
    return BaseNode::serialize_obj(state_, demo_rpc::State_fields);
  }
  UInt8Array serialize_config() {
    return BaseNode::serialize_obj(config_, demo_rpc::Config_fields);
  }
  void validate_config() {
    demo_rpc::Config &config = *((demo_rpc::Config *)output_buffer);
    config = Config_init_default;
    /* Validate the active configuration structure (i.e., trigger the
     * validation callbacks). */
    config_validator_.update(demo_rpc::Config_fields, config_, config);
  }
  void load_config() {
    if (!decode_obj_from_eeprom(0, config_, demo_rpc::Config_fields)) {
      /* Configuration could not be loaded from EEPROM; reset config. */
      reset_config();
      RETURN_CODE_ = 1;
    } else {
      RETURN_CODE_ = 0;
    }
    validate_config();
  }
  uint8_t return_code() { return RETURN_CODE_; }
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
  uint32_t i2c_address() { return config_.i2c_address; }
  uint32_t sizeof_parser() { return sizeof(PacketParser<FixedPacket>); }

  void i2c_parser_reset() { i2c_receiver_.reset(); }
  uint8_t i2c_packet_ready() { return i2c_receiver_.packet_ready(); }
  uint8_t i2c_packet_error() { return i2c_receiver_.packet_error(); }
  UInt8Array i2c_packet_read() {
    UInt8Array output = i2c_receiver_.packet_read();
    output.length = (output.length > max_payload_size() ? max_payload_size()
                     : output.length);
    return output;
  }
  uint8_t i2c_receiver_source_address() { return i2c_receiver_.source_addr_; }
  void set_i2c_receiver_source_address(uint8_t addr) {
    i2c_receiver_.source_addr_ = addr;
  }
};

#endif  // #ifndef ___NODE__H___
