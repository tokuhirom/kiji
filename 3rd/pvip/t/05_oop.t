use t::ParserTest;

__END__

===
--- code
class { }
--- expected
(statements (class (nop) (statements)))

===
--- code
class Foo { }
--- expected
(statements (class (ident "Foo") (statements)))

===
--- code
class NotComplex is Cool { }
--- expected
(statements (class (ident "NotComplex") (statements)))

===
--- code
3.WHAT.gist
--- expected
(statements (methodcall (methodcall (int 3) (ident "WHAT")) (ident "gist")))

===
--- code
multi method foo() { }
--- expected
(statements (multi (method (ident "foo") (args) (statements))))

===
--- code
@foo.push: 3
--- expected
(statements (methodcall (variable "@foo") (ident "push") (args (int 3))))

===
--- code
$foo.^methods
--- expected
(statements (meta_method_call (variable "$foo") (ident "methods") (nop)))

===
--- code
$foo.^methods(3)
--- expected
(statements (meta_method_call (variable "$foo") (ident "methods") (args (int 3))))
