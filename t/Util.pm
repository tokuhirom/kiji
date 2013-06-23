package t::Util;
use strict;
use warnings;
use utf8;
use parent qw(Exporter);

use File::Temp;
use Test::More;
use POSIX;

our @EXPORT = qw(run_test_cases);

sub run_test_cases {
    my @blocks = @_;
    for my $block (@blocks) {
        subtest $block->code => sub {
            my $tmp = File::Temp->new();
            print {$tmp} $block->code;

            my $ret = `./saru < $tmp`;
            ok(POSIX::WIFEXITED($?));
            is(POSIX::WEXITSTATUS($?), 0, 'exit status should be 0');

            $ret =~ s/\n+\z//;
            (my $expected = $block->expected) =~ s/\n+\z//;
            is($ret, $expected);
        };
    }
}

1;

