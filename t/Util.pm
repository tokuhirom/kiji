package t::Util;
use strict;
use warnings;
use utf8;

use Exporter::Auto;
use File::Temp;
use Test::More;
use Data::Dumper;
use JSON::PP;

sub mul($$) {
    +{
        type => 'NQPC_NODE_MUL',
        value => [@_],
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
    my $got = eval { JSON::PP->new->decode($json) };
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

