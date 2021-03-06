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
#include <math.h>

#include <sys/mman.h>

#include <sys/time.h>
    
#include <sys/types.h>
#include <sys/socket.h>

    
#include "spead_api.h"

#define BUFSIZE 1400

static volatile int run = 1;
#if 0
static volatile int child = 0;
#endif

void handle_us(int signum) 
{
  run = 0;
}
#if 0
void child_us(int signum)
{
  child = 1;
}
#endif

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
#if 0
  sa.sa_handler   = child_us;

  if (sigaction(SIGCHLD, &sa, NULL) < 0)
    return -1;
#endif

  return 0;
}

int usage(char **argv)
{
  fprintf(stderr, "usage:\n\t%s (options) destination"
                  "\n\n\tOptions"
                  "\n\t\t-h help"
                  "\n\t\t-p proto number"
                  "\n\t\t-s sender"
                  "\n\t\t-r receiver\n\n", argv[0]);
  return EX_USAGE;
}

int run_raw_sender(struct spead_socket *x)
{
  int rb, fd, i=0;
  unsigned char buffer[BUFSIZE+20];
  long flags=0;

#if 0
  struct data_file *df;
#endif

  fd = STDIN_FILENO;

  if (x == NULL)
    return -1;

  /*unset nonblocking i.e. blocking*/
  flags = fcntl(fd, F_GETFD, 0);
  flags &= ~O_NONBLOCK;
  if (fcntl(fd, F_SETFD, flags) < 0){
#ifdef DEBUG
    fprintf(stderr, "%s: error unsetting fcntl o_nonblock\n", __func__);
#endif
    return -1;
  }

#if 0
  df = load_raw_data_file("-");
  if (df == NULL)
    return -1;
#endif

  while (run){
#if 0
    rb = request_chunk_datafile(df, BUFSIZE, (void *) &buffer,  NULL);
    if (rb <= 0){
#ifdef DEBUG
      fprintf(stderr, "%s: unable to get chunk\n", __func__);
#endif
      continue;
    }
#endif
    rb = read(fd, buffer, BUFSIZE);
    if (rb <0){
      switch(errno){
        case EINTR:
        case EAGAIN:
          continue;
        case ESPIPE:
        default:
#ifdef DEBUG 
          fprintf(stderr, "%s: stream write error (%s)\n", __func__, strerror(errno));
#endif
          return -1;
      }
    }
    if (rb == 0){
#ifdef DEBUG
      fprintf(stderr, "%s: EOF\n", __func__);
#endif
      run = 0;
      break;
    }
    
#ifdef DEBUG
    fprintf(stderr, "%s: [%d] about to send %d bytes\n", __func__, i++, rb);
#endif

    if (send(x->x_fd, buffer, rb, MSG_CONFIRM) < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: send error (%s)\n", __func__, strerror(errno));
#endif
    }

#if 0
    //rb = recvfrom(x->x_fd, buffer, BUFSIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
    rb = recv(x->x_fd, buffer, BUFSIZE+20, 0);
    if (rb < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: unable to recvfrom: %s\n", __func__, strerror(errno));
#endif
      continue;
    } else if (rb == 0){
#ifdef DEBUG
      fprintf(stderr, "%s: peer shutdown detected\n", __func__);
#endif
      run = 0;
      continue;
    }
#ifdef DEBUG
    fprintf(stderr, "%s: [%d] received %d bytes\n", __func__, i++, rb);
#endif
#endif
    
    //usleep(100000);
  }

#if 0
  destroy_raw_data_file(df);
#endif
  return 0;
}

