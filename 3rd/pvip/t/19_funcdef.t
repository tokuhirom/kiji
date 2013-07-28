use t::ParserTest;
__END__

===
--- code
sub { }
--- expected
(statements (func (nop) (params) (nop) (statements)))

===
--- code
sub ok_auto { }
--- expected
(statements (func (ident "ok_auto") (params) (nop) (statements)))

===
--- code
sub is-true() { True }
--- expected
(statements (func (ident "is-true") (params) (nop) (statements (ident "True"))))

===
--- code
sub foo() is exportable { }
--- expected
(statements (func (ident "foo") (params) (exportable) (statements)))
