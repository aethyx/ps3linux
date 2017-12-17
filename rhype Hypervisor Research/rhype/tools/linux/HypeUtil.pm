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
#  Hype Tools Utility Module
#

package HypeUtil;
use File::stat;
use File::Basename;
use IO::File;
use IO::Handle;
use strict;
BEGIN{
  use Exporter ();
  our @EXPORT;
  our @ISA;
  @ISA = qw(Exporter File::stat);
  @EXPORT = qw(align_up align_down hexstr split_hexstr extract_data hcall
	       laddr_load abort get_tempfile cleanup
	       set_file get_file of_make_node of_set_prop
	       $vroot $pname);
}

our $vroot;
our $pname;


$vroot = "/var/hype";

#
# Manage all of the temporary files we create.
# On error, don't die; abort -- abort will delete all temp files
#
my @delfiles;
sub get_tempfile();
sub cleanup();
sub abort($); # string = error message


sub align_down($$);
sub align_up($$);

# if $foo is numeric
# hexstr($foo) eq sprintf("0x%x", $foo)
sub hexstr($){
  my $val = shift;
  if (0 == index $val, '0x') {
    return $val;
  }
  return "0x" . unpack("H*", pack("N*", $val));
}

sub split_hexstr($) {
  my $val = shift;
  $val =~ /0x([[:xdigit:]]{0,8}?)([[:xdigit:]]{1,8})$/;
  my $x = $1;
  if($x eq ""){
    $x = "0";
  }

  my @ret;
  push @ret, oct("0x" . $x);
  push @ret, oct("0x" . $2);
  return @ret;
}
#
# Gets/sets data to/from files in $vroot/$pname/
#
sub set_file($$);
#  (my $file, my $data) = @_;

sub get_file($);
#  my $file = shift;

sub of_make_node($);
#  my $node = shift;

sub of_set_prop($$$);
#  (my $node, my $prop, my $data) = @_;

# Same as above, but take data from the specified file
sub of_set_prop_from_file($$$);
#  (my $node, my $prop, my $file) = @_;

#
# Extract binary data from a file.
# Extracts $count units of size $size locate at $offset
#
sub extract_data($$$$); #  my ($fh, $offset, $count, $size) = @_;


#
# Perform an hcall, returns return values from hcall
#
sub hcall(@){
  my $l = join(" ", @_);
  print("Hcall: $l\n");
  my $ret = `hcall $l`;
  if($? == 0){
    my @x;
    @x = split /\s+/, $ret;
    return @x;
  } else {
    return undef;
  };
}

#
# Load a file into a logical address
#
sub laddr_load($$){
  my $file = shift;
  my $addr = shift;
  my $size = stat($file)->size;

  system("hload -l$addr -f$file");

  if(0 != $?) {
    abort("hload failure: " .  $?/256);
  }
  return $size;
}




sub align_down($$){
  (my $a, my $b) = @_;
  $a & ~($b - 1);
}

sub align_up($$){
  (my $a, my $b) = @_;
  ($a + $b - 1) & ~($b - 1);
}


sub get_tempfile(){
  my $tempfile = `tempfile`;
  $tempfile =~ s/\s//g;
  push @delfiles, $tempfile;
  return $tempfile;
}

sub cleanup(){
  while($#delfiles>=0){
    my $x = pop @delfiles;
    unlink $x;
  }
}

sub abort($){
  my $msg = shift;
  cleanup();
  print "ABORT: $msg\n";
  exit -1;
}

#
# Extract binary data from a file.
# Extracts $count units of size $size locate at $offset
#
sub extract_data($$$$){
  my ($fh, $offset, $count, $size) = @_;
  my $buf;
  $fh->sysseek($offset, SEEK_SET);
  my $ret = $fh->sysread($buf, $count * $size, 0);

  if($ret != $count * $size) { 
    abort("IO failure ($ret/$!)\n");
  }

  my $code;
  if($size == 1) { 
    $code = "C" . $count;
  } elsif($size == 2) {
    $code = "S" . $count;
  } elsif($size == 4) {
    $code = "L" . $count;
  } else {
    abort("Don't know how to extract units of size $size");
  }

  unpack($code, $buf);
}

sub set_file($$){
  (my $file, my $data) = @_;
  my $name = "$vroot/$pname/$file";
  my $fh = new IO::File $name, O_WRONLY|O_CREAT|O_TRUNC;

  if(!defined $fh) {
    abort "Can't create partition data file $vroot/$pname/$file";
  }

  $fh->syswrite($data);

  $fh->close;
}

sub get_file($){
  my $file = shift;
  my $name = "$vroot/$pname/$file";
  my $fh = new IO::File $name, O_RDONLY;

  if(!defined $fh) {
    return undef;
  }

  my $buf;
  $fh->sysread($buf, 4096);

  $fh->close;
  return $buf;
}


sub of_make_node($) {
  my $node = shift;
  my $name = "$vroot/$pname/of_tree/$node";
  if(-e $name) {
    return 0;
  }
  mkdir $name;
  return 1;
}

sub of_set_prop($$$) {
  (my $node, my $prop, my $data) = @_;
  set_file("of_tree/$node/$prop", $data);
}

sub of_set_prop_from_file($$$) {
  (my $node, my $prop, my $file) = @_;
  my $dir = "$vroot/$pname/of_tree/$node";
  if (-d $dir) { 
    `cp $file $dir/$prop`;
  }
}

1;
