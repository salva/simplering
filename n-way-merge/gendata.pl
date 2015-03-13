#!/usr/bin/perl

use strict;
use warnings;
use Sort::Packed qw(sort_packed);

my $keysize = 4;
my $n = 1024*1024*1;

$| = 1;

for my $ix (0..49) {
    print STDERR "generating data $ix\n";
    my $out = '';
    $out .= pack "j*", map int(rand 2**30), 1..$keysize for 1..$n;
    print STDERR "sorting data\n";
    sort_packed "j$keysize" => $out;
    print STDERR "saving\n";
    open my $fh, ">data/chunk-$ix.dat";
    print {$fh} $out;
    print STDERR "done $ix!\n";
}
