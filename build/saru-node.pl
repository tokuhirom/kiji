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
);

say qq!/* This file is generated from $0 */!;
say qq!#pragma once!;
say qq!!;
say qq!namespace saru {!;
say qq!  typedef enum {!;
say qq!      $_,! for @types;
say qq!  } NODE_TYPE;!;

say qq!  static const char* nqpc_node_type2name(saru::NODE_TYPE t) {!;
say qq!    switch (t) {!;
say qq!      case $_: return "$_";! for @types;
say qq!    }!;
say qq!    return "UNKNOWN";!;
say qq!  }!;
say qq!}!;
