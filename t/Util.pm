package t::Util;
use strict;
use warnings;
use utf8;
use parent qw(Exporter);

use File::Temp;
use Test::More;
use Data::Dumper;
use JSON::PP;

our @EXPORT = qw(int_ number test ident filter);

my $FILTER = sub { $_[0] };

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
);

sub _value_op {
    my ($name) = shift;
    no strict 'refs';
    push @EXPORT, $name;
    *{"$name"} = sub {
        $name =~ s/_$//;
        +{
            type => 'SARU_NODE_' . uc($name),
            value => @_,
        }
    };
}

_value_op($_) for qw(
    string
    ident
    number
    int_
);

sub filter(&) {
    $FILTER = $_[0];
}

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
        $FILTER->($expected),
        "code: $src"
    ) or do {
        $Data::Dumper::Sortkeys=1;
        diag "GOT:";
        diag Dumper($got);
        diag "EXPECTED:";
        diag Dumper(stmts($expected));
    };
}

1;

