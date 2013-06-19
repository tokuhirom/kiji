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
    Node() : type_(NODE_UNDEF) { }
    Node(const saru::Node &node) {
      this->type_ = node.type_;
      switch (type_) {
      case NODE_VARIABLE:
      case NODE_IDENT:
      case NODE_STRING:
        this->body_.pv = new std::string(*(node.body_.pv));
        break;
      case NODE_INT:
        this->body_.pv = NULL;
        this->body_.iv = node.body_.iv;
        break;
      case NODE_NUMBER:
        this->body_.pv = NULL;
        this->body_.nv = node.body_.nv;
        break;
      case NODE_WHILE:
      case NODE_ELSE:
      case NODE_RETURN:
      case NODE_FUNC:
      case NODE_PARAMS:
      case NODE_METHODCALL:
      case NODE_ATPOS:
      case NODE_ARRAY:
      case NODE_EQ:
      case NODE_NE:
      case NODE_LT:
      case NODE_LE:
      case NODE_GT:
      case NODE_GE:
      case NODE_IF:
      case NODE_STRING_CONCAT:
      case NODE_BIND:
      case NODE_MY:
      case NODE_ARGS:
      case NODE_FUNCALL:
      case NODE_STATEMENTS:
      case NODE_MUL:
      case NODE_DIV:
      case NODE_ADD:
      case NODE_SUB:
      case NODE_MOD:
        this->body_.pv = NULL;
        this->body_.children = new std::vector<saru::Node>();
        *(this->body_.children) = *(node.body_.children);
        break;
      case NODE_UNDEF:
        abort();
        break;
      }
    }
    ~Node() {
      switch (type_) {
      case NODE_VARIABLE:
      case NODE_STRING:
      case NODE_IDENT: {
        break;
      }
      case NODE_INT:
        break;
      case NODE_NUMBER:
        break;
      case NODE_WHILE:
      case NODE_ELSE:
      case NODE_RETURN:
      case NODE_FUNC:
      case NODE_PARAMS:
      case NODE_METHODCALL:
      case NODE_ATPOS:
      case NODE_ARRAY:
      case NODE_EQ:
      case NODE_NE:
      case NODE_LT:
      case NODE_LE:
      case NODE_GT:
      case NODE_GE:
      case NODE_IF:
      case NODE_STRING_CONCAT:
      case NODE_MY:
      case NODE_BIND:
      case NODE_ARGS:
      case NODE_FUNCALL:
      case NODE_STATEMENTS:
      case NODE_MUL:
      case NODE_DIV:
      case NODE_MOD:
      case NODE_ADD:
      case NODE_SUB:
        // delete this->body_.children;
        break;
      case NODE_UNDEF:
        break;
      }
    }

    void change_type(NODE_TYPE type) {
      this->type_ = type;
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
    void init_string() {
      this->type_ = NODE_STRING;
      this->body_.pv = new std::string();
    }
    void append_string(const char *txt, size_t length) {
      assert(this->type_ == NODE_STRING);
      this->body_.pv->append(txt, length);
    }

    long int iv() const {
      assert(this->type_ == NODE_INT);
      return this->body_.iv;
    } 
    double nv() const {
      assert(this->type_ == NODE_NUMBER);
      return this->body_.nv;
    } 
    const std::vector<saru::Node> & children() const {
      assert(this->type_ != NODE_INT);
      assert(this->type_ != NODE_NUMBER);
      assert(this->type_ != NODE_IDENT);
      assert(this->type_ != NODE_VARIABLE);
      return *(this->body_.children);
    }
    void push_child(saru::Node &child) {
      assert(this->type_ != NODE_INT);
      assert(this->type_ != NODE_NUMBER);
      assert(this->type_ != NODE_IDENT);
      assert(this->type_ != NODE_VARIABLE);
      this->body_.children->push_back(child);
    }
    void negate() {
      if (this->type_ == NODE_INT) {
        this->body_.iv = - this->body_.iv;
      } else {
        this->body_.nv = - this->body_.nv;
      }
    }
    NODE_TYPE type() const { return type_; }
    const std::string pv() const {
      assert(this->type_ == NODE_IDENT || this->type_ == NODE_VARIABLE || this->type_ == NODE_STRING);
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
      case NODE_INT:
        indent(depth+1);
        printf("\"value\":[%ld]\n", this->iv());
        break;
      case NODE_NUMBER:
        indent(depth+1);
        printf("\"value\":[%lf]\n", this->nv());
        break;
        // Node has a PV
      case NODE_VARIABLE:
      case NODE_STRING:
      case NODE_IDENT:
        indent(depth+1);
        printf("\"value\":[\"%s\"]\n", this->pv().c_str()); // TODO need escape
        break;
        // Node has children
      case NODE_WHILE:
      case NODE_ELSE:
      case NODE_RETURN:
      case NODE_FUNC:
      case NODE_PARAMS:
      case NODE_METHODCALL:
      case NODE_ATPOS:
      case NODE_ARRAY:
      case NODE_EQ:
      case NODE_NE:
      case NODE_LT:
      case NODE_LE:
      case NODE_GT:
      case NODE_GE:
      case NODE_IF:
      case NODE_BIND:
      case NODE_STRING_CONCAT:
      case NODE_MY:
      case NODE_ARGS:
      case NODE_FUNCALL:
      case NODE_MUL:
      case NODE_ADD:
      case NODE_SUB:
      case NODE_DIV:
      case NODE_MOD:
      case NODE_STATEMENTS: {
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
      case NODE_UNDEF:
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
  };

};

static saru::Node node_global;
static int line_number;

#define YYSTYPE saru::Node
