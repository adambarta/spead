/* (c) 2012 SKA SA */
/* Released under the GNU GPLv3 - see COPYING */

#ifndef SPEAD_API_H
#define SPEAD_API_H

#include <sys/stat.h>

#include "spead_packet.h"
#include "hash.h"
#include "server.h"

#define S_END             0
#define S_MODE            1
#define S_MODE_IMMEDIATE  2
#define S_MODE_DIRECT     3
#define S_GET_PACKET      4
#define S_NEXT_PACKET     5
#define S_GET_ITEM        6
#define S_NEXT_ITEM       7
#define S_GET_OBJECT      8
#define S_DIRECT_COPY     9
#define DC_NEXT_PACKET   10

void print_data(unsigned char *buf, int size);


struct spead_heap_store{
  int64_t             s_backlog;
  struct hash_o_list  *s_list;
  struct hash_table   **s_hash;
};

struct spead_heap_store *create_store_hs(uint64_t list_len, uint64_t hash_table_count, uint64_t hash_table_size);
void destroy_store_hs(struct spead_heap_store *hs);
int add_heap_hs(struct spead_heap_store *hs, struct spead_heap *h);
struct spead_heap *get_heap_hs(struct spead_heap_store *hs, int64_t hid);

void print_store_stats(struct spead_heap_store *hs);

struct spead_api_item{
  int           i_valid;
  int           i_id;
  void          *io_data;
  size_t        io_size;
  uint64_t      i_len;
  unsigned char i_data[];
};

struct spead_item_group {
  uint64_t  g_items;
  uint64_t  g_size;
  uint64_t  g_off;
  void      *g_map;
};


struct spead_item_group *create_item_group(uint64_t datasize, uint64_t nitems);
void destroy_item_group(struct spead_item_group *ig);
struct spead_api_item *new_item_from_group(struct spead_item_group *ig, uint64_t size);
#if 0
int grow_spead_item_group(struct spead_item_group *ig, uint64_t extradata, uint64_t extranitems);
struct spead_api_item *get_spead_item(struct spead_item_group *ig, uint64_t n);
#endif
struct spead_api_item *get_spead_item_at_off(struct spead_item_group *ig, uint64_t off);
int set_spead_item_io_data(struct spead_api_item *itm, void *ptr, size_t size);
int copy_to_spead_item(struct spead_api_item *itm, void *src, size_t len);

#if 0
struct spead_api_item *init_spead_api_item(struct spead_api_item *itm, int vaild, int id, int len, unsigned char *data);
#endif

int process_packet_hs(struct u_server *s, struct spead_api_module *m, struct hash_o *o);  

/*modules api*/

#define SAPI_CALLBACK "spead_api_callback"
#define SAPI_SETUP    "spead_api_setup"
#define SAPI_DESTROY  "spead_api_destroy"

struct spead_api_module {
  void *m_handle;
  void *(*m_setup)();
  int  (*m_cdfn)();
  int  (*m_destroy)(void *data);
  void *m_data;
};


struct spead_api_module *load_api_user_module(char *mod);
void unload_api_user_module(struct spead_api_module *m);
int setup_api_user_module(struct spead_api_module *m);
int destroy_api_user_module(struct spead_api_module *m);
int run_api_user_callback_module(struct spead_api_module *m, struct spead_item_group *ig);

/*shared_mem api*/

struct shared_mem {
  uint64_t  m_size;
  uint64_t  m_off;
  void      *m_ptr;
};

int create_shared_mem(uint64_t size);
void destroy_shared_mem();
void *shared_malloc(size_t size);

struct data_file{
  struct stat f_fs;
  int         f_fd;
  void        *f_fmap;
};

struct data_file *load_raw_data_file(char *fname);
void destroy_raw_data_file(struct data_file *f);
size_t get_data_file_size(struct data_file *f);
void *get_data_file_ptr_at_off(struct data_file *f, uint64_t off);

#endif

