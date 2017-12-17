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
# /

package HypeConst;
require Exporter;
@ISA = qw(Exporter);

# Resource types
use constant MEM_ADDR  => 1;
use constant MMIO_ADDR => 2;
use constant INTR_SRC  => 3;
use constant DMA_WIND  => 4;
use constant IPI_PORT  => 5;
use constant PRIVATE   => 6;


use constant HVIO_ACQUIRE => 1;
use constant HVIO_RELEASE => 2;

# VIO Classes / XIRR Classes
use constant HVIO_HW	=> 0;
use constant HVIO_IIC	=> 0;
use constant HVIO_LLAN	=> 1;
use constant HVIO_AIPC	=> 2;
use constant HVIO_CRQ	=> 3;
use constant HVIO_VTERM => 4;

# Hcalls
use constant H_CREATE_PARTITION => "0x6010";
use constant H_START		=> "0x6014";
use constant H_VIO_CTL		=> "0x6018";
use constant H_SET_SCHED_PARAMS => "0x6028";
use constant H_RESOURCE_TRANSFER => "0x602C";
use constant H_DESTROY_PARTITION => "0x6038";

use constant H_SUCCESS => 0;

use constant PGSIZE		=> 4096;
use constant MB			=> (1<<20);
use constant CHUNK_SIZE		=> 64 * MB;

# Export everything by default
@EXPORT = keys(%HypeConst::);

1;
