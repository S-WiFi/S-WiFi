#! /bin/sh
#
# Copyright (C) 2016 by Tao Zhao
#
# Reinstall ns-2.
#
# Abridged from the top-level install script.
#
# Copyright (C) 2000 by USC/ISI
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# $Header: /cvsroot/nsnam/ns-2/allinone/install,v 1.31 2007/03/10 23:40:05 tom_henderson Exp $

die() {
	echo "$@"  1>&2
	exit 1
}

# Package VERSIONs. Change these when releasing new packages
TCLVER=8.5.10
TKVER=8.5.10
OTCLVER=1.14
TCLCLVER=1.20
NSVER=2.35

echo "============================================================"
echo "* Build ns-$NSVER"
echo "============================================================"

if [ -f Makefile ] ; then 
	make distclean
fi

./configure --with-otcl=../otcl-$OTCLVER --with-tclcl=../tclcl-$TCLCLVER --with-tcl-ver=$TCLVER --with-tk-ver=$TKVER || die "Ns configuration failed! Exiting ...";

if make
then
	echo " Ns has been reinstalled successfully." 
else
	echo "Ns make failed!"
	echo "See http://www.isi.edu/nsnam/ns/ns-problems.html for problems"
	exit
fi
