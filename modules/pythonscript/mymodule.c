/* mymodule.c: Python scripting support - custom module for variable mappings
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

/* Large parts of this file are copy 'n' pasted form the python project
 * python/dist/src/Objects/moduleobject.c and python/dist/src/Python/modsupport.c
 */

#ifndef lint
static const char rcsid[] = "$Id: mymodule.c,v 1.1 2005/12/15 15:26:12 sven Exp $";
#endif

#include <Python.h>
#include <structmember.h>
#include <eggdrop/eggdrop.h>

#include "pythonscript.h"

static PyTypeObject MyModule_Type;

static PyObject *MyModule_New(char *name) {
	int set1, set2;
	PyObject *NameObj, *ModuleObj, **DictPtr;

	ModuleObj = PyObject_GC_New(PyObject, &MyModule_Type);   /* new reference */
	if (!ModuleObj) return 0;
	NameObj = PyString_FromString(name);    /* new reference */
	DictPtr = _PyObject_GetDictPtr(ModuleObj);
	*DictPtr = MyDict_New(&MyDict_Type, 0, 0);
	if (!*DictPtr || !NameObj) {
		Py_DECREF(ModuleObj);
		Py_XDECREF(NameObj);
		return 0;
	}
	set1 = PyDict_SetItemString(*DictPtr, "__name__", NameObj);
	Py_DECREF(NameObj);
	set2 = PyDict_SetItemString(*DictPtr, "__doc__", Py_None);
	if (set1 || set2) {
		Py_DECREF(ModuleObj);
		return 0;
	}
	PyObject_GC_Track(ModuleObj);
	return ModuleObj;
}

/* Warning: Like PyImport_AddModule() this does *not* return a new reference */

PyObject *MyModule_Add(char *name, char *doc) {
	PyObject *Modules, *ModuleObj;

	Modules = PyImport_GetModuleDict();        /* Warning: Borrowed reference */
	if ((ModuleObj = PyDict_GetItemString(Modules, name)) && PyModule_Check(ModuleObj)) return ModuleObj;
	ModuleObj = MyModule_New(name);            /* new reference */
	if (!ModuleObj) return 0;
	if (PyDict_SetItemString(Modules, name, ModuleObj)) {
		Py_DECREF(ModuleObj);
		return 0;
	}

	if (doc) {
		PyObject **DictPtr, *DocObj;

		DictPtr = _PyObject_GetDictPtr(ModuleObj);
		DocObj = PyString_FromString(doc);       /* new reference */
		if (DocObj) PyDict_SetItemString(*DictPtr, "__doc__", DocObj);
		Py_XDECREF(DocObj);
		PyErr_Clear();
	}

	Py_DECREF(ModuleObj); /* Yes, it still exists, in modules! */
	return ModuleObj;
}

static PyObject *DictString;

static PyObject *GetAttr(PyObject *self, PyObject *Key) {
	PyObject **DictPtr, *Ret;

	DictPtr = _PyObject_GetDictPtr(self);
	if (!PyObject_Compare(DictString, Key)) {
		Py_INCREF(*DictPtr);
		return *DictPtr;
	}
	Ret = (*DictPtr)->ob_type->tp_as_mapping->mp_subscript(*DictPtr, Key);
	if (!Ret && PyErr_ExceptionMatches(PyExc_KeyError)) {
		PyErr_Format(PyExc_AttributeError, "'%.50s' object has no attribute '%.400s'", self->ob_type->tp_name, PyString_AS_STRING(Key));
	}
	return Ret;
}

/* This is the same as SetItem in mydict.c
 * It has to be done twice to get the error messages right,
 * attribute vs. item and KeyError vs. TypeError
 * Remember to keep them both in sync. */
 
static int SetAttr(PyObject *self, PyObject *Key, PyObject *Value) {
	PyObject **DictPtr, *Cur;
	CallableObject *Call;
	script_linked_var_t *var;

	DictPtr = _PyObject_GetDictPtr(self);
	Cur = PyDict_GetItem(*DictPtr, Key);       /* Warning: borrowed reference */
	if (!Cur) {
		PyErr_Format(PyExc_AttributeError, "'%s' object has no attribute '%s'", self->ob_type->tp_name, PyString_AsString(Key));
		return 1;
	}
	if (Cur->ob_type != &Callable_Type) return PyDict_SetItem(*DictPtr, Key, Value);
	Call = (CallableObject *) Cur;
	var = Call->client_data;
	if (!Value || Call->type != PYTHON_VAR || var->type & SCRIPT_READONLY) {
		PyErr_SetString(PyExc_TypeError, "readonly attribute");
		return 1;
	}
	return SetVar(var, Value);
}

static PyTypeObject MyModule_Type = {
	PyObject_HEAD_INIT(0)        /* required by law */
	0,                           /* ob_size, obsolete, always 0 */
	"eggdropmodule",             /* tp_name, type callable in module eggdrop */
	0,                           /* tp_basicsize, fill it in later */
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
	0,                           /* tp_call */
	0,                           /* tp_str */
	GetAttr,                     /* tp_getattro */
	SetAttr,                     /* tp_setattro */
	0,                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,          /* tp_flags */
	"module containing special mapped eggdrop variables",  /* tp_doc */
	0,                           /* tp_traverse */
	0,                           /* tp_clear */
	0,                           /* tp_richcompare */
	0,                           /* tp_weaklistoffset */
	0,                           /* tp_iter */
	0,                           /* tp_iternext */
	0,                           /* tp_methods */
	0,                           /* tp_members */
	0,                           /* tp_getset, none yet, will be added later */
	&PyModule_Type,              /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	0,                           /* tp_init */
	0,                           /* tp_alloc */
	0                            /* tp_new */
};

int MyModule_Init() {
	MyModule_Type.tp_basicsize = PyModule_Type.tp_basicsize;
	MyModule_Type.tp_dictoffset = PyModule_Type.tp_dictoffset;
	DictString = PyString_FromString("__dict__");
	return PyType_Ready(&MyModule_Type);
}
