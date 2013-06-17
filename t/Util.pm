package t::Util;
use strict;
use warnings;
use utf8;
use parent qw(Exporter);

use File::Temp;
use Test::More;
use Data::Dumper;
use JSON::PP;

our @EXPORT = qw(test);

sub _children_op {
    my ($name) = shift;
    no strict 'refs';
    push @EXPORT, $name;
    *{"$name"} = sub {
        $name =~ s/_$//;
        +{
            type => 'SARU_NODE_' . uc($name),
            value => [@_],
        }
    };
}

_children_op($_) for qw(
    div mul
    add sub_
    funcall args
    statements
    bind_
    my_
    string_concat
    if_
    eq_
    string
    ident
    number
    int_
    variable
);

sub test {
    my ($src, $expected) = @_;

    my $tmp = File::Temp->new();
    print {$tmp} $src;

    my $json = `./_build/saru-parser < $tmp`;
    unless ($json =~ /\A\{/) {
        die "Cannot get json from '$src'";
    }
    note $json;
    my $got = eval {
        JSON::PP->new->decode($json)
    };
    if ($@) {
        diag $json;
        die $@
    }
    is_deeply(
        $got,
        $expected,
        "code: $src"
    ) or do {
        $Data::Dumper::Sortkeys=1;
        diag "GOT:";
        diag Dumper($got);
        diag "EXPECTED:";
        diag Dumper($expected);
    };
}

1;

