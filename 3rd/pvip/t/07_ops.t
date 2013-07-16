use t::ParserTest;

__END__

===
--- code
?$x
--- expected
(statements (unary_boolean (variable "$x")))
