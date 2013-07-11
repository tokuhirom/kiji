#pragma once
// vim:ts=2:sw=2:tw=0:

#include <stdio.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string>
#include <sstream>
#include "pvip.h"

namespace kiji {
  /*
  class Node {
  private:
    PVIPNode* node_;
  public:
    Node() {
      node_ = NULL;
    }
    Node(PVIPNode*node) {
      node_ = node;
    }
    PVIP_category_t node_type() const {
      if (node_->type) {
        return PVIP_node_category(node_->type);
      } else {
        return PVIP_CATEGORY_UNKNOWN;
      }
    }
    ~Node() { }

    int64_t iv() const {
      // assert(node_type() == NODE_TYPE_INT);
      return this->node_->iv;
    } 
    double nv() const {
      // assert(node_type() == NODE_TYPE_NUM);
      return this->node_->nv;
    } 
    const std::vector<kiji::Node> & children() const {
      // assert(node_type() == NODE_TYPE_CHILDREN);
      auto p = new std::vector<kiji::Node>;
      for (int i=0; i<this->node_->children.size; i++) {
        p->emplace_back(this->node_->children.nodes[i]);
      }
      // FIXME MEMORY LEAK!
      return *p;
    }
    const kiji::Node & child_at(int n) const {
      // assert(node_type() == NODE_TYPE_CHILDREN);
      return this->children().at(n);
    }
    PVIP_node_type_t type() const { return node_->type; }
    const std::string pv() const {
      return std::string(node_->pv->buf, node_->pv->len);
    }
    const char* type_name() const {
      return PVIP_node_name(node_->type);
    }
  };
  */
};

