#ifndef ___NODE__H___
#define ___NODE__H___

#include <stdint.h>
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
#include "open_drop_config_validate.h"
#include "open_drop_state_validate.h"
namespace open_drop {
#include "open_drop_config_pb.h"


const size_t FRAME_SIZE = (3 * sizeof(uint8_t)  // Frame boundary
                           - sizeof(uint16_t)  // UUID
                           - sizeof(uint16_t)  // Payload length
                           - sizeof(uint16_t));  // CRC

class Node;

typedef nanopb::EepromMessage<open_drop_Config,
                              typename
                              config_validate::Validator<Node> > config_t;
typedef nanopb::Message<open_drop_State,
                        typename state_validate::Validator<Node> > state_t;

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

  static const uint16_t BUFFER_SIZE = 128;  // >= longest property string
  static const uint16_t CHANNEL_COUNT = 40;

  uint8_t buffer_[BUFFER_SIZE];
  uint8_t state_of_channels_[CHANNEL_COUNT / 8];  // 8 channels per byte

  Node() : BaseNode(), BaseNodeConfig(open_drop_Config_fields),
           BaseNodeState(open_drop_State_fields) {}

  UInt8Array get_buffer() { return UInt8Array(sizeof(buffer_), buffer_); }
  /* This is a required method to provide a temporary buffer to the
   * `BaseNode...` classes. */

  void begin();
  void set_i2c_address(uint8_t value);  // Override to validate i2c address

  UInt8Array state_of_channels() const {
    return UInt8Array(sizeof(state_of_channels_),
                      (uint8_t *)&state_of_channels_[0]);
  }
  bool set_state_of_channels(UInt8Array channel_states) {
    if (channel_states.length == sizeof(state_of_channels_)) {
      for (int i = 0; i < channel_states.length; i++) {
        state_of_channels_[i] = channel_states.data[i];
      }
      return true;
    }
    return false;
  }

  uint16_t channel_count() const { return CHANNEL_COUNT; }

  bool on_state_voltage_changed(float original_value, float new_value) {
    /* This method is triggered whenever a voltage is included in a state
     * update. */
    return (0 <= new_value) && (new_value <= config_._.max_waveform_voltage);
  }
  bool on_state_frequency_changed(float new_value) {
    /* This method is triggered whenever a frequency is included in a state
     * update. */
    return ((config_._.min_waveform_frequency <= new_value) &&
            (new_value <= config_._.max_waveform_frequency));
  }
};

}  // namespace open_drop


#endif  // #ifndef ___NODE__H___
