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
