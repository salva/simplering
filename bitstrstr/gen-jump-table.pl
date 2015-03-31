#!/usr/bin/perl

use strict;
use warnings;

use Text::Wrap qw(wrap);

my $f = 1.04;

my @jumps;

my $current = 0;
for my $i (0..255) {
    push @jumps, $current;
    #printf "%d: %d (%f)\n", $i, $current, log($current||1)/log(10);
    my $next = int ($current * $f);
    $next++ if $next == $current;
    $current = $next;
}

print("static uint32_t\n",
      wrap('bm_jump[] = [ ',
           '              ',
           join(', ', @jumps))." ];\n");

