#!/usr/bin/perl

# This perl scrip is to be used out of eclipse
# It invokes the make and processes the output of errors and warnings of the
# KEIL v7.1 compiler so that these can be parsed successfully by 
# the standard error parser(s)

# use POSIX qw(locale_h);
# query and save the old locale
# my $old_locale = setlocale(LC_CTYPE);

$target = "";
my $line = "";
my $newLine = "";

if(@ARGV > 0) {
  $target = join(" ",@ARGV);
}

print "make $target\n";

open(MAKEFN, "make $target 2>&1 |" ) or die "Can\'t start make...\n";
while(<MAKEFN>) {
  chomp;
  $line = $_; 
  $line =~ s/\t/        /g;
  if($line =~ m/\*\*\* ERROR (\w+) IN LINE (\d+) OF ([\w\/-_\.]+:) (.*)/) {
  	$newLine = "$3$2: Error: $1 $4";
    print $newLine;
    next;
  }
# Orig.:
# C51 COMPILER V7.01 - SN: K1D4P-01775E
# COPYRIGHT KEIL ELEKTRONIK GmbH 1987 - 2002
# *** WARNING C280 IN LINE 63 OF ./SRC/AS3940_SPI.C: 'unused': unreferenced local variable
# *** WARNING C280 IN LINE 64 OF ./SRC/AS3940_SPI.C: 'unused2': unreferenced local variable
# Modified:
# ./SRC/AS3940_SPI.C: 'unused':63: Warning: 63 unreferenced local variable
# ./SRC/AS3940_SPI.C: 'unused2':64: Warning: 64 unreferenced local variable
  
  if($line =~ m/\*\*\* WARNING (\w+) IN LINE (\d+) OF ([\w\/-_\.]+:) (.*)/) {
  	$newLine = "$3$2: Warning: $1 $4";
    print $newLine;
    next;
  }
  print $_;
  print "\n";

}
close(MAKEFN);
$exitvalue = ($?>255) ? (255) : ($?);
exit $exitvalue;
