#pragma once

#include <stdio.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string>
#include "gen.node.h"

class SARUNode {
public:
    SARUNode() : type_(SARU_NODE_UNDEF) { }
    SARUNode(const SARUNode &node) {
        this->type_ = node.type_;
        switch (type_) {
        case SARU_NODE_IDENT:
            this->body_.pv = new std::string(*(node.body_.pv));
            break;
        case SARU_NODE_INT:
            this->body_.iv = node.body_.iv;
            break;
        case SARU_NODE_NUMBER:
            this->body_.nv = node.body_.nv;
            break;
        case SARU_NODE_ARGS:
        case SARU_NODE_FUNCALL:
        case SARU_NODE_STATEMENTS:
        case SARU_NODE_MUL:
        case SARU_NODE_DIV:
        case SARU_NODE_ADD:
        case SARU_NODE_SUB:
            this->body_.children = new std::vector<SARUNode>();
            *(this->body_.children) = *(node.body_.children);
            break;
        case SARU_NODE_UNDEF:
            abort();
            break;
        }
    }
    ~SARUNode() {
        switch (type_) {
        case SARU_NODE_IDENT: {
            break;
        }
        case SARU_NODE_INT:
            break;
        case SARU_NODE_NUMBER:
            break;
        case SARU_NODE_ARGS:
        case SARU_NODE_FUNCALL:
        case SARU_NODE_STATEMENTS:
        case SARU_NODE_MUL:
        case SARU_NODE_DIV:
        case SARU_NODE_ADD:
        case SARU_NODE_SUB:
            // delete this->body_.children;
            break;
        case SARU_NODE_UNDEF:
            break;
        }
    }

    void set(SARU_NODE_TYPE type, const SARUNode &child) {
        this->type_ = type;
        this->body_.children = new std::vector<SARUNode>();
        this->body_.children->push_back(child);
    }
    void set(SARU_NODE_TYPE type, const SARUNode &c1, const SARUNode &c2) {
        this->type_ = type;
        this->body_.children = new std::vector<SARUNode>();
        this->body_.children->push_back(c1);
        this->body_.children->push_back(c2);
    }
    void set_children(SARU_NODE_TYPE type) {
        this->type_ = type;
        this->body_.children = new std::vector<SARUNode>();
    }
    void set_number(const char*txt) {
        this->type_ = SARU_NODE_NUMBER;
        this->body_.nv = atof(txt);
    }
    void set_integer(const char*txt, int base) {
        this->type_ = SARU_NODE_INT;
        this->body_.iv = strtol(txt, NULL, base);
    }
    void set_ident(const char *txt, int length) {
        this->type_ = SARU_NODE_IDENT;
        this->body_.pv = new std::string(txt, length);
    }

    long int iv() const {
        assert(this->type_ == SARU_NODE_INT);
        return this->body_.iv;
    } 
    double nv() const {
        assert(this->type_ == SARU_NODE_NUMBER);
        return this->body_.nv;
    } 
    const std::vector<SARUNode> & children() const {
        return *(this->body_.children);
    }
    void push_child(SARUNode &child) {
        assert(this->type_ != SARU_NODE_INT);
        assert(this->type_ != SARU_NODE_NUMBER);
        assert(this->type_ != SARU_NODE_IDENT);
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

private:
    SARU_NODE_TYPE type_;
    union {
        long int iv; // integer value
        double nv; // number value
        std::string *pv;
        std::vector<SARUNode> *children;
    } body_;
};

static SARUNode node_global;
static int line_number;

#define YYSTYPE SARUNode

static void indent(int n) {
    for (int i=0; i<n*4; i++) {
        printf(" ");
    }
}

static void nqpc_dump_node(const SARUNode &node, unsigned int depth) {
    printf("{\n");
    indent(depth+1);
    printf("\"type\":\"%s\",\n", nqpc_node_type2name(node.type()));
    switch (node.type()) {
    case SARU_NODE_INT:
        indent(depth+1);
        printf("\"value\":%ld\n", node.iv());
        break;
    case SARU_NODE_NUMBER:
        indent(depth+1);
        printf("\"value\":%lf\n", node.nv());
        break;
        // Node has a PV
    case SARU_NODE_IDENT:
        indent(depth+1);
        printf("\"value\":\"%s\"\n", node.pv().c_str()); // TODO need escape
        break;
        // Node has children
    case SARU_NODE_ARGS:
    case SARU_NODE_FUNCALL:
    case SARU_NODE_MUL:
    case SARU_NODE_ADD:
    case SARU_NODE_SUB:
    case SARU_NODE_DIV:
    case SARU_NODE_STATEMENTS: {
        indent(depth+1);
        printf("\"value\":[\n");
        int i=0;
        for (auto &x:node.children()) {
            indent(depth+2);
            nqpc_dump_node(
                x, depth+2
            );
            if (i==node.children().size()-1) {
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
    }
    indent(depth);
    printf("}");
    if (depth == 0) {
        printf("\n");
    }
}

static void nqpc_dump_node(const SARUNode &node) {
    nqpc_dump_node(node, 0);
}
