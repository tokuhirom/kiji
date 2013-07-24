use t::ParserTest;
__END__

===
--- code
enum E <>;
--- expected
(statements (enum (ident "E") (list)))

===
--- code
enum < ook! ook. ook? >;
--- expected
(statements (enum (nop) (list (string "ook!") (string "ook.") (string "ook?"))))
