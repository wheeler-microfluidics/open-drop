#ifndef ___NODE__H___
#define ___NODE__H___

#include <BaseNodeRpc.h>
#include <BaseNodeEeprom.h>
#include <BaseNodeI2c.h>
#include <BaseNodePb.h>
#include <Array.h>
#include "pb_validate.h"
#include "demo_rpc_config_validate.h"
namespace demo_rpc {
#include "demo_rpc_config_pb.h"
}  // namespace demo_rpc


class Node :
  public BaseNode, public BaseNodeEeprom, public BaseNodeI2c,
  public BaseNodePb {
public:
  uint8_t output_buffer[128];
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
  }
  void begin() {
    // Set i2c clock-rate to 400kHz.
    TWBR = 12;
#if !defined(DISABLE_SERIAL)
    Serial.begin(115200);
#endif  // #ifndef DISABLE_SERIAL
    Wire.onReceive(i2c_receive_event);
  }
  virtual UInt8Array get_buffer() {
    UInt8Array output;
    output.data = output_buffer;
    output.length = sizeof(output_buffer);
    return output;
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
    return serialize_obj(state_, demo_rpc::State_fields);
  }
  UInt8Array serialize_config() {
    return serialize_obj(config_, demo_rpc::Config_fields);
  }
  void validate_config() {
    UInt8Array output = get_buffer();
    demo_rpc::Config &config = *((demo_rpc::Config *)output.data);
    config = Config_init_default;
    /* Validate the active configuration structure (i.e., trigger the
     * validation callbacks). */
    config_validator_.update(demo_rpc::Config_fields, config_, config);
  }
  void load_config() {
    if (!decode_obj_from_eeprom(0, config_, demo_rpc::Config_fields)) {
      /* Configuration could not be loaded from EEPROM; reset config. */
      reset_config();
    }
    validate_config();
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

  void i2c_packet_reset() { i2c_receiver_.reset(); }
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

  UInt8Array i2c_process_packet() {
    UInt8Array result = get_buffer();
    if (i2c_packet_ready()) {
      demo_rpc::CommandProcessor<Node> command_processor(*this);

      result = process_packet_with_processor(*i2c_receiver_.parser_.packet_,
                                             command_processor);
      if (result.data == NULL) { result.length = 0; }
      i2c_write_packet(i2c_receiver_.source_addr_, result);
      // Reset packet state since request may have overwritten packet buffer.
      i2c_packet_reset();
    } else {
      result.data = NULL;
      result.length = 0;
    }
    return result;
  }

  UInt8Array i2c_request(uint8_t address, UInt8Array data) {
    i2c_packet_reset();
    i2c_write_packet(address, data);
    uint32_t start_time = millis();
    while (5000UL > (millis() - start_time) && !i2c_packet_ready()) {}
    UInt8Array result = i2c_packet_read();
    // Reset packet state to prepare for incoming requests on I2C interface.
    i2c_packet_reset();
    return result;
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

  void i2c_write_packet(uint8_t address, UInt8Array data) {
    FixedPacket to_send;
    to_send.type(Packet::packet_type::DATA);
    to_send.reset_buffer(data.length, data.data);
    to_send.payload_length_ = data.length;

    /* Set the CRC checksum of the packet based on the contents of the payload.
    * */
    to_send.compute_crc();

    stream_byte_type startflag[] = "|||";
    const uint8_t source_addr = config_.i2c_address;
    uint8_t type_ = static_cast<uint8_t>(to_send.type());

    Wire.beginTransmission(address);
    serialize_any(Wire, source_addr);
    Wire.write(startflag, 3);
    serialize_any(Wire, to_send.iuid_);
    serialize_any(Wire, type_);
    serialize_any(Wire, static_cast<uint16_t>(to_send.payload_length_));
    Wire.endTransmission();

    while (to_send.payload_length_ > 0) {
      uint16_t length = ((to_send.payload_length_ > TWI_BUFFER_LENGTH - 1)
                         ? TWI_BUFFER_LENGTH - 1 : to_send.payload_length_);

      Wire.beginTransmission(address);
      serialize_any(Wire, source_addr);
      Wire.write((stream_byte_type*)to_send.payload_buffer_,
                  length);
      Wire.endTransmission();

      to_send.payload_buffer_ += length;
      to_send.payload_length_ -= length;
    }

    Wire.beginTransmission(address);
    serialize_any(Wire, source_addr);
    serialize_any(Wire, to_send.crc_);
    Wire.endTransmission();
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
};

#endif  // #ifndef ___NODE__H___
