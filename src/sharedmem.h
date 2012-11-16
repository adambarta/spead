/* (c) 2012 SKA SA */
/* Released under the GNU GPLv3 - see COPYING */

#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#include <stdint.h>

struct shared_mem {
  key_t     m_key;
  int       m_id;
  uint64_t  m_size;
  uint64_t  m_off;
  void      *m_ptr;
};

int create_shared_mem(uint64_t size);
void destroy_shared_mem();
void *shared_malloc(size_t size);

#if 0
void shared_free(void *ptr);
#endif

#endif
