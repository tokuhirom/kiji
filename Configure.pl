#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use 5.008_001;
use Getopt::Long;

my $ARGV = join(' ', @ARGV);

my $p = Getopt::Long::Parser->new(
    config => [qw(posix_default no_ignore_case auto_help)]
);
$p->getoptions(
    'debug!' => \my $debug,
    'debug-asm!' => \my $debug_asm,
);
my @CXXFLAGS = qw(-g -std=c++11);
my @CFLAGS= qw(-g);
if ($debug) {
    # push @CXXFLAGS, qw(-D_GLIBCXX_DEBUG -ferror-limit=3);
    # push @CFLAGS, qw(-D_GLIBCXX_DEBUG -ferror-limit=3);
    push @CXXFLAGS, qw(-D_GLIBCXX_DEBUG );
    push @CFLAGS, qw(-D_GLIBCXX_DEBUG );
} else {
    push @CXXFLAGS, qw(-O3);
    push @CFLAGS, qw(-O3);
}
push @CXXFLAGS, qw(-DDEBUG_ASM) if $debug_asm;
push @CXXFLAGS, '-stdlib=libc++' if $^O eq 'darwin' || $ENV{TRAVIS};
my $CXXFLAGS=join(' ', @CXXFLAGS);
my $CFLAGS=join(' ', @CFLAGS);

my @LLIBS = qw(-lapr-1 -lpthread -lm);
push @LLIBS, qw(-luuid) if $^O eq 'linux';
my $LLIBS = join(' ', @LLIBS);

my $tmpl = <<'...';
# File extensions
EXE = 
O   = .o

CXXFLAGS=<<CXXFLAGS>>
CFLAGS=<<CFLAGS>>

CXX=clang++
CINCLUDE = -I3rd/MoarVM/3rdparty/apr/include -I3rd/MoarVM/3rdparty/libatomic_ops/src -I3rd/MoarVM/3rdparty/libtommath/ -I3rd/MoarVM/3rdparty/sha1/ -I3rd/MoarVM/src -I3rd/MoarVM/3rdparty -I3rd/pvip/src/
LLIBS=<<LLIBS>>
MOARVM_OBJS = \
			  3rd/MoarVM/src/6model/6model.o                      3rd/MoarVM/src/core/bytecode.o \
