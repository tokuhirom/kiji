use t::ParserTest;

__END__

===
--- code
:10<0>
--- expected
(statements (int 0))

===
--- code
:10<1>
--- expected
(statements (int 1))

===
--- code
:16<deadbeef>
--- expected
(statements (int 3735928559))

===
--- code
:lang<perl5>
--- expected
(statements (lang "perl5"))

===
--- code
q/a'/
--- expected
(statements (string "a'"))

===
--- code
q{a'}
--- expected
(statements (string "a'"))

===
--- code
q|a'|
--- expected
(statements (string "a'"))

===
--- code
$!
--- expected
(statements (variable "$!"))

===
--- code
1..*
--- expected
(statements (range (int 1) (infinity)))

