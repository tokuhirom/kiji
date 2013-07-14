package Data::SExpression::Lite;
use strict;
use warnings;
use utf8;
use 5.010000;

sub new {
    my $class = shift;
    my $self = bless {}, $class;
    return $self;
}

sub parse {
    my ($self, $sexp) = @_;
    $self->_parse(\$sexp);
}

sub _parse {
    my ($self, $sexp) = @_;
    $self->lex($sexp) eq '(' or die "No opening paren";
    my @tokens;
    while (length($$sexp) =~ /\S/) {
        my $token = $self->lex($sexp);
        if ($token eq ')') {
            return @tokens;
        } elsif ($token eq '(') {
            push @tokens, $self->_parse(\"($$sexp");
        } else {
            push @tokens, $token;
        }
    }
    die "Unexpected EOF in sexp";
}

sub lex {
    my ($self, $sexp) = @_;
    $$sexp =~ s/^\s+//;

    if ($$sexp =~ s/^\(//) {
        return '(';
    } elsif ($$sexp =~ s/^\)//) {
        return ')';
    } elsif ($$sexp =~ s/^"(.*?)"//) {
        return $1;
    } elsif ($$sexp =~ s/^([0-9.]+)//) {
        return $1;
    } elsif ($$sexp =~ s/^([a-zA-Z0-9_-]+)//) {
        return $1;
    } elsif ($$sexp =~ s/^([^)]+)//) {
        return $1;
    } else {
        die "Unknown token: $$sexp";
    }
}

1;
