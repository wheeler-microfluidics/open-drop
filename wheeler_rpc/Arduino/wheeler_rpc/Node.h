#ifndef ___NODE__H___
#define ___NODE__H___

#include <Arduino.h>
#include <NadaMQ.h>
#include <BaseNodeRpc.h>
#include <BaseNodeEeprom.h>
#include <BaseNodeI2c.h>
#include <BaseNodeConfig.h>
#include <BaseNodeState.h>
#include <BaseNodeSerialHandler.h>
#include <BaseNodeI2cHandler.h>
#include <Array.h>
#include <I2cHandler.h>
#include <SerialHandler.h>
#include <pb_validate.h>
#include <pb_eeprom.h>
#include "wheeler_rpc_config_validate.h"
namespace wheeler_rpc {
#include "wheeler_rpc_config_pb.h"


const size_t FRAME_SIZE = (3 * sizeof(uint8_t)  // Frame boundary
                           - sizeof(uint16_t)  // UUID
                           - sizeof(uint16_t));  // Payload length

typedef nanopb::EepromMessage<Config, NodeConfigValidator> config_t;
typedef nanopb::Message<State, NodeStateValidator> state_t;

class Node :
  public BaseNode,
  public BaseNodeEeprom,
  public BaseNodeI2c,
  public BaseNodeConfig<config_t>,
  public BaseNodeState<state_t>,
#ifndef DISABLE_SERIAL
  public BaseNodeSerialHandler,
#endif  // #ifndef DISABLE_SERIAL
  public BaseNodeI2cHandler {
public:
  typedef PacketParser<FixedPacket> parser_t;

  uint8_t buffer_[128];

  Node() :
    BaseNode(), BaseNodeConfig(Config_fields), BaseNodeState(State_fields) {}

  UInt8Array get_buffer() { return UInt8Array(sizeof(buffer_), buffer_); }
  /* This is a required method to provide a temporary buffer to the
   * `BaseNode...` classes. */

  void begin();
  uint16_t packet_crc(UInt8Array data);
  UInt8Array test_parser(UInt8Array data);

  void set_i2c_address(uint8_t value);
  void set_serial_number(uint32_t value) { config_._.serial_number = value; }
  uint32_t serial_number() { return config_._.serial_number; }

  uint32_t max_payload_size() { return PACKET_SIZE - FRAME_SIZE; }
  uint32_t sizeof_node() { return sizeof(Node); }
  uint32_t sizeof_config() { return sizeof(BaseNodeConfig<config_t>); }
  uint32_t sizeof_state() { return sizeof(BaseNodeState<state_t>); }
  uint32_t sizeof_parser() { return sizeof(parser_t); }
  uint32_t sizeof_packet_struct() { return sizeof(FixedPacket); }
  uint32_t sizeof_packet() { return PACKET_SIZE; }
#ifndef DISABLE_SERIAL
  uint32_t sizeof_serial_handler() {
    return sizeof(BaseNodeSerialHandler::handler_type);
  }
#endif  // #ifndef DISABLE_SERIAL
  uint32_t sizeof_i2c_handler() {
    return sizeof(BaseNodeI2cHandler::handler_type);
  }
};

}  // namespace wheeler_rpc


#endif  // #ifndef ___NODE__H___
