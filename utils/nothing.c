#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include "spead_api.h"

/*
Linux bug??
12:40:57.931463 b8:ac:6f:53:93:c4 > 8c:89:a5:30:00:ab, ethertype IPv4 (0x0800), length 34: (tos 0x0, ttl 64, id 33051, offset 0, flags [DF], proto unknown (155), length 20)
    10.0.1.2 > 10.0.1.5:  ip-proto-155 0
      0x0000:  4500 0014 811b 4000 409b a32d 0a00 0102  E.....@.@..-....
      0x0010:  0a00 0105                                ....
12:40:57.931539 8c:89:a5:30:00:ab > b8:ac:6f:53:93:c4, ethertype IPv4 (0x0800), length 60: (tos 0x0, ttl 64, id 9794, offset 0, flags [DF], proto unknown (155), length 20)
    10.0.1.5 > 10.0.1.2:  ip-proto-155 0
      0x0000:  4500 0014 2642 4000 409b fe06 0a00 0105  E...&B@.@.......
      0x0010:  0a00 0102 0000 0000 0000 0000 0000 0000  ................
      0x0020:  0000 0000 0000 0000 0000 0000 0000       ..............
*/

int main(int argc, char *argv[])
{
  char *host = NULL, nothing[sizeof(struct iphdr)];
  struct spead_socket *x=NULL;
  int bt;

  if (argc < 2){
#ifdef DEBUG
    fprintf(stderr,"set a destination for nothing\n");
#endif
    return 0;
  }
  
  host = argv[1];

  x = create_raw_ip_spead_socket(host);
  if (x == NULL){
#ifdef DEBUG
    fprintf(stderr, "%s\n", strerror(errno));
#endif
    return EX_SOFTWARE;
  }

#if 0
  if (bind_spead_socket(x) < 0){
    goto cleanup;
  }
#endif

  if (connect_spead_socket(x) < 0){
    goto cleanup;
  }

rec:
  if ((bt = recv(x->x_fd, nothing, sizeof(nothing), MSG_WAITALL)) < sizeof(nothing)){
#ifdef DEBUG
    fprintf(stderr, "we eventually recieved less than nothing\n");
#endif
    goto cleanup;
  }

#ifdef DEBUG
  fprintf(stderr, "received nothing %d\n", bt);
#endif

  if ((bt = send(x->x_fd, NULL, 0, MSG_CONFIRM)) < 0){
#ifdef DEBUG
    fprintf(stderr, "%s: send error (%s)\n", __func__, strerror(errno));
#endif
    goto cleanup;
  }

#ifdef DEBUG
  fprintf(stderr, "sent nothing %d\n", bt);
#endif
  
  goto rec;


cleanup:  
  destroy_spead_socket(x);
  destroy_shared_mem();

  return 0;
}