3rd/MoarVM/src/6model/bootstrap.o                   3rd/MoarVM/src/core/coerce.o \
3rd/MoarVM/src/6model/reprconv.o                    3rd/MoarVM/src/core/compunit.o \
3rd/MoarVM/src/6model/reprs/HashAttrStore.o         3rd/MoarVM/src/core/exceptions.o \
3rd/MoarVM/src/6model/reprs/KnowHOWAttributeREPR.o  3rd/MoarVM/src/core/frame.o \
3rd/MoarVM/src/6model/reprs/KnowHOWREPR.o           3rd/MoarVM/src/core/hll.o \
3rd/MoarVM/src/6model/reprs/Lexotic.o               3rd/MoarVM/src/core/interp.o \
3rd/MoarVM/src/6model/reprs/MVMArray.o              3rd/MoarVM/src/core/loadbytecode.o \
3rd/MoarVM/src/6model/reprs/MVMCallCapture.o        3rd/MoarVM/src/core/ops.o \
3rd/MoarVM/src/6model/reprs/MVMCFunction.o          3rd/MoarVM/src/core/threadcontext.o \
3rd/MoarVM/src/6model/reprs/MVMCode.o               3rd/MoarVM/src/core/threads.o \
3rd/MoarVM/src/6model/reprs/MVMContext.o            3rd/MoarVM/src/core/validation.o \
3rd/MoarVM/src/6model/reprs/MVMHash.o               3rd/MoarVM/src/gc/allocation.o \
3rd/MoarVM/src/6model/reprs/MVMIter.o               3rd/MoarVM/src/gc/collect.o \
3rd/MoarVM/src/6model/reprs/MVMOSHandle.o           3rd/MoarVM/src/gc/gen2.o \
3rd/MoarVM/src/6model/reprs/MVMString.o             3rd/MoarVM/src/gc/orchestrate.o \
3rd/MoarVM/src/6model/reprs/MVMThread.o             3rd/MoarVM/src/gc/roots.o \
3rd/MoarVM/src/6model/reprs/NFA.o                   3rd/MoarVM/src/gc/wb.o \
3rd/MoarVM/src/6model/reprs.o                       3rd/MoarVM/src/gc/worklist.o \
3rd/MoarVM/src/6model/reprs/P6bigint.o              3rd/MoarVM/src/io/dirops.o \
3rd/MoarVM/src/6model/reprs/P6int.o                 3rd/MoarVM/src/io/fileops.o \
3rd/MoarVM/src/6model/reprs/P6num.o                 3rd/MoarVM/src/io/procops.o \
3rd/MoarVM/src/6model/reprs/P6opaque.o              3rd/MoarVM/src/io/socketops.o \
3rd/MoarVM/src/6model/reprs/P6str.o                 3rd/MoarVM/src/moarvm.o \
3rd/MoarVM/src/6model/reprs/SCRef.o                 3rd/MoarVM/src/strings/ascii.o \
3rd/MoarVM/src/6model/reprs/Uninstantiable.o        3rd/MoarVM/src/strings/latin1.o \
3rd/MoarVM/src/6model/sc.o                          3rd/MoarVM/src/strings/ops.o \
3rd/MoarVM/src/6model/serialization.o               3rd/MoarVM/src/strings/unicode.o \
3rd/MoarVM/src/core/args.o                          3rd/MoarVM/src/strings/utf16.o \
3rd/MoarVM/src/core/bytecodedump.o                  3rd/MoarVM/src/strings/utf8.o \
3rd/MoarVM/src/6model/containers.o 3rd/MoarVM/src/math/bigintops.o

