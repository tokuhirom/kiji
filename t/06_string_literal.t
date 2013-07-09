use strict;
use warnings;
use utf8;
use Test::More;
use Data::Dumper qw(Dumper);
use File::Temp;
use JSON::PP;
use Data::SExpression 0.14;

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

    my $json = `./kiji --dump-ast $tmp`;
    unless ($json =~ /\A\(/) {
        die "Cannot get sexp from '$src'";
    }
    note $json;
    my $got = eval {
        my $sexp = Data::SExpression->new({use_symbol_class => 1});
        my $dat = $sexp->read($json);
        $dat = mangle($dat);
        $dat;
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

sub mangle {
    my $data = shift;
    if (ref $data eq 'ARRAY') {
        if (@$data == 0) {
            return ();
        }
        my $header = shift @$data;
        my $value = [map { mangle($_) } @$data];
        return +{
            type  => 'NODE_'.uc($header->name),
            value => $value,
        };
    } else {
        $data =~ s/\A([0-9]+\.[0-9]+?)0*\z/$1/; # TODO remove this
        return $data;
    }
}
