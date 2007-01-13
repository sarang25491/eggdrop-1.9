/* mycallable.c: Python scripting support - custom object for variable mappings and function calls
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: mystdio.c,v 1.2 2007/01/13 12:23:40 sven Exp $";
#endif

#include <Python.h>
#include <structmember.h>
#include <eggdrop/eggdrop.h>

#include "pythonscript.h"

#define Stdio_Check(op) PyObject_TypeCheck(op, &Stdio_Type)

typedef struct {
	PyObject_HEAD
	int softspace;
	unsigned Type;
} StdioObject;

typedef struct {
	char *buf;
	unsigned bufsize;
	unsigned strlen;
} tLineBuf;

static tLineBuf LineBuf[2];

partymember_t *LogTarget;

static PyObject *Stdio_New(PyTypeObject *Type, PyObject *args, PyObject *kwds) {
	unsigned IOtype;
	PyObject *Para;
	StdioObject *Stdio;

	if (!PyArg_ParseTuple(args, "O", &Para)) return 0;
	if (Stdio_Check(Para)) {
		Stdio = (StdioObject *) Para;
		IOtype = Stdio->Type;
	} else if (PyInt_Check(Para)) {
		IOtype = PyInt_AsLong(Para) - 1;
	} else {
		PyErr_Format(PyExc_TypeError, "Expected eggdrop.stdio got %s", Para->ob_type->tp_name);
		return 0;
	}
	if (!(Stdio = (StdioObject *) Type->tp_alloc(Type, 0))) return 0;
	Stdio->softspace = 0;
	Stdio->Type = IOtype;
	return (PyObject *) Stdio;
}

static void Log(int Target, const char *Text) {
	char *Prefix;

	if (!Target) Prefix = "Python: %s";
	else Prefix = "Python Error: %s";

	if (LogTarget) {
		partymember_printf(LogTarget, Prefix, Text);
		return;
	}
	putlog(LOG_MISC, "*", Prefix, Text);
}

static size_t WriteLine(unsigned Target, char *Text, size_t len) {
	size_t written;
	char *newline;

	newline = memchr(Text, '\n', len);
	if (!newline) return 0;
	written = newline - Text + 1;
	*newline = 0;
	if (written > 1 && newline[-1] == '\r') newline[-1] = 0;
	Log(Target, Text);
	return written;
}

static void Write(int Target, const char *Text, int len) {
	int written;
	char *TextPtr;
	tLineBuf *Line = &LineBuf[Target];

	if (Line->strlen + len > Line->bufsize) {
		Line->bufsize = ((Line->strlen + len) & ~0x3FF) + 1024;     // make it up to 1 KB larger than necessary
		Line->buf = realloc(Line->buf, Line->bufsize);
	}
	memcpy(Line->buf + Line->strlen, Text, len);
	Line->strlen += len;
	TextPtr = Line->buf;
	while ((written = WriteLine(Target, TextPtr, Line->strlen))) {
		TextPtr += written;
		Line->strlen -= written;
	}
	if (Line->strlen) memmove(Line->buf, TextPtr, Line->strlen);
	if (Line->bufsize > 0x100000 && Line->strlen < 1024) {
		Line->bufsize = 1024;
		Line->buf = realloc(Line->buf, 1024);
	}
}

void Flush(unsigned Target) {
	if (LineBuf[Target].strlen) Write(Target, "\n", 1);
}

static PyObject *Stdio_Write(PyObject *self, PyObject *args) {
	int len;
	const char *Text;
	StdioObject *this = (StdioObject *) self;

	if (!PyArg_ParseTuple(args, "s#", &Text, &len)) return 0;
	if (this->Type < 2) Write(this->Type, Text, len);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *Stdio_Flush(PyObject *self, PyObject *args) {
	StdioObject *this = (StdioObject *) self;

	if (this->Type < 2) Flush(this->Type);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Stdio_Members[] = {
	{"softspace", T_INT, offsetof(StdioObject, softspace), 0, "Has a documented use for print and an undocumented use for the interactive interpreter."},
	{0}
};

static PyMethodDef Stdio_Methods[] = {
 {"write", Stdio_Write, METH_VARARGS, "Write some text"},
 {"flush", Stdio_Flush, METH_NOARGS, "Display the buffered content *now*"},
 {0}
};

PyTypeObject Stdio_Type = {
	PyObject_HEAD_INIT(0)        /* required by law */
	0,                           /* ob_size, obsolete, always 0 */
	"eggdrop.stdio",             /* tp_name, type callable in module eggdrop */
	sizeof(StdioObject),         /* tp_basicsize, size of the C struct belonging to this type */
	0,                           /* tp_itemsize, only for variable types, always 0 */ 
	0,                           /* tp_dealloc, the destructor, we don't need one */
	0,                           /* tp_print, don't write me into a file */
	0,                           /* tp_getattr, deprecated, we'll use tp_getattro */
	0,                           /* tp_setattr, deprecated, we'll use tp_setattro */
	0,                           /* tp_compare */
	0,                           /* tp_repr, should there be one */
	0,                           /* tp_as_number*/
	0,                           /* tp_as_sequence*/
	0,                           /* tp_as_mapping*/
	0,                           /* tp_hash, no hashing */
	0,                           /* tp_call, */
	0,                           /* tp_str */
	0,                           /* tp_getattro */
	0,                           /* tp_setattro */
	0,                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE,         /* tp_flags */
	"handels logging and partyline output using sys.stdout and sys.stderr", /* tp_doc */
	0,                           /* tp_traverse */
	0,                           /* tp_clear */
	0,                           /* tp_richcompare */
	0,                           /* tp_weaklistoffset */
	0,                           /* tp_iter */
	0,                           /* tp_iternext */
	Stdio_Methods,               /* tp_methods */
	Stdio_Members,               /* tp_members */
	0,                           /* tp_getset */
	0,                           /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	0,                           /* tp_init */
	0,                           /* tp_alloc */
	Stdio_New                    /* tp_new */
};