TOM = 3rd/MoarVM/3rdparty/libtommath/bn
# libtommath files
LIBTOMMATH_BIN = $(TOM)core$(O) \
        $(TOM)_error$(O) \
        $(TOM)_fast_mp_invmod$(O) \
        $(TOM)_fast_mp_montgomery_reduce$(O) \
        $(TOM)_fast_s_mp_mul_digs$(O) \
        $(TOM)_fast_s_mp_mul_high_digs$(O) \
        $(TOM)_fast_s_mp_sqr$(O) \
        $(TOM)_mp_2expt$(O) \
        $(TOM)_mp_abs$(O) \
        $(TOM)_mp_add_d$(O) \
        $(TOM)_mp_addmod$(O) \
        $(TOM)_mp_add$(O) \
        $(TOM)_mp_and$(O) \
        $(TOM)_mp_clamp$(O) \
        $(TOM)_mp_clear_multi$(O) \
        $(TOM)_mp_clear$(O) \
        $(TOM)_mp_cmp_d$(O) \
        $(TOM)_mp_cmp_mag$(O) \
        $(TOM)_mp_cmp$(O) \
        $(TOM)_mp_cnt_lsb$(O) \
        $(TOM)_mp_copy$(O) \
        $(TOM)_mp_count_bits$(O) \
        $(TOM)_mp_div_2d$(O) \
        $(TOM)_mp_div_2$(O) \
        $(TOM)_mp_div_3$(O) \
        $(TOM)_mp_div_d$(O) \
        $(TOM)_mp_div$(O) \
        $(TOM)_mp_dr_is_modulus$(O) \
        $(TOM)_mp_dr_reduce$(O) \
        $(TOM)_mp_dr_setup$(O) \
        $(TOM)_mp_exch$(O) \
        $(TOM)_mp_expt_d$(O) \
        $(TOM)_mp_exptmod_fast$(O) \
        $(TOM)_mp_exptmod$(O) \
        $(TOM)_mp_exteuclid$(O) \
        $(TOM)_mp_fread$(O) \
        $(TOM)_mp_fwrite$(O) \
        $(TOM)_mp_gcd$(O) \
        $(TOM)_mp_get_int$(O) \
        $(TOM)_mp_get_long$(O) \
        $(TOM)_mp_grow$(O) \
        $(TOM)_mp_init_copy$(O) \
        $(TOM)_mp_init_multi$(O) \
        $(TOM)_mp_init$(O) \
        $(TOM)_mp_init_set_int$(O) \
        $(TOM)_mp_init_set$(O) \
        $(TOM)_mp_init_size$(O) \
        $(TOM)_mp_invmod$(O) \
        $(TOM)_mp_invmod_slow$(O) \
        $(TOM)_mp_is_square$(O) \
        $(TOM)_mp_jacobi$(O) \
        $(TOM)_mp_karatsuba_mul$(O) \
        $(TOM)_mp_karatsuba_sqr$(O) \
        $(TOM)_mp_lcm$(O) \
        $(TOM)_mp_lshd$(O) \
        $(TOM)_mp_mod_2d$(O) \
        $(TOM)_mp_mod_d$(O) \
        $(TOM)_mp_mod$(O) \
        $(TOM)_mp_montgomery_calc_normalization$(O) \
        $(TOM)_mp_montgomery_reduce$(O) \
        $(TOM)_mp_montgomery_setup$(O) \
        $(TOM)_mp_mul_2d$(O) \
        $(TOM)_mp_mul_2$(O) \
        $(TOM)_mp_mul_d$(O) \
        $(TOM)_mp_mulmod$(O) \
        $(TOM)_mp_mul$(O) \
        $(TOM)_mp_neg$(O) \
        $(TOM)_mp_n_root$(O) \
        $(TOM)_mp_or$(O) \
        $(TOM)_mp_prime_fermat$(O) \
        $(TOM)_mp_prime_is_divisible$(O) \
        $(TOM)_mp_prime_is_prime$(O) \
        $(TOM)_mp_prime_miller_rabin$(O) \
        $(TOM)_mp_prime_next_prime$(O) \
        $(TOM)_mp_prime_rabin_miller_trials$(O) \
        $(TOM)_mp_prime_random_ex$(O) \
        $(TOM)_mp_radix_size$(O) \
        $(TOM)_mp_radix_smap$(O) \
        $(TOM)_mp_rand$(O) \
        $(TOM)_mp_read_radix$(O) \
        $(TOM)_mp_read_signed_bin$(O) \
        $(TOM)_mp_read_unsigned_bin$(O) \
        $(TOM)_mp_reduce_2k_l$(O) \
        $(TOM)_mp_reduce_2k$(O) \
        $(TOM)_mp_reduce_2k_setup_l$(O) \
        $(TOM)_mp_reduce_2k_setup$(O) \
        $(TOM)_mp_reduce_is_2k_l$(O) \
        $(TOM)_mp_reduce_is_2k$(O) \
        $(TOM)_mp_reduce$(O) \
        $(TOM)_mp_reduce_setup$(O) \
        $(TOM)_mp_rshd$(O) \
        $(TOM)_mp_set_int$(O) \
        $(TOM)_mp_set_long$(O) \
        $(TOM)_mp_set$(O) \
        $(TOM)_mp_shrink$(O) \
        $(TOM)_mp_signed_bin_size$(O) \
        $(TOM)_mp_sqrmod$(O) \
        $(TOM)_mp_sqr$(O) \
        $(TOM)_mp_sqrt$(O) \
        $(TOM)_mp_sub_d$(O) \
        $(TOM)_mp_submod$(O) \
        $(TOM)_mp_sub$(O) \
        $(TOM)_mp_toom_mul$(O) \
        $(TOM)_mp_toom_sqr$(O) \
        $(TOM)_mp_toradix_n$(O) \
        $(TOM)_mp_toradix$(O) \
        $(TOM)_mp_to_signed_bin_n$(O) \
        $(TOM)_mp_to_signed_bin$(O) \
        $(TOM)_mp_to_unsigned_bin_n$(O) \
        $(TOM)_mp_to_unsigned_bin$(O) \
        $(TOM)_mp_unsigned_bin_size$(O) \
        $(TOM)_mp_xor$(O) \
        $(TOM)_mp_zero$(O) \
        $(TOM)_prime_tab$(O) \
        $(TOM)_reverse$(O) \
        $(TOM)_s_mp_add$(O) \
        $(TOM)_s_mp_exptmod$(O) \
        $(TOM)_s_mp_mul_digs$(O) \
        $(TOM)_s_mp_mul_high_digs$(O) \
        $(TOM)_s_mp_sqr$(O) \
        $(TOM)_s_mp_sub$(O) \

