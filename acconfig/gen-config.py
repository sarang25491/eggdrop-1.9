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
# $Id: gen-config.py,v 1.1 2004/06/19 13:18:48 wingman Exp $
#
# This file generates our default config.xml out of all found "variables.xml"
# in all subdirectories.
#
import sys
import os
from xml.dom import minidom

class ConfigGenerator:

	def __init__(self, version):
		self.doc_out = minidom.getDOMImplementation().createDocument(None, "config", None)		
		self.doc_out.documentElement.setAttribute("version", version)	

	def evalSection(self, section):
		for node in section.childNodes:
			if node.nodeName == "variable":
				self.evalVariable(node)
		pass

	def getText(self, parent):
		text = ""
		for node in parent.childNodes:
			if node.nodeType == minidom.Node.TEXT_NODE or node.nodeType == minidom.Node.CDATA_SECTION_NODE:
				text += node.nodeValue
		return text.strip()
		
	def getDescription(self,variable):
		try:
			for node in variable.getElementsByTagName("description"):
				return self.getText(node)
		except:
			pass
		return ""
		
	def evalVariable(self, variable):
		name = variable.getAttribute("name")
		path = variable.getAttribute("path")
		type = variable.getAttribute("type")
		description = self.getDescription(variable)
		default = variable.getElementsByTagName("default")
			
		if (len(description) == 0):
			print "WARN: Missing description: %s" % (name)
			
		if (len(default) < 1):
			print "INFO: Skipping %s because of no default value." % (name)
			return
			
		# Create node
		parent = self.doc_out.documentElement	
		for tok in path.split('.'):
			nodes = parent.getElementsByTagName(tok)
			if (len(nodes) == 0):
				node = self.doc_out.createElement(tok)
				parent.appendChild (node)
				parent = node
			else:
				parent = nodes[0]			

		# Add description
		if len(description) > 0:
			parent.parentNode.insertBefore(
				self.doc_out.createComment(" %s " % (description)), parent)
			
			
		if not type == "list":
			parent.appendChild (self.doc_out.createTextNode (
				self.getText (default[0])))
		else:
			doc = minidom.parseString ("<root>%s</root>" % self.getText(default[0]))
			for node in doc.documentElement.childNodes:
				parent.appendChild (node)
		
	def load(self, path):
		for root, dirs, files in os.walk(path):	
			for file in files:
				if file == "variables.xml":
					self.parse("%s/%s" % (root, file))
			
	def parse(self, file):
		print "Adding : %s." % (file)
		self.doc_in = minidom.parse(file)
		for node in self.doc_in.documentElement.childNodes:
			if node.nodeName == "section":
				self.evalSection(node)
			elif node.nodeName == "variable":
				self.evalVariable(node)

	def countChildNodes(self, parent):
		i = 0
		for node in parent.childNodes:
			if node.nodeType == minidom.Node.ELEMENT_NODE:
				i = i + 1
		return i
				
	def save_element (self, out, indent, element):
		count = self.countChildNodes (element)
		
		out.write("%s<%s" % (indent, element.nodeName))
		for key in element._attrs:
			out.write(" %s='%s'" % (key, element.getAttribute(key)))
		out.write(">")
		
		if count > 0:
			out.write("\n")

		for node in element.childNodes:
			if node.nodeType == minidom.Node.ELEMENT_NODE:
				self.save_element(out, "%s\t" % (indent), node)
			elif node.nodeType == minidom.Node.TEXT_NODE:
				if len(node.nodeValue.strip()):
					out.write (node.nodeValue)
			elif node.nodeType == minidom.Node.COMMENT_NODE:
				out.write("%s\t<!-- %s -->\n" % (indent, node.nodeValue))

		if count > 0:
			out.write("%s" % indent)				
		out.write("</%s>\n" % (element.nodeName))		
		
	def save(self, file):
		print "Saving : %s." % (file)
		out = open(file, "w")
		out.write ("<?xml version='1.0'?>\n")
		self.save_element (out, "", self.doc_out.documentElement)
		out.close()
				
				
if len(sys.argv) != 4:
	print "Usage %s <config-version> <input> <output>" % (sys.argv[0])
else:
	if os.path.exists(sys.argv[3]):
		print "Output file already exists. If you want to regenerate "
		print "the default config file remove '%s'." % (sys.argv[3])
	else:
		gen = ConfigGenerator(sys.argv[1])
		gen.load(sys.argv[2])
		gen.save (sys.argv[3])
