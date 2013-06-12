use strict;
use warnings;
use utf8;
use Test::More;
use JSON::PP;
use File::Temp;

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

test( '33', int_(33));
test( '.33', number(0.33));
test( '0.33', number(0.33));
test( '3.14', number(3.14));
test( '0b1000', int_(8));
test( '0b0100', int_(4));
test( '0b0010', int_(2));
test( '0b0001', int_(1));
test( '0xdeadbeef', int_(0xdeadbeef));
test( '0o755', int_(0755));

done_testing;

sub test {
    my ($src, $expected) = @_;

    my $tmp = File::Temp->new();
    print {$tmp} $src;

    my $got = `./nqp-parser < $tmp`;
    note $got;
    is_deeply(
        JSON::PP->new->decode($got),
        $expected
    );
}
