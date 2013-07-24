#ifndef KIJI_LABEL_H_
#define KIJI_LABEL_H_

#include "../handy.h"

#define LABEL(name) \
  KijiLabel name; \
  Kiji_label_init(&name);

#define LABEL_PUT(name) \
  Kiji_label_put(&name, self)

#define LABEL_RESERVE(name) \
  Kiji_label_reserve(&name, self)

struct KijiCompiler;

struct KijiLabel {
  ssize_t address;

  ssize_t *reserved_addresses;
  size_t   num_reserved_addresses;
};

void Kiji_label_init(KijiLabel *self);
bool Kiji_label_is_solved(const KijiLabel *self);
void Kiji_label_reserve(KijiLabel *self, ssize_t address);
void Kiji_label_put(KijiLabel *self, KijiCompiler *compiler);

#endif /* KIJI_LABEL_H_ */
