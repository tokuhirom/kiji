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
