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
#include "dmf_control_rpc_config_validate.h"
namespace dmf_control_rpc {
#include "dmf_control_rpc_config_pb.h"


const size_t FRAME_SIZE = (3 * sizeof(uint8_t)  // Frame boundary
                           - sizeof(uint16_t)  // UUID
                           - sizeof(uint16_t));  // Payload length

class Node;

typedef nanopb::EepromMessage<Config, NodeConfigValidator> config_t;
typedef nanopb::Message<State, NodeStateValidator<Node> > state_t;

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

  Node() : BaseNode(), BaseNodeConfig(Config_fields),
           BaseNodeState(State_fields) {}

  UInt8Array get_buffer() { return UInt8Array(sizeof(buffer_), buffer_); }
  /* This is a required method to provide a temporary buffer to the
   * `BaseNode...` classes. */

  void begin();
  void set_i2c_address(uint8_t value);  // Override to validate i2c address

  bool on_voltage_changed(float original_value, float new_value) {
    /* This method is triggered whenever a voltage is included in a state
     * update. */
    return (0 <= new_value) && (new_value <= config_._.max_waveform_voltage);
  }
  bool on_frequency_changed(float original_value, float new_value) {
    /* This method is triggered whenever a frequency is included in a state
     * update. */
    return ((config_._.min_waveform_frequency <= new_value) &&
            (new_value <= config_._.max_waveform_frequency));
  }
};

}  // namespace dmf_control_rpc


#endif  // #ifndef ___NODE__H___
