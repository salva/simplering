#!/usr/bin/perl

use strict;
use warnings;

use Text::Wrap qw(wrap);

my $f = 1.04;

my @jumps = 0..64;

my $current = 9;
for my $i (65 .. 255) {
    push @jumps, $current * 8;
    my $next = int ($current * $f);
    $next++ if $next == $current;
    $current = $next;
}

print("static uint32_t\n",
      wrap('delta[256] = { ',
           '               ',
           join(', ', @jumps))." };\n");

