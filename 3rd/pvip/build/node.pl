#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use 5.010000;
use autodie;

my @types = qw(
    PVIP_NODE_UNDEF
    PVIP_NODE_INT
    PVIP_NODE_NUMBER
    PVIP_NODE_STATEMENTS
    PVIP_NODE_DIV
    PVIP_NODE_MUL
    PVIP_NODE_ADD
    PVIP_NODE_SUB
    PVIP_NODE_IDENT
    PVIP_NODE_FUNCALL
    PVIP_NODE_ARGS
    PVIP_NODE_STRING
    PVIP_NODE_MOD
    PVIP_NODE_VARIABLE
    PVIP_NODE_MY
    PVIP_NODE_OUR
    PVIP_NODE_BIND
    PVIP_NODE_STRING_CONCAT
    PVIP_NODE_IF
    PVIP_NODE_EQV
    PVIP_NODE_EQ
    PVIP_NODE_NE
    PVIP_NODE_LT
    PVIP_NODE_LE
    PVIP_NODE_GT
    PVIP_NODE_GE
    PVIP_NODE_ARRAY
    PVIP_NODE_ATPOS
    PVIP_NODE_METHODCALL
    PVIP_NODE_FUNC
    PVIP_NODE_PARAMS
    PVIP_NODE_RETURN
    PVIP_NODE_ELSE
    PVIP_NODE_WHILE
    PVIP_NODE_DIE
    PVIP_NODE_ELSIF
    PVIP_NODE_LIST
    PVIP_NODE_FOR
    PVIP_NODE_UNLESS
    PVIP_NODE_NOT
    PVIP_NODE_CONDITIONAL
    PVIP_NODE_NOP
    PVIP_NODE_STREQ
    PVIP_NODE_STRNE
    PVIP_NODE_STRGT
    PVIP_NODE_STRGE
    PVIP_NODE_STRLT
    PVIP_NODE_STRLE
    PVIP_NODE_POW
    PVIP_NODE_CLARGS
    PVIP_NODE_HASH
    PVIP_NODE_PAIR
    PVIP_NODE_ATKEY
    PVIP_NODE_LOGICAL_AND
    PVIP_NODE_LOGICAL_OR
    PVIP_NODE_LOGICAL_XOR
    PVIP_NODE_BIN_AND
    PVIP_NODE_BIN_OR
    PVIP_NODE_BIN_XOR
    PVIP_NODE_BLOCK
    PVIP_NODE_LAMBDA
    PVIP_NODE_USE
    PVIP_NODE_MODULE
    PVIP_NODE_CLASS
    PVIP_NODE_METHOD
    PVIP_NODE_UNARY_PLUS
    PVIP_NODE_UNARY_MINUS
    PVIP_NODE_IT_METHODCALL
    PVIP_NODE_LAST
    PVIP_NODE_NEXT
    PVIP_NODE_REDO
    PVIP_NODE_POSTINC
    PVIP_NODE_POSTDEC
    PVIP_NODE_PREINC
    PVIP_NODE_PREDEC
    PVIP_NODE_UNARY_BITWISE_NEGATION
    PVIP_NODE_BRSHIFT
    PVIP_NODE_BLSHIFT
    PVIP_NODE_ABS
    PVIP_NODE_CHAIN
    PVIP_NODE_INPLACE_ADD
    PVIP_NODE_INPLACE_SUB
    PVIP_NODE_INPLACE_MUL
    PVIP_NODE_INPLACE_DIV
    PVIP_NODE_INPLACE_POW
    PVIP_NODE_INPLACE_MOD
    PVIP_NODE_INPLACE_BIN_OR
    PVIP_NODE_INPLACE_BIN_AND
    PVIP_NODE_INPLACE_BIN_XOR
    PVIP_NODE_INPLACE_BLSHIFT
    PVIP_NODE_INPLACE_BRSHIFT
    PVIP_NODE_INPLACE_CONCAT_S
    PVIP_NODE_REPEAT_S
    PVIP_NODE_INPLACE_REPEAT_S
    PVIP_NODE_UNARY_TILDE
);

{
    open my $fh, '>', 'src/gen.node.h';
    say $fh qq!/* This file is generated from $0 */!;
    say $fh qq!#pragma once!;
    say $fh qq!!;
    say $fh qq!typedef enum {!;
    say $fh qq!    $_,! for @types;
    say $fh qq!} PVIP_node_type_t;!;
    say $fh qq!const char* PVIP_node_name(PVIP_node_type_t t);!;
}

{
    open my $fh, '>', 'src/gen.node.c';
    say $fh qq!/* This file is generated from $0 */!;
    say $fh qq!#include "gen.node.h"!;
    say $fh qq!const char* PVIP_node_name(PVIP_node_type_t t) {!;
    say $fh qq!  switch (t) {!;
    for (@types) {
        my $k = "$_";
        my $v = "$_";
        $v =~ s/^PVIP_NODE_//;
        printf $fh qq!    case %s: return "%s";\n!, $k, lc($v);
    }
    say $fh qq!  }!;
    say $fh qq!  return "UNKNOWN";!;
    say $fh qq!}!;
}
