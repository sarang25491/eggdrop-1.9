#!/usr/bin/python
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
# $Id: gen-docs.py,v 1.1 2004/06/19 13:18:48 wingman Exp $
#
# This file (is intended) to generate our developer documents (txt, html, ...)
# out of all binds.xml, commands.xml, functions.xml, ... found.
#
# Flow:
#
#   for each file found in ///*.xml
#	update doc/development/SCRIPTING-FUNCTIONS
#	update doc/development/SCRIPTING-BINDS
#	update doc/development/SCRIPTING-VARIABLES
#	update doc/html/functions.html
#	...
#   
