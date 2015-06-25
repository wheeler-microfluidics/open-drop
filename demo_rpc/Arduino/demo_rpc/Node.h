#ifndef ___NODE__H___
#define ___NODE__H___

#include <BaseNode.h>
#include <Array.h>
namespace demo_rpc {
#include "demo_rpc_config_pb.h"
}  // namespace demo_rpc


class Node : public BaseNode {
public:
  using BaseNode::output_buffer;
  using BaseNode::RETURN_CODE_;
  demo_rpc::Config config_;

  Node() : BaseNode() {
    load_config();
  }
  void reset_config() { config_ = Config_init_default; }
  void save_config() {
    UInt8Array serialized = serialize_config();
    if (serialized.data != NULL) {
      eeprom_update_block((void*)&serialized.length, (void*)0,
                          sizeof(uint16_t));
      update_eeprom_block(sizeof(uint16_t), serialized);
    }
  }
  UInt8Array serialize_config() {
    return BaseNode::serialize_obj(config_, demo_rpc::Config_fields);
  }
  void load_config() {
    if (!decode_obj_from_eeprom(0, config_, demo_rpc::Config_fields)) {
      /* Configuration could not be loaded from EEPROM; reset config. */
      reset_config();
      RETURN_CODE_ = 1;
    } else {
      RETURN_CODE_ = 0;
    }
    if (config_.i2c_address > 0 && config_.i2c_address > 0) {
      Wire.begin(static_cast<uint8_t>(config_.i2c_address));
    }
  }
  uint8_t return_code() { return RETURN_CODE_; }
  void set_serial_number(uint32_t value) { config_.serial_number = value; }
  uint32_t serial_number() { return config_.serial_number; }
  void set_i2c_address(uint32_t value) { config_.i2c_address = value; }
  uint32_t i2c_address() { return config_.i2c_address; }
};


#endif  // #ifndef ___NODE__H___
