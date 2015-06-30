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
#include "packet_handler.h"


demo_rpc::Node node_obj;
demo_rpc::CommandProcessor<demo_rpc::Node> command_processor(node_obj);

#ifndef DISABLE_SERIAL
typedef CommandPacketHandler
  <Stream, demo_rpc::CommandProcessor<demo_rpc::Node> > Handler;
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


void i2c_receive_event(int byte_count) { node_obj.i2c_handler_.receiver()(byte_count); }


void setup() {
  node_obj.begin();
#if !defined(DISABLE_SERIAL)
  packet.reset_buffer(PACKET_SIZE, &packet_buffer[0]);
  parser.reset(&packet);
#endif  // #ifndef DISABLE_SERIAL
  Wire.onReceive(i2c_receive_event);
}


void loop() {
#ifndef DISABLE_SERIAL
  /* Parse all new bytes that are available.  If the parsed bytes result in a
   * completed packet, pass the complete packet to the command-processor to
   * process the request. */
  reactor.parse_available();
#endif  // #ifndef DISABLE_SERIAL
  if (node_obj.i2c_handler_.packet_ready()) {
    node_obj.i2c_handler_.process_packet(command_processor);
  }
}
