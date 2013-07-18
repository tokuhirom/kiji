use t::ParserTest;
__END__

===
--- code
my ($k, $v);
--- expected
(statements (my (list (variable "$k") (variable "$v"))))

===
--- code
my ($k, $v) = (1,2);
--- expected
(statements (bind (my (list (variable "$k") (variable "$v"))) (list (int 1) (int 2))))

===
--- code
$var-name
--- expected
(statements (variable "$var-name"))

===
--- code
$var-1
--- expected
(statements (sub (variable "$var") (int 1)))

===
--- code
$$var
--- expected
(statements (scalar_deref (variable "$var")))

===
--- code
@*INC
--- expected
(statements (tw_inc))

===
--- code
$*VM
--- expected
(statements (tw_vm))

===
--- code
$?PACKAGE
--- expected
(statements (tw_package))

===
--- code
$?CLASS
--- expected
(statements (tw_class))

===
--- code
$?MODULE
--- expected
(statements (tw_module))

===
--- code
$~MAIN
--- expected
(statements (slangs "$~MAIN"))

===
--- code
$!
--- expected
(statements (special_variable_exceptions))

===
--- code
$/
--- expected
(statements (special_variable_regexp_match))
