#include <stdio.h>
#include <netdb.h>

#include <sys/ioclt.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>

#include <net/if.h>
#include <net/ethernet.h>

#include <netinet/in.h>

#include <netpacket/packet.h>


typedef struct arp_packet 
{
  // ETH Header 
  byte1 dest_mac[6];
  byte1 src_mac[6];
  byte2 ether_type;
  // ARP Header
  byte2 hw_type;
  byte2 proto_type;
  byte1 hw_size;
  byte1 proto_size;
  byte2 arp_opcode;
  byte1 sender_mac[6];
  byte4 sender_ip;
  byte1 target_mac[6];
  byte4 target_ip;
  // Paddign
  char padding[18];
}ARP_PKT;

int main(int argc, char *argv[])
{
#ifdef DEBUG
  fprintf(stderr, "hello world\n", __func__);
#endif

}



