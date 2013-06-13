package t::Util;
use strict;
use warnings;
use utf8;
use parent qw(Exporter);

use File::Temp;
use Test::More;
use Data::Dumper;
use JSON::PP;

our @EXPORT = qw(int_ number test stmts ident);

sub _children_op {
    my ($name) = shift;
    no strict 'refs';
    push @EXPORT, $name;
    *{"$name"} = sub {
        $name =~ s/_$//;
        +{
            type => 'NQPC_NODE_' . uc($name),
            value => [@_],
        }
    };
}

_children_op('div');
_children_op('mul');
_children_op('add');
_children_op('sub_');
_children_op('funcall');

sub ident {
    +{
        type => 'NQPC_NODE_IDENT',
        value => $_[0],
    }
}

sub stmts(@) {
    +{
        type => 'NQPC_NODE_STATEMENTS',
        value => [@_],
    }
}

sub number($) {
    +{
        type => 'NQPC_NODE_NUMBER',
        value => $_[0],
    }
}
sub int_($) {
    +{
        type => 'NQPC_NODE_INT',
        value => $_[0],
    }
}

sub test {
    my ($src, $expected) = @_;

    my $tmp = File::Temp->new();
    print {$tmp} $src;

    my $json = `./nqp-parser < $tmp`;
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
        stmts($expected),
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

