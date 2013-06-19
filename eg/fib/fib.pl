use 5.018000;

sub fib {
    my $n = shift;
    return $n if ( $n < 2 );
    return fib( $n - 1 ) + fib( $n - 2 );
}

say fib(25);
