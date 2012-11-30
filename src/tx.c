/* (c) 2012 SKA SA */
/* Released under the GNU GPLv3 - see COPYING */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sysexits.h>
#include <signal.h>
#include <netdb.h>


#include "avltree.h"
#include "spead_api.h"

static volatile int run = 1;
static volatile int child = 0;

struct spead_tx {
  struct spead_socket   *t_x;
  struct spead_workers  *t_w;
  struct data_file      *t_f;
  int                   t_pkt_size; 
  struct avl_tree       *t_t;
};

void handle_us(int signum) 
{
  run = 0;
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

void destroy_speadtx(struct spead_tx *tx)
{
  if (tx){
    destroy_spead_workers(tx->t_w);
    destroy_spead_socket(tx->t_x);
    destroy_raw_data_file(tx->t_f);
    free(tx);
  }
}

struct spead_tx *create_speadtx(char *host, char *port, char bcast, int pkt_size)
{
  struct spead_tx *tx; 

  tx = malloc(sizeof(struct spead_tx));
  if (tx == NULL){
#ifdef DEUBG
    fprintf(stderr, "%s: logic cannot malloc\n", __func__);
#endif
    return NULL;
  }

  tx->t_x         = NULL;
  tx->t_w         = NULL;
  tx->t_f         = NULL;
  tx->t_pkt_size  = pkt_size;
  tx->t_t         = NULL;

  tx->t_x = create_spead_socket(host, port);
  if (tx->t_x == NULL){
    destroy_speadtx(tx);
    return NULL;
  }
  
  if (connect_spead_socket(tx->t_x) < 0){
    destroy_speadtx(tx);
    return NULL;
  }

  if (bcast){
    set_broadcast_opt_spead_socket(tx->t_x);
  }

#ifdef DEBUG
  fprintf(stderr, "%s: pktsize: %d\n", __func__, pkt_size);
#endif
  
  return tx;
}

int worker_task_speadtx(void *data, struct spead_api_module *m, int cfd)
{
  struct spead_packet *p;
  struct spead_tx *tx;
  struct addrinfo *dst;
  pid_t pid;
  int i, sb, sfd;

  tx = data;
  if (tx == NULL)
    return -1;

  pid = getpid();
  sfd = get_fd_spead_socket(tx->t_x);
  dst = get_addr_spead_socket(tx->t_x);

  if (sfd <=0 || dst == NULL)
    return -1;

  p = malloc(sizeof(struct spead_packet));
  if (p == NULL)
    return -1;

#ifdef DEBUG
  fprintf(stderr, "%s: SPEADTX worker [%d] cfd[%d]\n", __func__, pid, cfd);
#endif
  
  spead_packet_init(p);
  
  p->n_items=6;
  p->is_stream_ctrl_term = SPEAD_STREAM_CTRL_TERM_VAL;
  
  SPEAD_SET_ITEM(p->data, 0, SPEAD_HEADER_BUILD(p->n_items));
  
  SPEAD_SET_ITEM(p->data, 1, SPEAD_ITEM_BUILD(SPEAD_IMMEDIATEADDR, SPEAD_HEAP_CNT_ID, 0x0));
  SPEAD_SET_ITEM(p->data, 2, SPEAD_ITEM_BUILD(SPEAD_IMMEDIATEADDR, SPEAD_HEAP_LEN_ID, 0x00));
  SPEAD_SET_ITEM(p->data, 3, SPEAD_ITEM_BUILD(SPEAD_IMMEDIATEADDR, SPEAD_PAYLOAD_OFF_ID, 0x0));
  SPEAD_SET_ITEM(p->data, 4, SPEAD_ITEM_BUILD(SPEAD_IMMEDIATEADDR, SPEAD_PAYLOAD_LEN_ID, 0x00));
  SPEAD_SET_ITEM(p->data, 5, SPEAD_ITEM_BUILD(SPEAD_IMMEDIATEADDR, 0x99, 555));
  SPEAD_SET_ITEM(p->data, 6, SPEAD_ITEM_BUILD(SPEAD_IMMEDIATEADDR, SPEAD_STREAM_CTRL_ID, SPEAD_STREAM_CTRL_TERM_VAL));


#if 0
  print_data(p->data, SPEAD_MAX_PACKET_LEN); 

  while(run){
    
    fprintf(stderr, "[%d]", pid);
    sleep(5);

  }
#endif

  sb = sendto(sfd, p->data, SPEAD_MAX_PACKET_LEN, 0, dst->ai_addr, dst->ai_addrlen);

#ifdef DEBUG
  fprintf(stderr, "[%d] sendto: %d bytes\n", pid, sb); 
#endif


  if (p)
    free(p);

#ifdef DEBUG
  fprintf(stderr, "%s: worker [%d] ending\n", __func__, pid);
#endif

  return 0;
}

struct avl_tree *create_spead_database()
{
  return create_avltree(&compare_spead_workers);
}

int register_speadtx(char *host, char *port, long workers, char broadcast, int pkt_size)
{
  struct spead_tx *tx;
  
  if (register_signals_us() < 0)
    return EX_SOFTWARE;
  
  tx = create_speadtx(host, port, broadcast, pkt_size);
  if (tx == NULL)
    return EX_SOFTWARE;

  tx->t_f = load_raw_data_file("/srv/pulsar/test.dat");
  if (tx->t_f == NULL){
    destroy_speadtx(tx);
    return EX_SOFTWARE;
  }

#if 0
  tx->t_t = create_spead_database();
  if (tx->t_t == NULL){
    destroy_speadtx(tx);
    return EX_SOFTWARE;
  }
#endif
  
  tx->t_w = create_spead_workers(tx, workers, &worker_task_speadtx);
  if (tx->t_w == NULL){
    destroy_speadtx(tx);
    return EX_SOFTWARE;
  }

  


  
  

  
  while (run){
    
    fprintf(stderr, ".");
    sleep(1);


    /*do stuff*/
    
    /*saw a SIGCHLD*/
    if (child){
      wait_spead_workers(tx->t_w);
    }
    
    

  }

  
  destroy_speadtx(tx);

#ifdef DEBUG
  fprintf(stderr, "%s: tx exiting cleanly\n", __func__);
#endif
  
  return 0;
}

int usage(char **argv, long cpus)
{
  fprintf(stderr, "usage:\n\t%s (options) destination port\n\n\tOptions\n\t\t-w [workers (d:%ld)]\n\t\t-x (enable send to broadcast [priv])\n\t\t-s [spead packet size]\n\n", argv[0], cpus);
  return EX_USAGE;
}

int main(int argc, char **argv)
{
  long cpus;
  char c, *port, *host, broadcast;
  int i,j,k, pkt_size;

  i = 1;
  j = 1;
  k = 0;

  host = NULL;

  broadcast = 0;
  
  pkt_size  = 1024;
  port      = PORT;
  cpus      = sysconf(_SC_NPROCESSORS_ONLN);

  if (argc < 2)
    return usage(argv, cpus);

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
          return usage(argv, cpus);

        /*settings*/
        case 's':
        case 'w':
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
            case 'w':
              cpus = atoll(argv[i] + j);
              break;
            case 's':
              pkt_size = atoi(argv[i] + j);
              if (pkt_size == 0)
                return usage(argv, cpus);
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
      /*parameters*/
      switch (k){
        case 0:
          host = argv[i];
          k++;
          break;
        case 1:
          port = argv[i];
          k++;
          break;
        default:
          fprintf(stderr, "%s: extra argument %s\n", argv[0], argv[i]);
          return EX_USAGE;
      }
      i++;
      j=1;
    }
  }
  
  

  return register_speadtx(host, port, cpus, broadcast, pkt_size);
}
  
