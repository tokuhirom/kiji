use t::ParserTest;
__END__

===
--- code
sub { }
--- expected
(statements (func (nop) (params) (statements)))

===
--- code
sub ok_auto { }
--- expected
(statements (func (ident "ok_auto") (params) (statements)))

===
--- code
sub is-true() { True }
--- expected
(statements (func (ident "is-true") (params) (statements (ident "True"))))
