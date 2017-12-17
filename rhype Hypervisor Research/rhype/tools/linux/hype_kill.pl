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
#  Kills running LPARs, returns them to "READY" state
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

use lib "$libdir/perl";
use HypeConst;
use HypeUtil;

Getopt::Long::Configure qw(no_ignore_case);


my $laddr;
my $ofstub = "$libdir/of_image";
my $oftree = "$libdir/of_tree";
my $help = 0;
my $man = 0;
GetOptions("h|help" => \$help,
	   "man"	=> \$man,
	   "n|name=s" => \$pname) or pod2usage(2);

pod2usage(1) if $help;
pod2usage(-exitstatus => 0, -verbose => 2) if $man;

if (!defined $pname) {
  die "Must specify an LPAR";
}

if (!-d "$vroot/$pname") {
  die "Partition '$pname' does not exist";
}

my $status = get_file("state");

if ($status ne "RUNNING") {
  print STDERR "Partition $pname is not running ($status)\n";
  exit -1;
}

my $lpid = get_file("lpid");

if(!defined $lpid || oct($lpid)==0){
  print STDERR "Bad lpid '$lpid'\n";
  exit -1;
}

(my $retval) = hcall(H_DESTROY_PARTITION, $lpid);


if(oct($retval) != H_SUCCESS){
  print STDERR "Failure: $retval\n";
  exit -1;
}

set_file("state", "READY");
set_file("lpid", "");
print "Status is: $status $retval\n";

exit 0;

__END__

=head1 NAME

oh_kill - Kill an LPAR

=head1 SYNOPSIS

oh_kill [options]

  Options:
	-h|--help		brief help message
	--man			prints full man page
	-n|--name		name of LPAR to kill

=head1 OPTIONS

=over 8

=item B<--name>

Specifies name of LPAR to kill.

=item B<--help>

Prints a brief message and exits.

=item B<--man>

Prints the manual page and exits.

=back

=head1 DESCRIPTION

B<This program> kills a currently running LPAR.

=cut
