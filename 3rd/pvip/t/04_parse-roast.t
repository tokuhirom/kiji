use strict;
use warnings;
use utf8;
use Test::More;
use lib 't/lib';
use Data::SExpression::Lite;
use Test::Base::Less;
use Data::Dumper;
use File::Temp;
# use Test::Difflet qw(is_deeply);

for my $block (blocks) {
    subtest 'T: ' . $block->code => sub {
        my $got = parse($block->code);
        my $expected = convert_expected($block->expected);
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

sub parse {
    my ($src) = @_;

    my $tmp = File::Temp->new();
    print {$tmp} $src;

    my $json = `./pvip $tmp`;
    unless ($json =~ /\A\(/) {
        die "Cannot get json from '$src': $json";
    }
    my $got = eval {
        my $sexp = Data::SExpression::Lite->new();
        my $dat = $sexp->parse($json);
        $dat = mangle($dat);
        $dat;
    };
    if ($@) {
        diag $json;
        die $@
    }
    return $got;
}

sub convert_expected {
    my $expected = shift;
    my $sexp = Data::SExpression::Lite->new();
    $expected = $sexp->parse($expected);
    $expected = mangle($expected);
    $expected;
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

__END__

===
--- code
try { 3 }
--- expected
(statements (try (statements (int 3))))

===
--- code
1
=begin END
--- expected
(statements (int 1))

===
--- code
1
=END
--- expected
(statements (int 1))

===
--- code
1
#`[
]
--- expected
(statements (int 1))

===
--- code
my %hash={a => 1};
--- expected
(statements (bind (my (variable "%hash")) (hash (pair (string "a") (int 1)))))

===
--- code
\@a
--- expected
(statements (ref (variable "@a")))

===
--- code
@a[1] = 3;
--- expected
(statements (bind (atpos (variable "@a") (int 1)) (int 3)))

===
--- code
()
--- expected
(statements (list))

===
--- code
my @a_o=<x y z>
--- expected
(statements (bind (my (variable "@a_o")) (list (string "x") (string "y") (string "z"))))

===
--- code
@sines.map();
--- expected
(statements (methodcall (variable "@sines") (ident "map") (args)))

