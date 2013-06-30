#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use 5.010000;

my @types = qw(
    NODE_UNDEF
    NODE_INT
    NODE_NUMBER
    NODE_STATEMENTS
    NODE_DIV
    NODE_MUL
    NODE_ADD
    NODE_SUB
    NODE_IDENT
    NODE_FUNCALL
    NODE_ARGS
    NODE_STRING
    NODE_MOD
    NODE_VARIABLE
    NODE_MY
    NODE_BIND
    NODE_STRING_CONCAT
    NODE_IF
    NODE_EQ
    NODE_NE
    NODE_LT
    NODE_LE
    NODE_GT
    NODE_GE
    NODE_ARRAY
    NODE_ATPOS
    NODE_METHODCALL
    NODE_FUNC
    NODE_PARAMS
    NODE_RETURN
    NODE_ELSE
    NODE_WHILE
    NODE_DIE
    NODE_ELSIF
    NODE_LIST
    NODE_FOR
    NODE_UNLESS
    NODE_NOT
    NODE_CONDITIONAL
    NODE_NOP
    NODE_STREQ
    NODE_STRNE
    NODE_STRGT
    NODE_STRGE
    NODE_STRLT
    NODE_STRLE
    NODE_POW
    NODE_CLARGS
    NODE_HASH
    NODE_PAIR
    NODE_ATKEY
    NODE_LOGICAL_AND
    NODE_LOGICAL_OR
    NODE_BIN_AND
    NODE_BIN_OR
    NODE_BIN_XOR
    NODE_BLOCK
    NODE_LAMBDA
    NODE_USE
    NODE_MODULE
    NODE_CLASS
    NODE_METHOD
    NODE_UNARY_PLUS
    NODE_IT_METHODCALL
    NODE_LAST
    NODE_NEXT
    NODE_REDO
    NODE_POSTINC
    NODE_POSTDEC
    NODE_PREINC
    NODE_PREDEC
    NODE_UNARY_BITWISE_NEGATION
    NODE_BRSHIFT
    NODE_BLSHIFT
);

say qq!/* This file is generated from $0 */!;
say qq!#pragma once!;
say qq!!;
say qq!namespace kiji {!;
say qq!  typedef enum {!;
say qq!      $_,! for @types;
say qq!  } NODE_TYPE;!;

say qq!  static const char* nqpc_node_type2name(kiji::NODE_TYPE t) {!;
say qq!    switch (t) {!;
say qq!      case $_: return "$_";! for @types;
say qq!    }!;
say qq!    return "UNKNOWN";!;
say qq!  }!;
say qq!}!;
