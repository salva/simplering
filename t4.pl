#!/usr/bin/perl

use strict;
use warnings;

my $tab = <<EOT;
------------
------------
--xx----xx--
--xx----xx--
------------
-----xx-----
-----xx-----
------------
--xx----xx--
--xx----x---
------------
------------
EOT

my $figures = <<EOF;
O
x
x
x
x
x

P
xx
xx
x

Q
xx
 x
 x
 x

R
 xx
xx
 x

S
  xx
xxx

T
xxx
 x
 x

U
x x
xxx

V
x
x
xxx

W
x
xx
 xx

X
 x
xxx
 x

Y
 x
xx
 x
 x

Z
xx
 x
 xx
EOF

my $cols = index $tab, "\n";

print "cols: $cols\n";

$figures =~ s/[A-Z]//g;

sub figure_normalize { ($_[0] =~ s/ /-/gr) =~ s/(.+)/$1 . ('-' x ($cols - length $1))/ger }

my @figures = map figure_normalize($_), ($figures =~ /(?:^ *x.*\n)+/gm);

sub figure_rotate {
    my @rot = (('') x $cols);
    for ($_[0] =~ /.+/g) {
        for my $i (0..5) {
            $rot[$i] .= substr($_, $i, 1);
        }
    }
    figure_normalize(join("\n", grep /x/, @rot). "\n");
}

sub figure_mirror_vertical { join '', reverse $_[0] =~ /.*\n/g }

sub figure_mirror_horizontal { figure_rotate figure_mirror_vertical figure_rotate $_[0] }

@figures = map { $_, figure_rotate($_) } @figures;

@figures = map { $_, figure_mirror_vertical($_) } @figures;

@figures = map { $_, figure_mirror_horizontal($_) } @figures;

s/^-*x// for @figures;

my %fig = map { $_ => 1 } @figures;

my @f;

for (keys %fig) {
    my $pos = -1;
    my @pos;
    while (($pos = index $_, 'x', $pos + 1) >= 0) {
        push @pos, $pos;
    }
    push @f, \@pos;
}

print "there are ".scalar(@f)." figures\n";

# print "[@$_]\n" for @f;

my $acu = { $tab => 1 };

sub dump;

for my $ix (0 .. length($tab) - 1) {
    my %next;
    while (my ($k, $c) = each %$acu) {
        if (substr($k, 0, 1, '') eq '-') {
        OUT: for my $f (@f) {
                if ($f->[-1] < length($k)) {
                    my $k1 = $k;
                    substr($k1, $_, 1, 'x') eq '-' or next OUT for @$f;
                    $next{$k1} += $c;
                    #print "k1>\n$k1<\n";
                }
            }
        }
        else {
            $next{$k} += $c;
        }
    }
    $acu = \%next;


    my $count = scalar keys %next;
    print "generation: $ix, count: $count\n";

    if (0 and (substr $tab, $ix, 1) eq '-') {
        # print "tab>\n$tab<\n";
        for my $k (sort keys %next) {
            print "sol[$next{$k}]>\n", (substr($tab, 0, length($tab) - length($k) - 1) =~ s/-/?/gr), '@' , $k;
        }
    }
}

my ($k, $c) = each %$acu;
print "total: $c\n";

