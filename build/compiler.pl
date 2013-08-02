#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use 5.010000;
use autodie;
use FindBin;

use lib "$FindBin::Bin/local/lib/perl5/";
use Text::MicroTemplate qw(render_mt);

my $TMPL_HEADER = <<'...';
#ifndef KIJI_GEN_ND_H_
#define KIJI_GEN_ND_H_

#ifdef __cplusplus
extern "C" {
#endif
int Kiji_compiler_compile_nodes(KijiCompiler *, const PVIPNode *node);

? for my $name (@{$_[0]}) {
int Kiji_compiler_nd_<?= $name ?>(KijiCompiler *self, const PVIPNode *node);
? }
#ifdef __cplusplus
}
#endif

#endif  /* KIJI_GEN_ND_H_ */
...

my $TMPL_C = <<'CCC';
#include "moarvm.h"
#include "pvip.h"
#include "../compiler.h"

int Kiji_compiler_compile_nodes(KijiCompiler *self, const PVIPNode *node) {
  switch (node->type) {
? for my $name (@{$_[0]}) {
  case PVIP_<?= $name ?>:
    return Kiji_compiler_nd_<?= $name ?>(self, node);
? }
  }
  MVM_panic(MVM_exitcode_compunit, "AH... '%s' is not implemented yet.", PVIP_node_name(node->type));
}
CCC

&main; exit;

sub main {
  my @funcs = funcs();
  {
    my $out = render_mt($TMPL_HEADER, \@funcs);
    spew('src/compiler/gen.nd.h', $out);
  }
  {
    my $out = render_mt($TMPL_C, \@funcs);
    spew('src/compiler/gen.nd.c', $out);
  }
}

sub funcs {
  my @funcs;
  for my $fname (glob("src/compiler/nd_*.c")) {
    open my $fh, '<', $fname
      or die "Cannot open $fname for reading: $!";;
    while (<$fh>) {
      if (/\AND\(([^)]+)\)/) {
        push @funcs, $1;
      }
    }
  }
  return @funcs;
}

sub spew {
  my $fname = shift;
  open my $fh, '>', $fname
    or Carp::croak("Can't open '$fname' for writing: '$!'");
  print {$fh} $_[0];
}

