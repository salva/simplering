use strict;
use warnings;

use Sort::Packed qw(sort_packed);

my $keysize = 4;

print STDERR "reading data\n";
my $data = do { undef $/; <> };
print STDERR "data read\n";
my $rem = length($data) % (8 * $keysize);
substr $data, length($data) - $rem, $rem, '';
print STDERR "data size after trimming: " , length $data, "\n";
sort_packed "j$keysize" => $data;
print STDERR "data size after sorting: " , length $data, "\n";
print $data;
print STDERR "data written\n";

