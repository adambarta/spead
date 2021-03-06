/* MULTICORE UDP RECEIVER*/
/* Released under the GNU GPLv3 - see COPYING */
/*  Author: Adam Barta (adam@ska.ac.za)*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <wait.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#ifndef IKATCP
#include <katcl.h>
#include <katcp.h>
#endif

#include "mcs.h"
#include "stack.h"
#include "spead_api.h"
#include "hash.h"
#include "mutex.h"


static volatile int run = 1;
static volatile int timer = 0;
static volatile int child = 0;


void handle_us(int signum) 
{
  run = 0;
}

void timer_us(int signum) 
{
  timer = 1;
}

void child_us(int signum)
{
  child = 1;
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

  sa.sa_handler   = child_us;

  if (sigaction(SIGCHLD, &sa, NULL) < 0)
    return -1;

  return 0;
}

struct mcs_rx *create_server_us(long cpus, char *raw_pkt_file)
{
  struct mcs_rx *s;

  if (cpus < 1){
#ifdef DEBUG
    fprintf(stderr, "%s: must have at least 1 cpu\n", __func__);
#endif
    return NULL;
  }

  s = mmap(NULL, sizeof(struct mcs_rx), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, (-1), 0);
  if (s == MAP_FAILED)
    return NULL;
  
  s->s_x       = NULL;
  s->s_w       = NULL;
  s->s_f       = NULL;
  s->s_pc      = 0;
  s->s_bc      = 0;
  s->s_cpus    = cpus;
  s->s_m       = 0;

  if (raw_pkt_file){
    s->s_f = write_raw_data_file(raw_pkt_file);
  }

  return s;
}

void destroy_server_us(struct mcs_rx *s)
{
  if (s){

    destroy_spead_socket(s->s_x);
    destroy_spead_workers(s->s_w);
    
    destroy_raw_data_file(s->s_f);

    destroy_shared_mem();
    munmap(s, sizeof(struct mcs_rx));
  }

#ifdef DEBUG
  fprintf(stderr, "%s: destroyed server\n",  __func__);
#endif
}

int startup_server_us(struct mcs_rx *s, char *port, int broadcast, char *multi_grp, char *multi_if)
{
  if (s == NULL || port == NULL)
    return -1;
  
  s->s_x = create_udp_spead_socket(NULL, port);
  if (s->s_x == NULL){
#ifdef DEBUG
    fprintf(stderr, "%s: cannot create spead socket\n", __func__);
#endif
    return -1;
  }
  
  if (broadcast){
    if (set_broadcast_opt_spead_socket(s->s_x) < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: WARN spead socket broadcast option not set\n", __func__);
#endif
    }
  }

  if (bind_spead_socket(s->s_x) < 0)
    return -1;
  
  if (multi_grp && multi_if){
    if (set_multicast_receive_opts_spead_socket(s->s_x, multi_grp, multi_if) < 0){
      return -1;
    }
#ifdef DEBUG
    fprintf(stderr, "%s: running in multicast mode\n", __func__);
#endif
  }

  //get_fd_spead_socket(s->s_x);

#ifdef DEBUG
  fprintf(stderr,"\tSERVER:\t\t[%d]\n\tport:\t\t%s\n\tnice:\t\t%d\n", getpid(), port, nice(0));
#endif

  return 0;
}

void shutdown_server_us(struct mcs_rx *s)
{
  if (s){
    destroy_server_us(s);
  }
#ifdef DEBUG
  fprintf(stderr, "%s: server shutdown complete\n", __func__);
#endif
} 


void print_bitrate(struct mcs_rx *s, char x, uint64_t bps)
{
  char *rates[] = {"bits", "kbits", "mbits", "gbits", "tbits"};
  int i;
  double style;
  uint64_t bitsps;
  
  if (s == NULL)
    return;

  bitsps = bps * 8;
  bps = bitsps;
  style = 0;

#ifdef DATA
  if (bps > 0){
    
    for (i=0; (bitsps / 1024) > 0; i++, bitsps /= 1024){
      style = bitsps / 1024.0;
    }
    
    switch(x){

      case 'T':
        fprintf(stderr, "TOTAL\t[%d]:\t%10.6f %s\n", getpid(), style, rates[i]);
        break;

      case 'R':
        fprintf(stderr, "RATE\t[%d]:\t%10.6f %sps %ld bps\n", getpid(), style, rates[i], bps);
        break;

      case 'D':
        fprintf(stderr, "DATA\t[%d]:\t%10.6f %s\n", getpid(), style, rates[i]);
        break;
    }
  }
#endif
}

int server_run_loop(struct mcs_rx *s)
{
  struct timespec ts;
  struct sigaction sa;
  uint64_t total;
  int rtn;
  sigset_t empty_mask;

  if (s == NULL){
#ifdef DEBUG
    fprintf(stderr, "%s: param error\n", __func__);
#endif  
    return -1;
  }

  sigfillset(&sa.sa_mask);
  sa.sa_handler   = timer_us;
  sa.sa_flags     = 0;

  sigaction(SIGALRM, &sa, NULL);
  
  alarm(1);

  sigemptyset(&empty_mask);

  total = 0;

  ts.tv_sec  = 0;
  ts.tv_nsec = 100000000;

  while (run){

    if (populate_fdset_spead_workers(s->s_w) < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: error populating fdset\n", __func__);
#endif
      run = 0;
      break;
    }

    rtn = pselect(get_high_fd_spead_workers(s->s_w) + 1, get_in_fd_set_spead_workers(s->s_w), (fd_set *) NULL, (fd_set *) NULL, &ts, &empty_mask);
    if (rtn < 0){
      switch(errno){
        case EAGAIN:
        case EINTR:
          //continue;
          break;
        default:
#ifdef DEBUG
          fprintf(stderr, "%s: pselect error\n", __func__);
#endif    
          run = 0;
          continue;
      }
    }

    
#ifdef DATA
    if (timer){
      lock_mutex(&(s->s_m));
      total = s->s_bc - total;
      unlock_mutex(&(s->s_m));
      print_bitrate(s, 'R', total);

      alarm(1);
      timer = 0;
      lock_mutex(&(s->s_m));
      total = s->s_bc;
      unlock_mutex(&(s->s_m));
    }
#endif

    /*saw a SIGCHLD*/
    if (child){
      wait_spead_workers(s->s_w);
    }
  }

  fprintf(stderr, "%s: final packet count: %ld\n", __func__, s->s_pc);

