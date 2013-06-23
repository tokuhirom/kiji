use strict;
use warnings;
use utf8;
use Test::More;
use Data::Dumper qw(Dumper);
use File::Temp;
use JSON::PP;

test(
    '"hoge"',
    {
        'type'  => 'NODE_STATEMENTS',
        'value' => [
            {
                'type'  => 'NODE_STRING',
                'value' => [ 'hoge' ]
            }
        ]
    }
);

test(
    '"ho\nge"',
    {
        'type'  => 'NODE_STATEMENTS',
        'value' => [
            {
                'type'  => 'NODE_STRING',
                'value' => [ "ho\nge" ]
            }
        ]
    }
);

done_testing;

sub test {
    my ($src, $expected) = @_;

    my $tmp = File::Temp->new();
    print {$tmp} $src;

    my $json = `./saru --dump-ast $tmp`;
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

