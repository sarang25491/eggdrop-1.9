/* myegguser.c: Python scripting support - custom object representing an eggdrop user
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
static const char rcsid[] = "$Id: myegguser.c,v 1.1 2006/01/05 20:42:42 sven Exp $";
#endif

#include <Python.h>
#include <eggdrop/eggdrop.h>

#include "pythonscript.h"

#define Return_None do {\
	Py_INCREF(Py_None);\
	return Py_None;\
} while (0)

#define CheckSelf user_t *user;\
	EgguserObject *O = (EgguserObject *) self;\
	user = O->user;\
	if (!user || (user->flags & USER_DELETED)) {\
		PyErr_SetString(PyExc_RuntimeError, "This user has been deleted.");\
		return 0;\
	}

static PyObject *Egguser_New(PyTypeObject *Type, PyObject *args, PyObject *kwds) {
	user_t *user = 0;
	PyObject *Para;
	EgguserObject *Egguser;

	if (args) {
		if (!PyArg_ParseTuple(args, "O", &Para)) return 0;
		if (Egguser_Check(Para)) {
			Egguser = (EgguserObject *) Para;
			user = Egguser->user;
		} else if (PyInt_Check(Para)) {
			user = user_lookup_by_uid(PyInt_AsLong(Para));
		} else if (PyString_Check(Para)) {
			user = user_lookup_by_handle(PyString_AsString(Para));
		} else {
			PyErr_Format(PyExc_TypeError, "Expected eggdrop.egguser or int or str got %s.", Para->ob_type->tp_name);
			return 0;
		}
		if (!user) {
			PyErr_SetString(PyExc_RuntimeError, "No such user.");
			return 0;
		}
	}
	if (!(Egguser = (EgguserObject *) Type->tp_alloc(Type, 0))) return 0;
	Egguser->user = user;
	return (PyObject *) Egguser;
}

static PyObject *Repr(PyObject *self) {
	EgguserObject *Obj = (EgguserObject *) self;
	user_t *user = Obj->user;

//	if (!user) return PyString_FromString("<Deleted eggdrop user. Drop this reference.>");
//	return PyString_FromFormat("<Eggdrop user \"%s\" with uid %d>", user->handle, user->uid);
	return PyString_FromFormat("eggdrop.egguser(%d)", user->uid);
}

static int Compare(PyObject *self, PyObject *other) {
	user_t *u2;
	EgguserObject *O2 = (EgguserObject *) other;
	CheckSelf

	if (!Egguser_Check(other)) return -1;
	u2 = O2->user;
	if (!u2 || (u2->flags & USER_DELETED)) {
		PyErr_SetString(PyExc_RuntimeError, "This user has been deleted.");
		return -1;
	}
	return user->uid < u2->uid ? -1 : user->uid != u2->uid;
}

static long Hash(PyObject *self) {
	CheckSelf

	return user->uid * 15485863;
}

static PyObject *GetUid(PyObject *self, void *ignored) {
	CheckSelf

	return PyInt_FromLong(user->uid);
}

static PyObject *GetHandle(PyObject *self, void *ignored) {
	CheckSelf

	return PyString_FromString(user->handle);
}

static PyObject *GetIrcMasks(PyObject *self, void *ignored) {
	script_var_t retval;
	CheckSelf

	retval.type = SCRIPT_ARRAY | SCRIPT_STRING;
	retval.len = user->nircmasks;
	retval.value = user->ircmasks;
	return c_to_python_var(&retval);
}

static PyObject *Delete(PyObject *self, PyObject *na) {
	CheckSelf

	user_delete(user);
	Return_None;
}

static PyObject *AddMask(PyObject *self, PyObject *args) {
	const char *mask;
	CheckSelf

	if (!PyArg_ParseTuple(args, "s", &mask)) return 0;
	user_add_ircmask(user, mask);
	Return_None;
}

static PyObject *DelMask(PyObject *self, PyObject *args) {
	const char *mask;
	CheckSelf

	if (!PyArg_ParseTuple(args, "s", &mask)) return 0;
	if (user_del_ircmask(user, mask)) {
		PyErr_Format(PyExc_RuntimeError, "Could not delete IRC mask %s from user %s: No such mask.", mask, user->handle);
		return 0;
	}
	Return_None;
}

static PyObject *Get(PyObject *self, PyObject *args) {
	const char *setting, *chan = 0;
	script_var_t retval;
	CheckSelf

	if (!PyArg_ParseTuple(args, "s|s", &setting, &chan)) return 0;
	if (user_get_setting(user, chan, setting, (char **) &retval.value)) {
		PyErr_Format(PyExc_RuntimeError, "Could not get setting %s from user %s.", setting, user->handle);
		return 0;
	}
	retval.type = SCRIPT_STRING;
	retval.len = -1;
	return c_to_python_var(&retval);
}

static PyObject *Set(PyObject *self, PyObject *args) {
	const char *setting, *value, *chan = 0;
	CheckSelf

	if (!PyArg_ParseTuple(args, "ss|s", &setting, &value, &chan)) return 0;
	user_set_setting(user, chan, setting, value);
	Return_None;
}

static PyObject *GetFlags(PyObject *self, PyObject *args) {
	const char *chan = 0;
	flags_t flags;
	char flagstr[64];
	script_var_t retval;
	CheckSelf

	if (!PyArg_ParseTuple(args, "|s", &chan)) return 0;

	if (user_get_flags(user, chan, &flags)) {
		PyErr_Format(PyExc_RuntimeError, "Could not get flags for user %s.", user->handle);
		return 0;
	}
	flag_to_str(&flags, flagstr);
	retval.type = SCRIPT_STRING;
	retval.value = flagstr;
	retval.len = -1;
	return c_to_python_var(&retval);
}

static PyObject *SetFlags(PyObject *self, PyObject *args) {
	const char *chan = 0, *flags;
	CheckSelf

	if (!PyArg_ParseTuple(args, "s|s", &flags, &chan)) return 0;

	user_set_flags_str(user, chan, flags);
	Return_None;
}

static PyObject *MatchFlags(PyObject *self, PyObject *args) {
	const char *chan = 0, *flags;
	flags_t flags_left, flags_right;
	int r = 0;
	CheckSelf

	if (!PyArg_ParseTuple(args, "s|s", &flags, &chan)) return 0;
	if (user_get_flags(user, chan, &flags_right)) {
		PyErr_Format(PyExc_RuntimeError, "Could not get flags for user %s.", user->handle);
		return 0;
	}
	flag_from_str(&flags_left, flags);
	r = flag_match_subset(&flags_left, &flags_right);
	return PyInt_FromLong(r);
}

static PyObject *MatchFlagsOr(PyObject *self, PyObject *args) {
	const char *chan = 0, *flags;
	flags_t flags_left, flags_right;
	int r = 0;
	CheckSelf

	if (!PyArg_ParseTuple(args, "s|s", &flags, &chan)) return 0;
	if (user_get_flags(user, chan, &flags_right)) {
		PyErr_Format(PyExc_RuntimeError, "Could not get flags for user %s.", user->handle);
		return 0;
	}
	flag_from_str(&flags_left, flags);
	r = flag_match_partial(&flags_left, &flags_right);
	return PyInt_FromLong(r);
}

static PyObject *HasPass(PyObject *self, PyObject *args) {
	CheckSelf

	return PyInt_FromLong(user_has_pass(user));
}

static PyObject *CheckPass(PyObject *self, PyObject *args) {
	const char *pass;
	CheckSelf

	if (!PyArg_ParseTuple(args, "s", &pass)) return 0;
	return PyInt_FromLong(user_check_pass(user, pass));
}

static PyObject *SetPass(PyObject *self, PyObject *args) {
	const char *pass = 0;
	CheckSelf

	if (!PyArg_ParseTuple(args, "|s", &pass)) return 0;
	user_set_pass(user, pass);
	Return_None;
}

static PyMethodDef Methods[] = {
	{"delete", Delete, METH_NOARGS, "This method will delete this user and all his data. "
	                                "If he is currently on the partyline he will be booted. "
	                                "Useing this object or any other object referring to the same user "
	                                "after calling this method will raise an exception."},
	{"addmask", AddMask, METH_VARARGS, "Adds an IRC hostmask to this user."},
	{"delmask", DelMask, METH_VARARGS, "Deletes an IRC hostmask from this user."},
	{"get", Get, METH_VARARGS, ""},
	{"set", Set, METH_VARARGS, ""},
	{"getflags", GetFlags, METH_VARARGS, ""},
	{"setflags", SetFlags, METH_VARARGS, ""},
	{"matchflags", MatchFlags, METH_VARARGS, ""},
	{"matchflags_or", MatchFlagsOr, METH_VARARGS, ""},
	{"haspass", HasPass, METH_NOARGS, ""},
	{"checkpass", CheckPass, METH_VARARGS, ""},
	{"setpass", SetPass, METH_VARARGS, ""},
	{0}
};

static PyGetSetDef GetSeter[] = {
	{"uid", GetUid, 0, 0, 0},
	{"handle", GetHandle, 0, 0, 0},
	{"ircmasks", GetIrcMasks, 0, 0, 0},
	{0}
};

PyTypeObject Egguser_Type = {
	PyObject_HEAD_INIT(0)        /* required by law */
	0,                           /* ob_size, obsolete, always 0 */
	"eggdrop.egguser",           /* tp_name, type callable in module eggdrop */
	sizeof(EgguserObject),       /* tp_basicsize, size of the C struct belonging to this type */
	0,                           /* tp_itemsize, only for variable types, always 0 */ 
	0,                           /* tp_dealloc, the destructor, we don't need one */
	0,                           /* tp_print, don't write me into a file */
	0,                           /* tp_getattr, deprecated, we'll use tp_getattro */
	0,                           /* tp_setattr, deprecated, we'll use tp_setattro */
	Compare,                     /* tp_compare */
	Repr,                        /* tp_repr, should there be one */
	0,                           /* tp_as_number*/
	0,                           /* tp_as_sequence*/
	0,                           /* tp_as_mapping*/
	Hash,                           /* tp_hash */
	0,                           /* tp_call */
	0,                           /* tp_str */
	0,                           /* tp_getattro */
	0,                           /* tp_setattro */
	0,                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
	"an eggdrop user",           /* tp_doc */
	0,                           /* tp_traverse */
	0,                           /* tp_clear */
	0,                           /* tp_richcompare */
	0,                           /* tp_weaklistoffset */
	0,                           /* tp_iter */
	0,                           /* tp_iternext */
	Methods,                     /* tp_methods */
	0,                           /* tp_members */
	GetSeter,                    /* tp_getset */
	0,                           /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	0,                           /* tp_init */
	0,                           /* tp_alloc */
	Egguser_New                  /* tp_new */
};
