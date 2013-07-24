#pragma once

#include <vector>

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
  std::vector<ssize_t> reserved_addresses;
};

void Kiji_label_put(KijiLabel *self, KijiCompiler *compiler);
void Kiji_label_reserve(KijiLabel *self, ssize_t address);
bool Kiji_label_is_solved(const KijiLabel *self);
KIJI_STATIC_INLINE void Kiji_label_init(KijiLabel *self) {
  self->address = -1;
}
