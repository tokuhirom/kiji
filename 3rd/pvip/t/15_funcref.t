use t::ParserTest;

__END__

===
--- code
&a
--- expected
(statements (funcref (ident "a")))

===
--- code
my &a = sub { }
--- expected
(statements (bind (my (funcref (ident "a"))) (func (nop) (params) (statements))))