int run_raw_receiver(struct spead_socket *x)
{
  int rb,i=0;
  //unsigned char buffer[BUFSIZE+20];
  unsigned char buffer[BUFSIZE];
#if 1
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_len;
#endif

  struct data_file *df;

  if (x == NULL)
    return -1;

  df = write_raw_data_file("-");
  if (df == NULL)
    return -1;
  
  //bzero(buffer, BUFSIZE+20);
  bzero(buffer, BUFSIZE);
  
  while (run){
    
    rb = recvfrom(x->x_fd, buffer, BUFSIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
    //rb = recv(x->x_fd, buffer, BUFSIZE+20, 0);
    if (rb < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: unable to recvfrom: %s\n", __func__, strerror(errno));
#endif
      continue;
    } else if (rb == 0){
#ifdef DEBUG
      fprintf(stderr, "%s: peer shutdown detected\n", __func__);
#endif
      run = 0;
      continue;
    }

#ifdef DEBUG
    fprintf(stderr, "%s: [%d] received %d bytes from: %s:%d\n", __func__, i++, rb, inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
#endif

    //if (write_next_chunk_raw_data_file(df, buffer+20, rb-20) < 0){
    if (write_next_chunk_raw_data_file(df, buffer, rb) < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: cannot write to stream\n", __func__);
#endif
    }

#if 0
    /*echo reply*/
    if (sendto(x->x_fd, buffer+20, rb-20, MSG_CONFIRM, (struct sockaddr *) &peer_addr, peer_addr_len) < 0){
#ifdef DEBUG
      fprintf(stderr, "%s: sendto error (%s)\n", __func__, strerror(errno));
#endif
    }
#endif

  }

  destroy_raw_data_file(df);
  
  return 0;
}

int main(int argc, char *argv[])
{
  int i=1,j=1,k=0, flag = 0, proto = 155;
  char c, *host = NULL;

  struct spead_socket *x=NULL;

  if (argc < 3)
    return usage(argv);

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
        case 'r':
          j++;
          flag = 1;
          break;

        case 's':
          j++;
          flag = 0;
          break;

         case 'n':
          j++;
          flag = 2;
          break;

        case 'h':
          return usage(argv);

        /*settings*/
        case 'p':
          j++;
          if (argv[i][j] == '\0'){
            j = 0;
            i++;
          }
          if (i >= argc){
            fprintf(stderr, "%s: option -%c requires a parameter\n", argv[0], c);
            return EX_USAGE;
          }
          switch(c){
            case 'p':
              proto = atoi(argv[i] + j); 
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
        default:
          fprintf(stderr, "%s: extra argument %s\n", argv[0], argv[i]);
          return EX_USAGE;
      }
      i++;
      j=1;
    }
  }

  if (k < 1){
    fprintf(stderr, "%s: insufficient arguments\n", __func__);
    return EX_USAGE;
  }

  if (register_signals_us() < 0)
    return EX_SOFTWARE;

  x = create_raw_ip_spead_socket(host, proto);
  if (x == NULL){
#ifdef DEBUG
    fprintf(stderr, "%s\n", strerror(errno));
#endif
    return EX_SOFTWARE;
  }

  fprintf(stderr, "using proto number [%d]\n", proto);

  struct timeval tv;
  switch (flag){
    
    case 0: /*sender*/
#ifdef DEBUG
      fprintf(stderr, "%s: running sender\n", __func__);
#endif
      if (connect_spead_socket(x) < 0){
        goto cleanup;
      }
      
      if (run_raw_sender(x) < 0){
#ifdef DEBUG
        fprintf(stderr, "%s: run sender fail\n", __func__);
#endif
      }

      break;

    case 1: /*receiver*/
#ifdef DEBUG
      fprintf(stderr, "%s: running receiver\n", __func__);
#endif

      if (bind_spead_socket(x) < 0){
        goto cleanup;
      }

      if (run_raw_receiver(x) < 0){
#ifdef DEBUG
        fprintf(stderr, "%s: run receiver fail\n", __func__);
#endif
      }

      break;

    case 2:


      if (gettimeofday(&tv, NULL) < 0){
#ifdef DEBUG
        fprintf(stderr, "%s: gettimeofday() fail\n", __func__);
#endif
        return 1;
      }

#ifdef DEBUG
      fprintf(stderr, "%s: sending nothing @ %lu.%ld\n", __func__, tv.tv_sec, tv.tv_usec);
#endif

      if (connect_spead_socket(x) < 0){
        goto cleanup;
      }

      if (send(x->x_fd, NULL, 0, MSG_CONFIRM) < 0){
#ifdef DEBUG
        fprintf(stderr, "%s: send error (%s)\n", __func__, strerror(errno));
#endif
      }

      break;
  }

cleanup:  
  destroy_spead_socket(x);
  
  destroy_shared_mem();
#ifdef DEBUG
  fprintf(stderr,"%s: done\n", __func__);
#endif
  return 0;
}
