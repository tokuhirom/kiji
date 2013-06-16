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
  class Node {
  public:
    Node() : type_(SARU_NODE_UNDEF) { }
    Node(const saru::Node &node) {
      this->type_ = node.type_;
      switch (type_) {
      case SARU_NODE_VARIABLE:
      case SARU_NODE_IDENT:
      case SARU_NODE_STRING:
        this->body_.pv = new std::string(*(node.body_.pv));
        break;
      case SARU_NODE_INT:
        this->body_.iv = node.body_.iv;
        break;
      case SARU_NODE_NUMBER:
        this->body_.nv = node.body_.nv;
        break;
      case SARU_NODE_IF:
      case SARU_NODE_STRING_CONCAT:
      case SARU_NODE_BIND:
      case SARU_NODE_MY:
      case SARU_NODE_ARGS:
      case SARU_NODE_FUNCALL:
      case SARU_NODE_STATEMENTS:
      case SARU_NODE_MUL:
      case SARU_NODE_DIV:
      case SARU_NODE_ADD:
      case SARU_NODE_SUB:
      case SARU_NODE_MOD:
        this->body_.children = new std::vector<saru::Node>();
        *(this->body_.children) = *(node.body_.children);
        break;
      case SARU_NODE_UNDEF:
        abort();
        break;
      }
    }
    ~Node() {
      switch (type_) {
      case SARU_NODE_VARIABLE:
      case SARU_NODE_STRING:
      case SARU_NODE_IDENT: {
        break;
      }
      case SARU_NODE_INT:
        break;
      case SARU_NODE_NUMBER:
        break;
      case SARU_NODE_IF:
      case SARU_NODE_STRING_CONCAT:
      case SARU_NODE_MY:
      case SARU_NODE_BIND:
      case SARU_NODE_ARGS:
      case SARU_NODE_FUNCALL:
      case SARU_NODE_STATEMENTS:
      case SARU_NODE_MUL:
      case SARU_NODE_DIV:
      case SARU_NODE_MOD:
      case SARU_NODE_ADD:
      case SARU_NODE_SUB:
        // delete this->body_.children;
        break;
      case SARU_NODE_UNDEF:
        break;
      }
    }

    void set(SARU_NODE_TYPE type, const saru::Node &child) {
      this->type_ = type;
      this->body_.children = new std::vector<saru::Node>();
      this->body_.children->push_back(child);
    }
    void set(SARU_NODE_TYPE type, const saru::Node &c1, const saru::Node &c2) {
      this->type_ = type;
      this->body_.children = new std::vector<saru::Node>();
      this->body_.children->push_back(c1);
      this->body_.children->push_back(c2);
    }
    void set_children(SARU_NODE_TYPE type) {
      this->type_ = type;
      this->body_.children = new std::vector<saru::Node>();
    }
    void set_number(const char*txt) {
      this->type_ = SARU_NODE_NUMBER;
      this->body_.nv = atof(txt);
    }
    void set_integer(const char*txt, int base) {
      this->type_ = SARU_NODE_INT;
      this->body_.iv = strtoll(txt, NULL, base);
    }
    void set_ident(const char *txt, int length) {
      this->type_ = SARU_NODE_IDENT;
      this->body_.pv = new std::string(txt, length);
    }
    void set_variable(const char *txt, int length) {
      this->type_ = SARU_NODE_VARIABLE;
      this->body_.pv = new std::string(txt, length);
    }
    void init_string() {
      this->type_ = SARU_NODE_STRING;
      this->body_.pv = new std::string();
    }
    void append_string(const char *txt, size_t length) {
      assert(this->type_ == SARU_NODE_STRING);
      this->body_.pv->append(txt, length);
    }

    long int iv() const {
      assert(this->type_ == SARU_NODE_INT);
      return this->body_.iv;
    } 
    double nv() const {
      assert(this->type_ == SARU_NODE_NUMBER);
      return this->body_.nv;
    } 
    const std::vector<saru::Node> & children() const {
      return *(this->body_.children);
    }
    void push_child(saru::Node &child) {
      assert(this->type_ != SARU_NODE_INT);
      assert(this->type_ != SARU_NODE_NUMBER);
      assert(this->type_ != SARU_NODE_IDENT);
      assert(this->type_ != SARU_NODE_VARIABLE);
      this->body_.children->push_back(child);
    }
    void negate() {
      if (this->type_ == SARU_NODE_INT) {
        this->body_.iv = - this->body_.iv;
      } else {
        this->body_.nv = - this->body_.nv;
      }
    }
    SARU_NODE_TYPE type() const { return type_; }
    const std::string pv() const {
      return *(this->body_.pv);
    }
    const char* type_name() const {
      return nqpc_node_type2name(this->type());
    }

    void dump_json(unsigned int depth) const {
      printf("{\n");
      indent(depth+1);
      printf("\"type\":\"%s\",\n", nqpc_node_type2name(this->type()));
      switch (this->type()) {
      case SARU_NODE_INT:
        indent(depth+1);
        printf("\"value\":%ld\n", this->iv());
        break;
      case SARU_NODE_NUMBER:
        indent(depth+1);
        printf("\"value\":%lf\n", this->nv());
        break;
        // Node has a PV
      case SARU_NODE_VARIABLE:
      case SARU_NODE_STRING:
      case SARU_NODE_IDENT:
        indent(depth+1);
        printf("\"value\":\"%s\"\n", this->pv().c_str()); // TODO need escape
        break;
        // Node has children
      case SARU_NODE_IF:
      case SARU_NODE_BIND:
      case SARU_NODE_STRING_CONCAT:
      case SARU_NODE_MY:
      case SARU_NODE_ARGS:
      case SARU_NODE_FUNCALL:
      case SARU_NODE_MUL:
      case SARU_NODE_ADD:
      case SARU_NODE_SUB:
      case SARU_NODE_DIV:
      case SARU_NODE_MOD:
      case SARU_NODE_STATEMENTS: {
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
      case SARU_NODE_UNDEF:
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

  private:
    SARU_NODE_TYPE type_;
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
  };

};

static saru::Node node_global;
static int line_number;

#define YYSTYPE saru::Node
