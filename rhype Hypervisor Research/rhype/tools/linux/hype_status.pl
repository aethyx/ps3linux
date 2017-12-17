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
#  Prints status of LPARs.
#
use File::stat;
use File::Basename;
use IO::File;
use IO::Handle;
use strict;
use Getopt::Long;

my $libdir;
BEGIN{
  use Cwd 'abs_path';
  $libdir = abs_path(dirname($0)) . '/../lib/hype';
}

use lib "$libdir/perl";
use HypeConst;
use HypeUtil;

my $status;
my $lpid;
format HEADER =
LPAR Name      LPID	Status
.
format STDOUT =
@<<<<<<<<<<<<< @<<<<<<< @<<<<<<<<<
$pname, $lpid, $status
.


opendir(DIR, $vroot) or abort "$vroot not accessible\n";

write HEADER;

my @dirs = grep { !/^\.{1,2}$/ } readdir(DIR);

map {
  $pname = $_;
  $status = get_file("state");
  $lpid = get_file("lpid");
  write STDOUT;
} @dirs;
