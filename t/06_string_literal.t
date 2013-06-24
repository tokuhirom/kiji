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
    q!"ho\nge\afuga\""!,
    {
        'type'  => 'NODE_STATEMENTS',
        'value' => [
            {
                'type'  => 'NODE_STRING',
                'value' => [ "ho\nge\afuga\"" ]
            }
        ]
    }
);

test(
    q!""!,
    {
        'type'  => 'NODE_STATEMENTS',
        'value' => [
            {
                'type'  => 'NODE_STRING',
                'value' => [ "" ]
            }
        ]
    }
);

test(
    q{''},
    {
        'type'  => 'NODE_STATEMENTS',
        'value' => [
            {
                'type'  => 'NODE_STRING',
                'value' => [ '' ]
            }
        ]
    }
);

test(
    q{'hoge'},
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
    q{'ho\nge'},
    {
        'type'  => 'NODE_STATEMENTS',
        'value' => [
            {
                'type'  => 'NODE_STRING',
                'value' => [ 'ho\nge' ]
            }
        ]
    }
);

test(
    q{'ho\\n\'ge'},
    {
        'type'  => 'NODE_STATEMENTS',
        'value' => [
            {
                'type'  => 'NODE_STRING',
                'value' => [ q!ho\n'ge! ]
            }
        ]
    }
);

test(
    q{"\x6f\x6b 8"},
    {
        'type'  => 'NODE_STATEMENTS',
        'value' => [
            {
                'type'  => 'NODE_STRING',
                'value' => [ q!ok 8! ]
            }
        ]
    }
);

done_testing;

sub test {
    my ($src, $expected) = @_;

    note "-----------------------------------------------------------------------";

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

