package t::Util;
use strict;
use warnings;
use utf8;
use Test::More;
use Data::Section::TestBase;
use lib 't/lib';
use Data::SExpression::Lite;
use Data::Section::TestBase;
use Data::Dumper;
use File::Temp;

END {
    for my $block (Data::Section::TestBase->new(package => scalar(caller(1)))->blocks) {
        subtest 'T: ' . $block->code => sub {
            my $got = parse_perl6($block->code);
            my $expected = parse_sexp($block->expected);
            is_deeply($got, $expected, 'Test: ' . $block->code) or do {
                $Data::Dumper::Sortkeys=1;
                $Data::Dumper::Indent=1;
                $Data::Dumper::Terse=1;
                diag "GOT:";
                diag Dumper($got);
                diag "EXPECTED:";
                diag Dumper($expected);
            };
        };
    }

    done_testing;
}

sub parse_perl6 {
    my ($src) = @_;

    my $tmp = File::Temp->new();
    print {$tmp} $src;

    my $sexp = `./pvip $tmp`;
    unless ($sexp =~ /\A\(/) {
        die "Cannot get sexp from '$src': $sexp";
    }
    my $got = eval {
        parse_sexp($sexp);
    };
    if ($@) {
        diag $sexp;
        die $@
    }
    return $got;
}

sub parse_sexp {
    my $expected = shift;
    my $sexp = Data::SExpression::Lite->new();
    $expected = [$sexp->parse($expected)];
    $expected;
}

1;

