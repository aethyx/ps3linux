#! /bin/sh
#
# Make sure we are run by bash
#
if [ -z "$interp" ]; then
    #
    # Need to make sure we are running bash.
    # We also want to time the script.
    #
    interp=yes
    export interp
    exec sh -c "bash $0 $*"
fi
#
# Copyright (C) 2005 Jimi Xenidis <jimix@watson.ibm.com>, IBM Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# $Id$
# 
#
#set -x

indent=$(type -p indent)

if test -z "$indent"; then
  echo "indent not found"
  exit 1;
fi

if ! ${indent} --k-and-r-style < /dev/null; then
  echo "only GNU indent is supported"
  exit 1
fi

if test -z "$1"; then
  p=.
else
  p=$1;
fi

# --k-and-r-style is
#     -nbad -bap -bbo -nbc -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0
#     -cp33 -cs -d0 -di1 -nfc1 -nfca -hnl -i4 -ip0 -l75 -lp -npcs
#     -nprs -npsl -saf -sai -saw -nsc -nsob -nss
knr="
--no-blank-lines-after-declarations
--blank-lines-after-procedures
--break-before-boolean-operator
--no-blank-lines-after-commas
--braces-on-if-line
--braces-on-struct-decl-line
--comment-indentation33
--declaration-comment-column33
--no-comment-delimiters-on-blank-lines
--cuddle-else
--continuation-indentation4
--case-indentation0
--else-endif-column33
--space-after-cast
--line-comments-indentation0
--declaration-indentation1
--dont-format-first-column-comments
--dont-format-comments
--honour-newlines
--indent-level4
--parameter-indentation0
--comment-line-length75
--continue-at-parentheses
--no-space-after-function-call-names
--no-space-after-parentheses
--dont-break-procedure-type
--space-after-for
--space-after-if
--space-after-while
--dont-star-comments
--leave-optional-blank-lines
--dont-space-special-semicolon
"

not_knr="
--blank-lines-after-declarations
--break-after-boolean-operator
--blank-lines-after-commas
--continuation-indentation8
--declaration-indentation0
--indent-level8
--start-left-side-of-comments
--space-special-semicolon
"
linux="
--tab-size8
--swallow-optional-blank-lines
--no-space-after-casts
"
indent_opts="
${knr} ${not_knr} ${linux}
--blank-before-sizeof
--cuddle-do-while
--else-endif-column8
--line-length79
--procnames-start-lines
-T uval -T uval8 -T uval16 -T uval32 -T uval64 -T uval128
-T sval -T sval8 -T sval16 -T sval32 -T sval64 -T sval128
-T rw_lock_t -T lock_t -T slot_mask_t -T htab_t -T ofdn_t
-T phandle -T ihandle -T xirr_t
"
ind="$indent $indent_opts"

fsrc() {
  prune="
-name CVS -o
-name gpltools -o
-name tools -o
-name thinwire-src
"
  skip="
-name tlb4xx.h -o
-name tlb44x.h 
"

  find $1 \( $prune \) -prune -o -type f ! \( $skip \) -name \*.[ch] -print
}


if test -d $p; then
  for file in $(fsrc $p); do
      $ind -st $file | diff -u $file -
  done
else
  file=$p
  $ind -st $file
fi


