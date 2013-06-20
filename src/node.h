#pragma once
// vim:ts=2:sw=2:tw=0:

#include <stdio.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string>
#include "gen.node.h"

namespace saru {
  enum node_type_t {
    NODE_TYPE_UNDEF,
    NODE_TYPE_INT,
    NODE_TYPE_NUM,
    NODE_TYPE_STR,
    NODE_TYPE_CHILDREN
  };
  class Node {
  private:
    NODE_TYPE type_;
    union {
        int64_t iv; // integer value
        double nv; // number value
        std::string *pv;
        std::vector<saru::Node> *children;
    } body_;
    static void indent(int n) {
      for (int i=0; i<n*4; i++) {
          printf(" ");
      }
    }
  public:
    Node() : type_(NODE_UNDEF) { }
    Node(const saru::Node &node) {
      this->type_ = node.type_;
      switch (this->node_type()) {
      case NODE_TYPE_STR:
        this->body_.pv = new std::string(*(node.body_.pv));
        break;
      case NODE_TYPE_INT:
        this->body_.pv = NULL;
        this->body_.iv = node.body_.iv;
        break;
      case NODE_TYPE_NUM:
        this->body_.pv = NULL;
        this->body_.nv = node.body_.nv;
        break;
      case NODE_TYPE_CHILDREN:
        this->body_.pv = NULL;
        this->body_.children = new std::vector<saru::Node>();
        *(this->body_.children) = *(node.body_.children);
        break;
      default:
        abort();
      }
    }
    node_type_t node_type() const {
      switch (type_) {
      case NODE_VARIABLE:
      case NODE_IDENT:
      case NODE_STRING:
        return NODE_TYPE_STR;
      case NODE_INT:
        return NODE_TYPE_INT;
      case NODE_NUMBER:
        return NODE_TYPE_NUM;
      case NODE_UNDEF:
        return NODE_TYPE_UNDEF;
      default:
        return NODE_TYPE_CHILDREN;
      }
    }
    ~Node() { }

    void change_type(NODE_TYPE type) {
      this->type_ = type;
    }

    void set_clargs() {
      this->type_ = NODE_CLARGS;
      this->body_.children = new std::vector<saru::Node>();
    }
    void set_nop() {
      this->type_ = NODE_NOP;
      this->body_.children = new std::vector<saru::Node>();
    }
    void set(NODE_TYPE type, const saru::Node &child) {
      this->type_ = type;
      this->body_.children = new std::vector<saru::Node>();
      this->body_.children->push_back(child);
    }
    void set(NODE_TYPE type, const saru::Node &c1, const saru::Node &c2) {
      this->type_ = type;
      this->body_.children = new std::vector<saru::Node>();
      this->body_.children->push_back(c1);
      this->body_.children->push_back(c2);
    }
    void set(NODE_TYPE type, const saru::Node &c1, const saru::Node &c2, const saru::Node &c3) {
      this->type_ = type;
      this->body_.children = new std::vector<saru::Node>();
      this->body_.children->push_back(c1);
      this->body_.children->push_back(c2);
      this->body_.children->push_back(c3);
    }
    void set_children(NODE_TYPE type) {
      this->type_ = type;
      this->body_.children = new std::vector<saru::Node>();
    }
    void set_number(const char*txt) {
      this->type_ = NODE_NUMBER;
      this->body_.nv = atof(txt);
    }
    void set_integer(const char*txt, int base) {
      this->type_ = NODE_INT;
      this->body_.iv = strtoll(txt, NULL, base);
    }
    void set_ident(const char *txt, int length) {
      this->type_ = NODE_IDENT;
      this->body_.pv = new std::string(txt, length);
    }
    void set_variable(const char *txt, int length) {
      this->type_ = NODE_VARIABLE;
      this->body_.pv = new std::string(txt, length);
    }
    void set_string(const char *txt, int length) {
      this->type_ = NODE_STRING;
      this->body_.pv = new std::string(txt, length);
    }
    void init_string() {
      this->type_ = NODE_STRING;
      this->body_.pv = new std::string();
    }
    void append_string(const char *txt, size_t length) {
      assert(this->type_ == NODE_STRING);
      this->body_.pv->append(txt, length);
    }

    long int iv() const {
      assert(node_type() == NODE_TYPE_INT);
      return this->body_.iv;
    } 
    double nv() const {
      assert(node_type() == NODE_TYPE_NUM);
      return this->body_.nv;
    } 
    const std::vector<saru::Node> & children() const {
      assert(node_type() == NODE_TYPE_CHILDREN);
      return *(this->body_.children);
    }
    void push_child(saru::Node &child) {
      assert(node_type() == NODE_TYPE_CHILDREN);
      this->body_.children->push_back(child);
    }
    void negate() {
      assert(node_type() != NODE_TYPE_CHILDREN);
      if (this->type_ == NODE_INT) {
        this->body_.iv = - this->body_.iv;
      } else {
        this->body_.nv = - this->body_.nv;
      }
    }
    NODE_TYPE type() const { return type_; }
    const std::string pv() const {
      assert(node_type() == NODE_TYPE_STR);
      return *(this->body_.pv);
    }
    const char* type_name() const {
      return nqpc_node_type2name(this->type());
    }

    void dump_json(unsigned int depth) const {
      printf("{\n");
      indent(depth+1);
      printf("\"type\":\"%s\",\n", nqpc_node_type2name(this->type()));
      switch (this->node_type()) {
      case NODE_TYPE_INT:
        indent(depth+1);
        printf("\"value\":[%ld]\n", this->iv());
        break;
      case NODE_TYPE_NUM:
        indent(depth+1);
        printf("\"value\":[%lf]\n", this->nv());
        break;
      case NODE_TYPE_STR:
        indent(depth+1);
        printf("\"value\":[\"%s\"]\n", this->pv().c_str()); // TODO need escape
        break;
      case NODE_TYPE_CHILDREN: {
        indent(depth+1);
        printf("\"value\":[\n");
        int i=0;
        for (auto &x: this->children()) {
          indent(depth+2);
          x.dump_json(depth+2);
          if (i==this->children().size()-1) {
            printf("\n");
          } else {
            printf(",\n");
          }
          i++;
        }
        indent(depth+1);
        printf("]\n");
        break;
      }
      case NODE_TYPE_UNDEF:
        break;
      default:
        abort();
      }
      indent(depth);
      printf("}");
      if (depth == 0) {
        printf("\n");
      }
    }

    void dump_json() const {
      this->dump_json(0);
    }
  };

};

static saru::Node node_global;
static int line_number;

#define YYSTYPE saru::Node
