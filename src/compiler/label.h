#pragma once

#include <vector>

#define LABEL(name) \
  KijiLabel name(self);

struct KijiCompiler;

class KijiLabel {
private:
  KijiCompiler *compiler_;
  ssize_t address_;
  std::vector<ssize_t> reserved_addresses_;
public:
  KijiLabel(KijiCompiler *compiler) :compiler_(compiler), address_(-1) { }
  ~KijiLabel() {
    assert(address_ != -1 && "Unsolved label");
  }
  ssize_t address() const {
    return address_;
  }
  void put();
  void reserve(ssize_t address) {
    reserved_addresses_.push_back(address);
  }
  bool is_solved() const { return address_!=-1; }
};
