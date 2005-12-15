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
static const char rcsid[] = "$Id: mycallable.c,v 1.1 2005/12/15 15:26:12 sven Exp $";
#endif

#include <Python.h>
#include <eggdrop/eggdrop.h>

#include "pythonscript.h"

static PyObject *my_command_handler(PyObject *self, PyObject *pythonargs, PyObject *kwd) {
	script_var_t retval;
	script_args_t args; 
	CallableObject *Instance = (CallableObject *) self;
	script_raw_command_t *cmd = Instance->client_data;

	if (Instance->type != PYTHON_FUNC) {
		PyErr_SetString(PyExc_TypeError, "Object is not callable");
		return 0;
	}

	if (!cmd) {
		PyErr_SetString(PyExc_TypeError, "'callable' object is not callable. The C function has been removed. "
		 "DON'T KEEP REFERENCES TO EGGDROP FUNCTIONS!");   /* no really, don't */
		return 0;
	}

	retval.type = 0;
	retval.value = 0;
	retval.len = -1;
	
	args.module = &my_script_interface;
	args.client_data = pythonargs;
	args.len = PyObject_Length(pythonargs);

	cmd->callback(cmd->client_data, &args, &retval);
	if (retval.type & SCRIPT_ERROR) {
		if (retval.type & SCRIPT_STRING) PyErr_SetString(PyExc_RuntimeError, retval.value);
		else PyErr_SetString(PyExc_TypeError, "error message unavailable");
		return 0;
	}
	return c_to_python_var(&retval);
}

PyObject *Repr(PyObject *self) {
	PyObject *Ret, *Repr;
	CallableObject *Obj = (CallableObject *) self;
	script_linked_var_t *var = Obj->client_data;
	script_raw_command_t *func = Obj->client_data;

	if (!Obj->client_data) return PyString_FromString("<Expired eggdrop object. DROP THIS REFERENCE!>");
	if (Obj->type == PYTHON_FUNC) {
		script_command_t *com = func->client_data;    /* Can we really assume this always works? */
		return PyString_FromFormat("<Eggdrop function '%s': %s>", func->name, com->syntax_error);
	}
	Ret = GetVar(var);   /* new reference */
	if (!Ret) {
		PyObject *ExcType, *ExcText, *Trace;
		PyErr_Fetch(&ExcType, &ExcText, &Trace);   /* 3 new references */
		Repr = PyString_FromFormat("<%s>", PyString_AsString(ExcText));
		Py_XDECREF(ExcType);
		Py_XDECREF(ExcText);
		Py_XDECREF(Trace);
		return Repr;
	}
	Repr = PyObject_Repr(Ret);
	Py_DECREF(Ret);
	return Repr;
}

PyTypeObject Callable_Type = {
	PyObject_HEAD_INIT(0)        /* required by law */
	0,                           /* ob_size, obsolete, always 0 */
	"eggdrop.callable",          /* tp_name, type callable in module eggdrop */
	sizeof(CallableObject),      /* tp_basicsize, size of the C struct belonging to this type */
	0,                           /* tp_itemsize, only for variable types, always 0 */ 
	0,                           /* tp_dealloc, the destructor, we don't need one */
	0,                           /* tp_print, don't write me into a file */
	0,                           /* tp_getattr, deprecated, we'll use tp_getattro */
	0,                           /* tp_setattr, deprecated, we'll use tp_setattro */
	0,                           /* tp_compare */
	Repr,                        /* tp_repr, should there be one */
	0,                           /* tp_as_number*/
	0,                           /* tp_as_sequence*/
	0,                           /* tp_as_mapping*/
	0,                           /* tp_hash, no hashing */
	my_command_handler,          /* tp_call, this is the point of the whole thing */
	0,                           /* tp_str */
	0,                           /* tp_getattro */
	0,                           /* tp_setattro */
	0,                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,          /* tp_flags */
	"callable eggdrop function", /* tp_doc */
	0,                           /* tp_traverse */
	0,                           /* tp_clear */
	0,                           /* tp_richcompare */
	0,                           /* tp_weaklistoffset */
	0,                           /* tp_iter */
	0,                           /* tp_iternext */
	0,                           /* tp_methods */
	0,                           /* tp_members */
	0,                           /* tp_getset */
	0,                           /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	0,                           /* tp_init */
	0,                           /* tp_alloc */
	0                            /* tp_new */
};
