/* (c) 2012 SKA SA */
/* Released under the GNU GPLv3 - see COPYING */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/utsname.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "udpserver.h"


static volatile int run = 1;

void handle_us(int signum) 
{
  run = 0;
}

int register_signals_us()
{
  struct sigaction sa;

  sigfillset(&sa.sa_mask);
  sa.sa_handler   = handle_us;
  sa.sa_flags     = 0;

  if (sigaction(SIGINT, &sa, NULL) < 0)
    return -1;

  if (sigaction(SIGTERM, &sa, NULL) < 0)
    return -1;

  return 0;
}

struct u_server *create_server_us(int (*cdfn)(struct u_client *c))
{
  struct u_server *s;

#if 1
  if (cdfn == NULL)
    return NULL;
  
  s = malloc(sizeof(struct u_server));
  if (s == NULL)
    return NULL;
  
  s->s_fd = 0;
  s->s_bc = 0;

#endif
  return s;
}

void destroy_server_us(struct u_server *s)
{
  if (s)
    free(s);

#ifdef DEBUG
  fprintf(stderr, "%s: destroyed server\n",  __func__);
#endif
}

int startup_server_us(struct u_server *s, char *port)
{
  struct addrinfo hints;
  struct addrinfo *res, *rp;
  int backlog, reuse_addr;

  if (s == NULL || port == NULL)
    return -1;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family     = AF_UNSPEC;
  hints.ai_socktype   = SOCK_DGRAM;
  hints.ai_flags      = AI_PASSIVE;
  hints.ai_protocol   = 0;
  hints.ai_canonname  = NULL;
  hints.ai_addr       = NULL;
  hints.ai_next       = NULL;
  
  if ((reuse_addr = getaddrinfo(NULL, port, &hints, &res)) != 0) {
#ifdef DEBUG
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(reuse_addr));
#endif
    return -1;
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    if (rp->ai_family == AF_INET6)
      break;
  }

  rp = (rp == NULL) ? res : rp;

  s->s_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
  if (s->s_fd < 0){
#ifdef DEBUG
    fprintf(stderr,"%s: error socket\n", __func__);
#endif
    freeaddrinfo(res);
    return -1;
  }

  reuse_addr   = 1;

  setsockopt(s->s_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

  if (bind(s->s_fd, rp->ai_addr, rp->ai_addrlen) < 0){
#ifdef DEBUG
    fprintf(stderr,"%s: error bind on port: %s\n", __func__, port);
#endif
    freeaddrinfo(res);
    return -1;
  }

  freeaddrinfo(res);

  backlog      = 10;

#ifdef DEBUG
  fprintf(stderr,"%s: server pid: %d running on port: %s\n", __func__, getpid(), port);
#endif

  return 0;
}

void shutdown_server_us(struct u_server *s)
{
  if (s){

    if (close(s->s_fd) < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: error server shutdown: %s\n", __func__, strerror(errno));
#endif
    }

    destroy_server_us(s);
  }

#ifdef DEBUG
  fprintf(stderr, "%s: server shutdown complete\n", __func__);
#endif
} 


int run_loop_us(struct u_server *s)
{
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len;
  unsigned char *buf;
  ssize_t nread;
  int rtn;
  
  if (s == NULL)
    return -1;

  buf = malloc(sizeof(unsigned char)*BUF_SIZE);
  if (buf == NULL)
    return -1;

  while (run) {
    
    peer_addr_len = sizeof(struct sockaddr_storage); 
    nread = recvfrom(s->s_fd, buf, BUF_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
    if (nread == -1)
      continue;               /* Ignore failed request */

    //char host[NI_MAXHOST], service[NI_MAXSERV];

    //rtn = getnameinfo((struct sockaddr *) &peer_addr, peer_addr_len, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
#if 0
#ifdef DEBUG
    if (rtn == 0)
      printf("Received %ld bytes from %s:%s\n", (long) nread, host, service);
    else
      fprintf(stderr, "getnameinfo: %s\n", gai_strerror(rtn));
#endif
#endif

#if 0
    if (sendto(s->s_fd, buf, nread, 0, (struct sockaddr *) &peer_addr, peer_addr_len) != nread)
#ifdef DEBUG
      fprintf(stderr, "Error sending response\n");
#endif
#endif 

    s->s_bc += nread;

  }

  free(buf);

#ifdef DEBUG
  fprintf(stderr, "%s: final recv count: %llu bytes\n", __func__, s->s_bc);
#endif

  return 0;
}

int register_client_handler_server(int (*client_data_fn)(struct u_client *c), char *port)
{
  struct u_server *s;
  
  if (register_signals_us() < 0){
#ifdef DEBUG
    fprintf(stderr, "%s: error register signals\n", __func__);
#endif
    return -1;
  }

  s = create_server_us(client_data_fn);
  if (s == NULL){
#ifdef DEBUG
    fprintf(stderr, "%s: error could not create server\n", __func__);
#endif
    return -1;
  }

  if (startup_server_us(s, port) < 0){
#ifdef DEBUG
    fprintf(stderr,"%s: error in startup\n", __func__);
#endif
    shutdown_server_us(s);
    return -1;
  }

  if (run_loop_us(s) < 0){ 
#ifdef DEBUG
    fprintf(stderr,"%s: error during run\n", __func__);
#endif
    shutdown_server_us(s);
    return -1;
  }
  
  shutdown_server_us(s);

#ifdef DEBUG
  fprintf(stderr,"%s: server exiting\n", __func__);
#endif

  return 0;
}


int capture_client_data(struct u_client *c)
{

}

int main(int argc, char *argv[])
{
  
  return register_client_handler_server(&capture_client_data , PORT);
}
