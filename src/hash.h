#ifndef HASH_H
#define HASH_H

#include <stdint.h>

#include "mutex.h"

struct hash_table;

struct hash_o_list {
  uint64_t l_len;
  uint64_t l_olen;
  struct hash_o *l_top;
  void *(*l_create)();
  void (*l_destroy)(void *data);
  mutex l_m;
};

/*create a list of objects each with element size*/
struct hash_o_list *create_o_list(uint64_t len, uint64_t hlen, uint64_t hsize, void *(*create)(), void (*destroy)(void *data), uint64_t size);
void destroy_o_list(struct hash_o_list *l);

int push_hash_o(struct hash_o_list *l, struct hash_o *o);
struct hash_o *pop_hash_o(struct hash_o_list *l);


struct hash_o {
  void *o;
  struct hash_o *o_next;
};

void destroy_hash_o(struct hash_o_list *l, struct hash_o *o);
struct hash_o *create_hash_o(void *(*create)(), void (*destroy)(void *data), uint64_t len);

struct hash_o *get_o_ht(struct hash_table *t, uint64_t id);
int add_o_ht(struct hash_table *t, struct hash_o *o);

void *get_data_hash_o(struct hash_o *o);


struct hash_table {
  mutex         t_m;
  uint64_t      t_id;
  uint64_t      t_len;
  uint64_t      t_data_count;
  int64_t       t_data_id;
  uint64_t      t_items;
  struct hash_o_list *t_l;
  struct hash_o **t_os;

  uint64_t (*t_hfn)(struct hash_table *t, struct hash_o *o);
#if 0
  uint64_t (*t_hfn)(struct hash_table *t, uint64_t in);
#endif
};


void destroy_hash_table(struct hash_table *t);
struct hash_table *create_hash_table(struct hash_o_list *l, uint64_t id, uint64_t len, uint64_t (*hfn)(struct hash_table *t, struct hash_o *o));

int empty_hash_table(struct hash_table *ht);

#endif