#ifdef DEBUG
  fprintf(stderr, "%s: rx exiting cleanly\n", __func__);
#endif

  return 0;
}

int raw_spead_cap_worker(void *data, struct spead_pipeline *l, int cfd)
{
  struct mcs_rx *s;
  struct spead_packet *p;

  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len;

  size_t nread;
  
  struct data_file *f;

  s = data;
  if (s == NULL){
#ifdef DEBUG
    fprintf(stderr, "%s: param error\n", __func__);
#endif
    return -1;
  }
  
  f = s->s_f;
  if (f == NULL){
#ifdef DEBUG
    fprintf(stderr, "%s: data file not ready\n", __func__);
#endif
    return -1;
  }

  p = malloc(sizeof(struct spead_packet));
  if (p == NULL) {
#ifdef DEBUG
    fprintf(stderr, "%s: logic error cannot malloc\n", __func__);
#endif
    return -1;
  }

#ifdef DEBUG
  fprintf(stderr, "\t  CHILD\t\t[%d]\n", getpid());
#endif
  
  peer_addr_len = sizeof(struct sockaddr_storage);

  while(run){
    
    nread = recvfrom(get_fd_spead_socket(s->s_x), p->data, SPEAD_MAX_PACKET_LEN, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
    if (nread < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: unable to recvfrom: %s\n", __func__, strerror(errno));
#endif
      continue;
    } else if (nread == 0){
#ifdef DEBUG
      fprintf(stderr, "%s: peer shutdown detected\n", __func__);
#endif
      continue;
    }
    
    if (write_next_chunk_raw_data_file(f, p->data, nread) < 0) {
#ifdef DEBUG
      fprintf(stderr, "%s: write next chunk fail\n", __func__);
#endif
    }
    
    lock_mutex(&(s->s_m));
    s->s_bc += nread;
    s->s_pc++;
    unlock_mutex(&(s->s_m));

  }
  
  if (p)
    free(p);

  return 0;
}

int mcs_packet_worker(void *data, struct spead_pipeline *l, int cfd)
{
  struct mcs_rx *s;
  struct spead_packet *p;

  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len;

  size_t nread;
  
  s = data;
  if (s == NULL){
#ifdef DEBUG
    fprintf(stderr, "%s: param error\n", __func__);
#endif
    return -1;
  }
  
  p = malloc(sizeof(struct spead_packet));
  if (p == NULL) {
#ifdef DEBUG
    fprintf(stderr, "%s: logic error cannot malloc\n", __func__);
#endif
    return -1;
  }

#ifdef DEBUG
  fprintf(stderr, "\t  CHILD\t\t[%d]\n", getpid());
#endif
  
  peer_addr_len = sizeof(struct sockaddr_storage);

  while(run){
    
    nread = recvfrom(get_fd_spead_socket(s->s_x), p->data, SPEAD_MAX_PACKET_LEN, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
    if (nread < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: unable to recvfrom: %s\n", __func__, strerror(errno));
#endif
      continue;
    } else if (nread == 0){
#ifdef DEBUG
      fprintf(stderr, "%s: peer shutdown detected\n", __func__);
#endif
      continue;
    }
    
    if (mcs_rx_packet_callback(p->data, nread) < 0) {
#ifdef DEBUG
      fprintf(stderr, "%s: write next chunk fail\n", __func__);
#endif
    }
    
    lock_mutex(&(s->s_m));
    s->s_bc += nread;
    s->s_pc++;
    unlock_mutex(&(s->s_m));

  }
  
  if (p)
    free(p);

  return 0;
}

int register_client_handler_server(char *port, long cpus, int broadcast, char *multi_grp, char *multi_if, char *raw_pkt_file)
{
  struct mcs_rx *s;
  
  if (register_signals_us() < 0){
    fprintf(stderr, "%s: error register signals\n", __func__);
    return -1;
  }

  s = create_server_us(cpus, raw_pkt_file);
  if (s == NULL){
    fprintf(stderr, "%s: error could not create server\n", __func__);
    return -1;
  }

  if (startup_server_us(s, port, broadcast, multi_grp, multi_if) < 0){
    fprintf(stderr,"%s: error in startup\n", __func__);
    shutdown_server_us(s);
    return -1;
  }


  if (raw_pkt_file){
    s->s_w = create_spead_workers(NULL, s, cpus, &raw_spead_cap_worker);
    if (s->s_w == NULL){
      shutdown_server_us(s);
      fprintf(stderr, "%s: create spead workers failed\n", __func__);
      return -1;
    }
  } else {
    s->s_w = create_spead_workers(NULL, s, cpus, &mcs_packet_worker);
    if (s->s_w == NULL){
      shutdown_server_us(s);
      fprintf(stderr, "%s: create spead workers failed\n", __func__);
      return -1;
    }
  }


  if (server_run_loop(s) < 0){
    shutdown_server_us(s);
    fprintf(stderr, "%s: server run loop failed\n", __func__);
    return -1;
  }
  
  shutdown_server_us(s);

#ifdef DEBUG
  fprintf(stderr,"%s: server exiting\n", __func__);
#endif

  return 0;
}


int main(int argc, char *argv[])
{
  long cpus;
  int i, j, c, broadcast;
  char *port, *raw_pkt_file, multi_start, *multi_grp, *multi_if;
  
  if (check_spead_version(VERSION) < 0){
    fprintf(stderr, "%s: FATAL version mismatch\n", __func__);
    return EX_SOFTWARE;
  }

  i = 1;
  j = 1;
  broadcast = 0;
  multi_start = 0;
  multi_grp = NULL;
  multi_if = NULL;

  raw_pkt_file = NULL;

  port = PORT;
  cpus = sysconf(_SC_NPROCESSORS_ONLN);

  while (i < argc){
    if (argv[i][0] == '-'){
      c = argv[i][j];

      switch(c){
        case '\0':
          j = 1;
          i++;
          break;
        case '-':
          j++;
          break;

        /*switches*/  
        case 'x':
          j++;
          broadcast = 1;
          break;

        case 'h':
          fprintf(stderr, "usage:\n\t%s\n\t\t-w [workers (d:%ld)]\n\t\t-p [port (d:%s)]\n\t\t-x (enable receive from broadcast [priv])\n\t\t-r (dump raw packets)\n\t\t-m multicast_group_ip local_interface_ip\n\n", argv[0], cpus, port);
          return EX_OK;

        /*settings*/
        case 'r':
        case 'p':
        case 'w':
        case 'm':
          j++;
          if (argv[i][j] == '\0'){
            j = 0;
            i++;
          }
          if (i >= argc){
            fprintf(stderr, "%s: option -%c requires a parameter\n", argv[0], c);
            return EX_USAGE;
          }
          switch (c){
            case 'r':
              raw_pkt_file = argv[i] +j;
              break;
            case 'p':
              port = argv[i] + j;  
              break;
            case 'w':
              cpus = atoll(argv[i] + j);
              break;
            case 'm':
              multi_start = 1;
              multi_grp = argv[i]+j;
              break;
          }
          i++;
          j = 1;
          break;

        default:
          fprintf(stderr, "%s: unknown option -%c\n", argv[0], c);
          return EX_USAGE;
      }

    } else {
      if (multi_start){
        multi_start = 0;
        multi_if = argv[i];
        i++;  
      } else {
        fprintf(stderr, "%s: extra argument %s\n", argv[0], argv[i]);
        return EX_USAGE;
      }
    }
    
  }

  return register_client_handler_server(port, cpus, broadcast, multi_grp, multi_if, raw_pkt_file);
}

