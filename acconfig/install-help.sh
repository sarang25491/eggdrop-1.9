#!/bin/sh
#
# Copyright (C) 2004 Eggheads Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Id: install-help.sh,v 1.2 2004/06/17 20:52:34 wingman Exp $
#

if [ "$2" = "" ];
then
	echo "Usage: $0 <destdir> <srcdir>";
	exit
fi

DESTDIR=$1
SRCDIR=$2
HELPDIR=help
HELPFILES="commands.xml variables.xml"
MODULE=`pwd | awk '{ len = split ($0, tok, "/"); print tok[len]; }'`

if [ "$MODULE" = "eggdrop1.7" ];
then
	MODULE="core"
fi

if [ ! -d $DESTDIR/$HELPDIR ];
then
	mkdir $DESTDIR/$HELPDIR
fi

for dir in $SRCDIR/$HELPDIR/*;
do
	MATCH=`echo "$dir" | grep "[a-z]\{2\}_[A-Z]\{2\}"`
	if [ ! -z "$MATCH" ];
	then
		LANG=`echo $dir | awk '{ len = split ($0, tok, "/"); print tok[len]}'`

		if [ ! -d "$DESTDIR/$HELPDIR/$LANG" ];
		then	
			mkdir "$DESTDIR/$HELPDIR/$LANG"
		fi

		for file in $HELPFILES
		do
			if [ -e "$dir/$file" ]
			then
				echo "Installing $HELPDIR file for language $LANG: $file"
				cp $dir/$file $DESTDIR/$HELPDIR/$LANG/$MODULE-$file
			fi
		done
	fi
done

