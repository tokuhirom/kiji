#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use 5.008_001;
use lib 'build/local/lib/perl5/';
use Getopt::Long;
use File::Spec;
use Text::MicroTemplate qw(render_mt);
use File::Basename;
use Cwd;
use Data::Dumper;

&main;exit;

sub main {
    my $ARGV = join(' ', @ARGV);

    my $p = Getopt::Long::Parser->new(
        config => [qw(posix_default no_ignore_case auto_help)]
    );
    $p->getoptions(
        'debug!' => \my $debug,
        'debug-asm!' => \my $debug_asm,
    );

    my @CFLAGS= qw(-g);
    if ($debug) {
        push @CFLAGS, qw( );
    } else {
        push @CFLAGS, qw(-O3);
    }
    push @CFLAGS, qw(-DDEBUG_ASM) if $debug_asm;

    my @LLIBS = qw(-lapr-1 -lpthread -lm);
    push @LLIBS, qw(-luuid) if $^O eq 'linux';

    my @kiji_srcs = qw(
        src/kiji.c
        src/compiler/nd_ops.c src/asm.c src/builtin/array.c src/builtin/hash.c src/builtin/int.c src/builtin/io.c src/builtin/str.c src/commander.c src/frame.c src/compiler/loop.c src/compiler/label.c src/compiler/core.c src/compiler/op_helper.c src/compiler/nd_control.c
        src/builtin/pair.c
        src/compiler/nd_literal.c
        src/compiler/nd_func.c
    );
    my @kiji_objs;
    for (@kiji_srcs) {
        my $n = $_;
        $n =~ s/\.c$/.o/;
        push @kiji_objs, $n;
    }

    my $tmpl = slurp('build/Makefile.in');
    $tmpl =~ s!^    !\t!smg;
    my $src = render_mt(
        $tmpl, {
            LLIBS => join(' ', @LLIBS),
            CFLAGS => join(' ', @CFLAGS),
            ARGV => $ARGV,
            KIJI_OBJS => \@kiji_objs,
            KIJI_DEPS => [scan_sources(@kiji_srcs)],
        }
    );

    open my $fh, '>', 'Makefile';
    print {$fh} $src;
    close $fh;

    print "Generated Makefile\n";
}

sub slurp {
    my $fname = shift;
    open my $fh, '<', $fname
        or Carp::croak("Can't open '$fname' for reading: '$!'");
    scalar(do { local $/; <$fh> })
}

sub scan_sources {
    my @files = @_;
    my @ret;
    my @PATH = qw(src/ src/compiler/ 3rd/MoarVM/src/ 3rd/pvip/src/), (grep { -d $_ } <src/*>);
    for my $file (@files) {
        my $src = slurp($file);
        my @incs;
        $src =~ s/^#include "([^"]+)"/
            my $base = basename($1);
            if ($base =~ m{\Agen\.}) {
                push @incs, File::Spec->catfile("src", $base);
            } else {
                for (@PATH) {
                    my $path = File::Spec->catfile($_,$base);
                    if (-f $path) {
                        push @incs, $path;
                    }
                }
            }
        /gesm;
        (my $o = $file) =~ s/\.c$/\.o/g;
        push @ret, [$o, \@incs];
    }
    return @ret;
}