KIJI_OBJS = src/kiji.o src/asm.o src/builtin/array.o src/builtin/hash.o src/builtin/int.o src/builtin/io.o src/builtin/str.o src/commander.o src/frame.o

KIJI_HEADERS = src/gen.assembler.h src/compiler.h src/builtin.h src/commander.h src/frame.h src/compiler.h src/handy.h src/asm.h

all: Makefile kiji

Makefile: Configure.pl
    perl Configure.pl <<ARGV>>
    echo "Re-run make"
    exit 1

kiji: 3rd/MoarVM/moarvm $(KIJI_OBJS) 3rd/pvip/libpvip.a
	$(CXX) $(CXXFLAGS) -Wall $(CINCLUDE) -o kiji $(KIJI_OBJS) $(MOARVM_OBJS) 3rd/MoarVM/3rdparty/apr/.libs/libapr-1.a 3rd/MoarVM/3rdparty/sha1/sha1.o $(LIBTOMMATH_BIN) $(LLIBS) 3rd/pvip/libpvip.a

src/kiji.o: $(KIJI_HEADERS) Makefile
src/frame.o: $(KIJI_HEADERS) Makefile

src/builtin/array.o: 3rd/MoarVM/moarvm $(KIJI_HEADERS) Makefile

src/builtin/asm.o: 3rd/MoarVM/moarvm $(KIJI_HEADERS) Makefile

.c.o: src/builtin.h
    $(CC) $(CINCLUDE) $(CFLAGS) -c -o $@ $<

.cc.o: src/builtin.h
	$(CXX) $(CINCLUDE) $(CXXFLAGS) -c -o $@ $<

test: kiji
	prove -r t
    cd 3rd/pvip/ && make test

3rd/greg/greg:
	cd 3rd/greg/ && $(CC) -g -o greg greg.c compile.c tree.c

3rd/MoarVM/Makefile: 3rd/MoarVM/build/Makefile.in 3rd/MoarVM/Configure.pl 3rd/MoarVM/build/Config/BuildEnvironment.pm
	cd 3rd/MoarVM/ && perl Configure.pl --clang

3rd/MoarVM/moarvm: 3rd/MoarVM/Makefile 3rd/MoarVM/src/core/*.c 3rd/MoarVM/src/core/validation.c 3rd/MoarVM/src/6model/reprs/*.c
	cd 3rd/MoarVM/ && make

3rd/pvip/libpvip.a: 3rd/greg/greg 3rd/pvip/src/*.c 3rd/pvip/src/*.h
	cd 3rd/pvip/ && make

clean:
	rm -rf kiji src/gen.* 3rd/greg/greg 3rd/greg/*.o vgcore.* core Makefile 3rd/pvip/libpvip.a
    cd 3rd/pvip/ && make clean
    cd 3rd/MoarVM/ && make clean

src/gen.assembler.h: build/asm.pl
	perl build/asm.pl

.PHONY: all clean test
...
$tmpl =~ s!<<([a-zA-Z_-]+)>>!"\$$1"!gee;
$tmpl =~ s!^    !\t!smg;

open my $fh, '>', 'Makefile';
print {$fh} $tmpl;
close $fh;

print "Generated Makefile\n";
