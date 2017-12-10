#!/bin/sh
#
# Copyright (C) 2012, 2013 glevand <geoffrey.levand@mail.ru>
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

if [ $# -lt 1 ]; then
	echo "usage: $0 <version> [<local version>]" >&2
	exit 1
fi

VERSION=$1
LOCALVERSION=$2
INSTALL_DIR=linux-${VERSION}${LOCALVERSION}-build

#make ARCH=powerpc CROSS_COMPILE=powerpc64-linux- menuconfig

make ARCH=powerpc CROSS_COMPILE=powerpc64-linux- clean
make -j4 ARCH=powerpc CROSS_COMPILE=powerpc64-linux- LOCALVERSION=$LOCALVERSION

rm -rf $HOME/$INSTALL_DIR*
mkdir -p $HOME/$INSTALL_DIR/boot

make ARCH=powerpc CROSS_COMPILE=powerpc64-linux- install INSTALL_PATH=$HOME/$INSTALL_DIR/boot LOCALVERSION=$LOCALVERSION
make ARCH=powerpc CROSS_COMPILE=powerpc64-linux- modules_install INSTALL_MOD_PATH=$HOME/$INSTALL_DIR LOCALVERSION=$LOCALVERSION
make ARCH=powerpc CROSS_COMPILE=powerpc64-linux- headers_install INSTALL_HDR_PATH=$HOME/$INSTALL_DIR/usr LOCALVERSION=$LOCALVERSION

cd $HOME
tar cvJf $INSTALL_DIR.tar.xz $INSTALL_DIR
