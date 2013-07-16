use t::ParserTest;

__END__

===
--- code
?$x
--- expected
(statements (unary_boolean (variable "$x")))

===
--- code
^$n
--- expected
(statements (unary_upto (variable "$n")))

===
--- code
0b0101 +^ 0b1111
--- expected
(statements (bin_xor (int 5) (int 15)))

