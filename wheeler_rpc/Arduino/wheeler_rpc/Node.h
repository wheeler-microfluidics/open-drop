#ifndef ___NODE__H___
#define ___NODE__H___

#include <Arduino.h>
#include <NadaMQ.h>
#include <BaseNodeRpc.h>
#include <BaseNodeEeprom.h>
#include <BaseNodeI2c.h>
#include <BaseNodeConfig.h>
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

class Node :
  public BaseNode,
  public BaseNodeEeprom,
  public BaseNodeI2c,
  public BaseNodeConfig<config_t>,
#ifndef DISABLE_SERIAL
  public BaseNodeSerialHandler,
#endif  // #ifndef DISABLE_SERIAL
  public BaseNodeI2cHandler {
public:
  typedef PacketParser<FixedPacket> parser_t;

#if __AVR_ATmega2560__
  uint8_t buffer_[2048];
#else
  uint8_t buffer_[128];
#endif  // #if __AVR_ATmega2560__

  Node() : BaseNode(), BaseNodeConfig(Config_fields) {}

  UInt8Array get_buffer() { return UInt8Array(sizeof(buffer_), buffer_); }
  /* This is a required method to provide a temporary buffer to the
   * `BaseNode...` classes. */

  void begin();
  void set_i2c_address(uint8_t value);  // Override to validate i2c address
};

}  // namespace wheeler_rpc


#endif  // #ifndef ___NODE__H___
