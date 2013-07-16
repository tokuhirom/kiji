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
