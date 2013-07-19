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

===
--- code
1 and 2
--- expected
(statements (logical_and (int 1) (int 2)))

===
--- code
1 andthen 2
--- expected
(statements (logical_andthen (int 1) (int 2)))

===
--- code
1===3
--- expected
(statements (chain (int 1) (value_identity (int 3))))

===
--- code
1 cmp 3
--- expected
(statements (cmp (int 1) (int 3)))
