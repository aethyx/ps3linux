#!/usr/bin/perl
#
#  Copyright (C) 2005 Michal Ostrowski <mostrows@watson.ibm.com>, IBM Corp.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
#  $Id$
#
#  Prepares an LPAR container directory.
#  Allows use to specify OF tree contents, kernel image, ramdisk image.
#
#
use File::stat;
use File::Basename;
use IO::File;
use IO::Handle;
use strict;
use Getopt::Long;
use Pod::Usage;

my $libdir;
BEGIN{
  use Cwd 'abs_path';
  $libdir = abs_path(dirname($0)) . '/../lib/hype';
}

use Cwd 'fast_abs_path';
use lib "$libdir/perl";
use HypeConst;
use HypeUtil;
use POSIX qw(uname);

Getopt::Long::Configure qw(no_ignore_case);

my $verbose;
my $laddr;
my $ofstub = "$libdir/of_image";
my $oftree = "$libdir/of_tree";
my $ramdisk;
my $create;
my $kernel;
my @props;
my @fileprops;
my $membottom = 0;
my $memtop = CHUNK_SIZE;
my $help = 0;
my $man = 0;
my $processor = (uname)[4];
GetOptions("i|image=s" => \$kernel,
	   "r|ramdisk=s" => \$ramdisk,
	   "h|help" => \$help,
	   "p|prop=s" => \@props,
	   "P|fileprop=s" => \@fileprops,
	   "man"	=> \$man,
	   "n|name=s" => \$pname,
	   "C|create" => \$create) or pod2usage(2);

pod2usage(1) if $help;
pod2usage(-exitstatus => 0, -verbose => 2) if $man;

if(!defined $create && !defined $pname){
  die "Must create or name a partition";
}


sub run_command($) {
  my $x = shift;
  #printf "Run command: $x\n";
  system($x);
  if ($?!=0) {
    abort("running command $x");
  }
}

my %ofprop;
#
# Check syntax of -p and -P arguments
#
sub parse_prop_args($@){
  (my $type, my @args) = @_;
  map {
    if(!/^([^=]+)=(.*)$/){
      abort "Invalid argument format: $_";
    }
    my $prop = $1;
    my $val=$2;
    $ofprop{$prop}->{val} = $val;
    $ofprop{$prop}->{type} = $type;
  } @args;
}

parse_prop_args("val",@props);
parse_prop_args("file", @fileprops);


if(!defined $pname){
  my $x = 0;
  do{
    $pname = sprintf("lpar%02x", $x++);
  } while (-d "$vroot/$pname");
}

my $state = get_file("state");
if ($state eq "CREATED" || $state eq "RUNNING"){
  print "Will not over-write RUNNING partition\n";
  exit -1;
}

if(defined $create){
  if (-d "$vroot/$pname") {
    `rm -rf $vroot/$pname`;
  }
  if(! -d "$vroot/$pname" && !mkdir "$vroot/$pname"){
    print "Can't create partition directory $vroot/pname\n";
    exit -1;
  }

  if(! -d "$vroot/$pname/of_tree") {
    run_command("cp -r $oftree $vroot/$pname/of_tree");
  }
  
  if ($processor eq "ppc64") {
    run_command("objcopy --output-target=binary $ofstub $vroot/$pname/of_image");
  }

  set_file("of_tree/ibm,partition-name", $pname);
  set_file("state", "READY");
}


#
# Look at all OF properties we need to set and set them
map {
  my $prop = $_;
  my $type = $ofprop{$prop}->{type};
  my $val = $ofprop{$prop}->{val};
  (my $base, my $path) = fileparse($prop);
  my @nodes = split /\/+/, $path;
  my $x;

  # Create any OF nodes
  while ($#nodes>=0) {
    my $y = shift @nodes;
    next if(!defined $y || $y eq "");
    $x = $x . "/" . $y;
    of_make_node($x);
  }

  # Set the prop from value or file
  if($type eq "val") {
    of_set_prop($x, $base, $val);
  } else {
    of_set_prop_from_file($x, $base, $val);
  }

} keys(%ofprop);


if(defined $kernel){
  my $image = "$vroot/$pname/image00";

  #
  # Must locate entry point function descriptor to identify
  # toc value.  We enter the image at 0 regardless of the pc value
  #
  my $is_elf = 1;
  my $entry;
  my $bintype = "UNKNOWN";
  my $base;
  open READELF, "readelf -lh $kernel 2>&1 |";
  while (<READELF>) {
    if (/Not an ELF file/) {
      $is_elf = 0;
      last;
    }
    my @words = split;
    if($words[0] eq "Class:"){
      $bintype = $words[1];
      next;
    }
    if($words[0] eq "LOAD" && !defined $base){
      $base = $words[3];
      $base =~ /0x.{0,8}?([[:xdigit:]]{1,8})$/;
      $base = "0x" . $1;
      next;
    }
    if(/Entry point address:\s+0x.{0,8}?([[:xdigit:]]{1,8})$/){
      $entry = "0x" . $1;
      next;
    }
  }
  close READELF;

  run_command("objcopy --output-target=binary $kernel $vroot/$pname/image00");


  if($bintype eq "ELF64" and oct($base) ne oct($entry)){
    my $bin_fh = new IO::File $image, O_RDWR;
    if(!defined $bin_fh) {
      abort("$! open $image");
    }
    #
    # Assume only lower-order 32-bits of the 2 64-bits values are relevant
    my @func_desc = extract_data($bin_fh, oct($entry) - oct($base), 4, 4);
    my $pc = $func_desc[1];
    my $toc= $func_desc[3];

    $bin_fh->close;
    set_file("r2", hexstr($toc));
    set_file("pc", hexstr($pc));
  } else {
    set_file("pc", $entry);
  }

  set_file("image00_load", $base);
}

if (defined $ramdisk) {
  run_command("cp $ramdisk $vroot/$pname/image01");
  set_file("image01_load", hexstr(24 * MB));
  set_file("r3", hexstr(24*MB));
  set_file("r4", hexstr(stat($ramdisk)->size));
}

print "LPAR configured: $pname\n";

exit 0;

__END__

=head1 NAME

oh_prep_lpar - Create and setup LPAR containers

=head1 SYNOPSIS

oh_prep_lpar [options]

  Options:
	-h|--help		 brief help message
	-i|--image <file>	 prepare "file" as kernel image for LPAR
	-r|--ramdisk <file>	 prepare "file" as ramdisk image for LPAR
	-C|--create		 create new LPAR
	-p|--prop=<p>=<val>	 set OF property "p" to "val"
	-P|--fileprop=<p>=<file> set OF property "p" with contents of 
				 "file"
	-n|--name		 name of LPAR

=head1 OPTIONS

=over 8

=item B<--image>

Specifies the kernel image to prepare.

=item B<--ramdisk>

Specifies the ramdisk image to prepare.

=item B<--create>

Creates a new LPAR container; uses an auto-generated name if the
"--name" option is not specified.  If the specified LPAR already
exists, it will be removed first.

=item B<--prop>    

Takes arguments of the form "/node/path/prop=val" and
sets the corresponding OF property.  (Use this to set /chosen/bootargs.)

=item B<--fileprop>

Takes arguments of the form "/node/path/prop=file" and
sets the corresponding OF property, using the contents of the file "file".

=item B<--name>

Specifies name of LPAR container to operate on.

=item B<--help>

Prints a brief message and exits.

=item B<--man>

Prints the manual page and exits.

=back

=head1 DESCRIPTION

B<This program> prepares and manages LPAR images prior to LPAR start.

=cut
