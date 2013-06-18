use strict;
use warnings;
use utf8;
use Test::More;
use Data::SExpression 0.41;
use Test::Base::Less;
use Data::Dumper;
use File::Temp;
use JSON::PP;

for my $block (blocks) {
    subtest 'T: ' . $block->code => sub {
        my $got = parse($block->code);
        my $expected = convert_expected($block->expected);
        is_deeply($got, $expected, 'Test: ' . $block->code) or do {
            $Data::Dumper::Sortkeys=1;
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

    my $json = `./_build/saru-parser < $tmp`;
    unless ($json =~ /\A\{/) {
        die "Cannot get json from '$src'";
    }
    my $got = eval {
        JSON::PP->new->decode($json)
    };
    if ($@) {
        diag $json;
        die $@
    }
    return $got;
}

sub convert_expected {
    my $expected = shift;
    my $sexp = Data::SExpression->new({use_symbol_class => 1});
    $expected = $sexp->read($expected);
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
        return $data;
    }
}

__END__

===
--- code: 33
--- expected
(statements (int 33))

===
--- code: .33
--- expected
(statements (number 0.33))

===
--- code: 0.33
--- expected
(statements (number 0.33))

===
--- code: 3.14
--- expected
(statements (number 3.14))

===
--- code: 0b1000
--- expected
(statements (int 8))

===
--- code: 0b0100
--- expected
(statements (int 4))

===
--- code: 0b0010
--- expected
(statements (int 2))

===
--- code: 0b0001
--- expected
(statements (int 1))

===
--- code: 0xdeadbeef
--- expected
(statements (int 3735928559))

===
--- code: 0o755
--- expected
(statements (int 493))

===
--- code: -5963
--- expected
(statements (int "-5963"))

===
--- code: 3*4
--- expected
(statements (mul (int 3) (int 4)))

===
--- code: 3/4
--- expected
(statements (div (int 3) (int 4)))

===
--- code: 3+4
--- expected
(statements (add (int 3) (int 4)))

===
--- code: 3-4
--- expected
(statements (sub (int "3") (int 4)))

===
--- code: 3-4-2
--- expected
(statements (sub (sub (int "3") (int 4)) (int 2)))

===
--- code: 3+4*2
--- expected
(statements (add (int "3") (mul (int 4) (int 2))))

===
--- code: 3==4
--- expected
(statements (eq (int 3) (int 4)))

===
--- code: say()
--- expected
(statements (funcall (ident "say") (args ())))

===
--- code: say(3)
--- expected
(statements (funcall (ident "say") (args (int 3))))

===
--- code: "hoge"
--- expected
(statements (string "hoge"))

===
--- code: (3+4)*2
--- expected
(statements (mul (add (int 3) (int 4)) (int 2)))

===
--- code: $n
--- expected
(statements (variable "$n"))

===
--- code: my $n := 3
--- expected
(statements (bind (my (variable "$n")) (int 3)))

=== test( '"H" ~ "M"', string_concat(string("H"), string("M")));
--- code: "H" ~ "M"
--- expected
(statements (string_concat (string "H") (string "M")))

===
--- code: if 1 {say(4)}
--- expected
(statements
    (if (int 1)
        (statements
            (funcall (ident "say") (args (int 4))))))

===
--- code: if 1 { say(4) }
--- expected
(statements
    (if (int 1)
        (statements
            (funcall (ident "say") (args (int 4))))))

===
--- code: 1;2
--- expected
(statements
    (int 1)
    (int 2))

===
--- code: []
--- expected
(statements
    (array))

===
--- code: [1,2,3]
--- expected
(statements
    (array (int 1) (int 2) (int 3)))

===
--- code
sub foo() { 4 }
--- expected
(statements
    (func
        (ident "foo")
        (params)
        (statements (int 4))))
