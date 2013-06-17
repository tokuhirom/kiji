#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use 5.010000;

my @types = qw(
    SARU_NODE_UNDEF
    SARU_NODE_INT
    SARU_NODE_NUMBER
    SARU_NODE_STATEMENTS
    SARU_NODE_DIV
    SARU_NODE_MUL
    SARU_NODE_ADD
    SARU_NODE_SUB
    SARU_NODE_IDENT
    SARU_NODE_FUNCALL
    SARU_NODE_ARGS
    SARU_NODE_STRING
    SARU_NODE_MOD
    SARU_NODE_VARIABLE
    SARU_NODE_MY
    SARU_NODE_BIND
    SARU_NODE_STRING_CONCAT
    SARU_NODE_IF
    SARU_NODE_EQ
    SARU_NODE_NE
    SARU_NODE_LT
    SARU_NODE_LE
    SARU_NODE_GT
    SARU_NODE_GE
    SARU_NODE_ARRAY
);

say qq!/* This file is generated from $0 */!;
say qq!#pragma once!;
say qq!!;
say qq!typedef enum {!;
say qq!    $_,! for @types;
say qq!} SARU_NODE_TYPE;!;
say qq!!;

say qq!static const char* nqpc_node_type2name(SARU_NODE_TYPE t) {!;
say qq!    switch (t) {!;
say qq!        case $_: return "$_";! for @types;
say qq!    }!;
say qq!    return "UNKNOWN";!;
say qq!}!;
