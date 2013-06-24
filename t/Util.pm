package t::Util;
use strict;
use warnings;
use utf8;
use parent qw(Exporter);

use File::Temp;
use Test::More;
use POSIX;
use Encode;

our @EXPORT = qw(run_test_cases);

sub run_test_cases {
    my @blocks = @_;
    for my $block (@blocks) {
        subtest $block->code => sub {
            my $tmp = File::Temp->new();
            my $code = $block->code;
            $code = encode_utf8($code) if utf8::is_utf8($code);
            print {$tmp} $code;

            my $ret = `./kiji < $tmp`;
            ok(POSIX::WIFEXITED($?));
            is(POSIX::WEXITSTATUS($?), 0, 'exit status should be 0');

            $ret =~ s/\n+\z//;
            (my $expected = $block->expected) =~ s/\n+\z//;
            is($ret, $expected);
        };
    }
}

{
    # utf8 hack.
    binmode Test::More->builder->$_, ":utf8" for qw/output failure_output todo_output/;
    no warnings 'redefine';
    my $code = \&Test::Builder::child;
    *Test::Builder::child = sub {
        my $builder = $code->(@_);
        binmode $builder->output,         ":utf8";
        binmode $builder->failure_output, ":utf8";
        binmode $builder->todo_output,    ":utf8";
        return $builder;
    };
}

1;

