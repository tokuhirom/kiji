#ifndef KIJI_LABEL_H_
#define KIJI_LABEL_H_

#include "../handy.h"
#include <stdbool.h>

#define LABEL(name) \
  KijiLabel name; \
  Kiji_label_init(&name);

#define LABEL_PUT(name) \
  Kiji_label_put(&name, self)

#define LABEL_RESERVE(name) \
  Kiji_label_reserve(&name, self)

struct _KijiCompiler;

typedef struct _KijiLabel {
  ssize_t address;

  ssize_t *reserved_addresses;
  size_t   num_reserved_addresses;
} KijiLabel;

#ifdef __cplusplus
extern "C" {
#endif
void Kiji_label_init(KijiLabel *self);
bool Kiji_label_is_solved(const KijiLabel *self);
void Kiji_label_reserve(KijiLabel *self, ssize_t address);
void Kiji_label_put(KijiLabel *self, struct _KijiCompiler *compiler);
#ifdef __cplusplus
};
#endif

#endif /* KIJI_LABEL_H_ */
