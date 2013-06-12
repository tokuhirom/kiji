#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use 5.010000;

my @types = qw(
    NQPC_NODE_INT
    NQPC_NODE_NUMBER
    NQPC_NODE_STRING
    NQPC_NODE_FUNCALL
);

say qq!#pragma once!;
say qq!!;
say qq!typedef enum {!;
say qq!    $_,! for @types;
say qq!} NQPC_NODE_TYPE;!;
say qq!!;

say qq!static const char* nqpc_node_type2name(NQPC_NODE_TYPE t) {!;
say qq!    switch (t) {!;
say qq!        case $_: return "$_";! for @types;
say qq!    }!;
say qq!    return "UNKNOWN";!;
say qq!}!;
