#include "EEPROM.h"
#include "SPI.h"
#include "Wire.h"
#include "Memory.h"
#include "nanopb.h"
#include "NadaMQ.h"
#include "ArduinoRPC.h"
#include "Array.h"
#include "RPCBuffer.h"
#include "NodeCommandProcessor.h"
#include "Node.h"
#include "I2cHandler.h"
#include "packet_handler.h"


Node node_obj;
demo_rpc::CommandProcessor<Node> command_processor(node_obj);

#ifndef DISABLE_SERIAL
typedef CommandPacketHandler
  <Stream, demo_rpc::CommandProcessor<Node> > Handler;
typedef PacketReactor<PacketParser<FixedPacket>, Stream, Handler> Reactor;

FixedPacket packet;
/* `reactor` maintains parse state for a packet, and updates state one-byte
 * at-a-time. */
PacketParser<FixedPacket> parser;
/* `handler` processes complete packets and sends response as necessary. */
Handler handler(Serial, command_processor);
/* `reactor` uses `parser` to parse packets from input stream and passes
 * complete packets to `handler` for processing. */
Reactor reactor(parser, Serial, handler);
#endif  // #ifndef DISABLE_SERIAL
base_node::I2cHandler<FixedPacket> i2c_handler;


void setup() {
#ifdef __AVR_ATmega2560__
  /* Join I2C bus as master. */
  Wire.begin();
#else
  /* Join I2C bus as slave. */
  Wire.onReceive(i2c_receive_event);
  Wire.onRequest(i2c_request_event);
#endif  // #ifdef __AVR_ATmega328__
  // Set i2c clock-rate to 400kHz.
  TWBR = 12;
#if !defined(DISABLE_SERIAL)
  Serial.begin(115200);
  packet.reset_buffer(PACKET_SIZE, &packet_buffer[0]);
  parser.reset(&packet);
#endif  // #ifndef DISABLE_SERIAL
  i2c_handler.request_packet.reset_buffer(I2C_PACKET_SIZE,
                                          &i2c_packet_buffer[0]);
}


void loop() {
#ifndef DISABLE_SERIAL
  /* Parse all new bytes that are available.  If the parsed bytes result in a
   * completed packet, pass the complete packet to the command-processor to
   * process the request. */
  reactor.parse_available();
#endif  // #ifndef DISABLE_SERIAL
  i2c_handler.process_available(command_processor);
}


void i2c_receive_event(int byte_count) { i2c_handler.on_receive(byte_count); }
void i2c_request_event() { i2c_handler.on_request(); }
