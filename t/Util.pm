package t::Util;
use strict;
use warnings;
use utf8;

use Exporter::Auto;
use File::Temp;
use Test::More;

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

    my $got = `./nqp-parser < $tmp`;
    is_deeply(
        JSON::PP->new->decode($got),
        stmts($expected),
        "code: $src"
    ) or do {
        $Data::Dumper::Sortkeys=1;
        diag $got;
        diag Dumper(stmts($expected));
    };
}

1;

