/* mydict.c: Python scripting support - custom dict for variable mappings
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
static const char rcsid[] = "$Id: mydict.c,v 1.1 2005/12/15 15:26:12 sven Exp $";
#endif

#include <Python.h>
#include <eggdrop/eggdrop.h>

#include "pythonscript.h"

#define MyDict_Check(op) PyObject_TypeCheck(op, &MyDict_Type)

PyObject *MyDict_New(PyTypeObject *Type, PyObject *args, PyObject *kwds) {
	PyObject *O = PyDict_Type.tp_new(Type, args, kwds);
	return O;
}

PyObject *GetVar(script_linked_var_t *var) {
	script_var_t newvalue = {0};

	newvalue.len = -1;
	if (var->callbacks && var->callbacks->on_read) {
		if (var->callbacks->on_read(var, &newvalue)) {
			PyErr_Format(PyExc_LookupError, "internal eggdrop error in 'on_read' function for '%s'", var->name);
			return 0;
		}
	} else {
		newvalue.type = var->type & SCRIPT_TYPE_MASK;;
		newvalue.value = *(void **)var->value;
	}
	return c_to_python_var(&newvalue);
}

int SetVar(script_linked_var_t *var, PyObject *Value) {
	script_var_t newvalue = {0};

	if (python_to_c_var(Value, &newvalue, var->type)) return 1;
	if (script_linked_var_on_write(var, &newvalue)) {
		PyErr_SetString(PyExc_RuntimeError, "set");
		return 1;
	}
	return 0;
}

static PyObject *GetItem(PyObject *self, PyObject *Key) {
	PyObject *Ret;
	CallableObject *Cal;

	Ret = PyDict_GetItem(self, Key); /* Warning: Borrowed reference */
	Cal = (CallableObject *) Ret;
	
	if (!Ret || !Cal->client_data) {
		PyErr_Format(PyExc_KeyError, "'%s'", PyString_AS_STRING(Key));
		return 0;
	}
	if (Ret->ob_type != &Callable_Type || Cal->type == PYTHON_FUNC) {
		Py_INCREF(Ret);
		return Ret;
	}
	return GetVar(Cal->client_data);
}

/* This is the same as SetAttr in mymodule.c
 * It has to be done twice to get the error messages right,
 * attribute vs. item and KeyError vs. TypeError
 * Remember to keep them both in sync. */
 
static int SetItem(PyObject *self, PyObject *Key, PyObject *Value) {
	PyObject *Ret;
	CallableObject *Call;
	script_linked_var_t *var;

	Ret = PyDict_GetItem(self, Key);        /* Warning: Borrowed reference */
	if (!Ret || !Value) {
		PyErr_SetString(PyExc_TypeError, "object does not support item assignment");
		return 1;
	}
	if (Ret->ob_type != &Callable_Type) return PyDict_SetItem(self, Key, Value);
	Call = (CallableObject *) Ret;
	var = Call->client_data;
	if (!var || Call->type != PYTHON_VAR || var->type & SCRIPT_READONLY) {
		PyErr_SetString(PyExc_TypeError, "object does not support item assignment");
		return 1;
	}
	return SetVar(var, Value);
}

static PyMappingMethods mapstr = {
	0,                           /* GetLen, defaults to normal dict function */
	GetItem,
	SetItem
};

PyTypeObject MyDict_Type = {
	PyObject_HEAD_INIT(0)        /* required by law */
	0,                           /* ob_size, obsolete, always 0 */
	"eggdrop.mapping",           /* tp_name, type callable in module eggdrop */
	sizeof(MyDictObject),        /* tp_basicsize, size of the C struct belonging to this type */
	0,                           /* tp_itemsize, only for variable types, always 0 */ 
	0,                           /* tp_dealloc, the destructor, we don't need one */
	0,                           /* tp_print, don't write me into a file */
	0,                           /* tp_getattr, deprecated, we'll use tp_getattro */
	0,                           /* tp_setattr, deprecated, we'll use tp_setattro */
	0,                           /* tp_compare */
	0,                           /* tp_repr, should there be one */
	0,                           /* tp_as_number*/
	0,                           /* tp_as_sequence*/
	&mapstr,                     /* tp_as_mapping*/
	0,                           /* tp_hash, no hashing */
	0,                           /* tp_call */
	0,                           /* tp_str */
	0,                           /* tp_getattro */
	0,                           /* tp_setattro */
	0,                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,          /* tp_flags */
	"mapped eggdrop variables",  /* tp_doc */
	0,                           /* tp_traverse */
	0,                           /* tp_clear */
	0,                           /* tp_richcompare */
	0,                           /* tp_weaklistoffset */
	0,                           /* tp_iter */
	0,                           /* tp_iternext */
	0,                           /* tp_methods */
	0,                           /* tp_members */
	0,                           /* tp_getset, none yet, will be added later */
	&PyDict_Type,                /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	0,                           /* tp_init */
	0,                           /* tp_alloc */
	MyDict_New                   /* tp_new */
};
