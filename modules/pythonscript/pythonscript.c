/* pythonscript.c: Python scripting support
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
static const char rcsid[] = "$Id: pythonscript.c,v 1.2 2005/12/17 01:25:06 sven Exp $";
#endif

#include <Python.h>
#include <structmember.h>


#include <eggdrop/eggdrop.h>

#include "pythonscript.h"

/* Implementation of the script module interface. */
static int my_load_script(void *ignore, char *fname);
static int my_link_var(void *ignore, script_linked_var_t *var);
static int my_unlink_var(void *ignore, script_linked_var_t *var);
static int my_create_command(void *ignore, script_raw_command_t *info);
static int my_delete_command(void *ignore, script_raw_command_t *info);
static int my_get_arg(void *ignore, script_args_t *args, int num, script_var_t *var, int type);

script_module_t my_script_interface = {
	"Python", NULL,
	my_load_script,
	my_link_var, my_unlink_var,
	my_create_command, my_delete_command,
	my_get_arg
};

static PyObject *SysPath, *CommonModule, *PartylineModule, *PartylineDict, *RealHelper, *SysMod;
static PyObject *MinInt, *MaxInt, *MinUInt, *MaxUInt;

/* Load a python script. */
static int my_load_script(void *ignore, char *fname)
{
	int len;
	char *fullname, *name, *path;
	PyObject *PathString, *CommonDict, *NewMod;

	/* Check the filename and make sure it ends in .py */
	len = strlen(fname);
	if (len < 3 || fname[len-1] != 'y' || fname[len-2] != 'p' || fname[len-3] != '.') {
		/* Nope, let someone else load it. */
		return SCRIPT_ERR_NOT_RESPONSIBLE;
	}
	fullname = strdup(fname);
	name = rindex(fullname, '/');
	if (!name) {
		name = fullname;
		path = "";
	} else {
		*name++ = 0;
		path = fullname;
	}
	name[strlen(name) - 3] = 0;

	PathString = PyString_FromString(path);
	PyList_Insert(SysPath, 0, PathString);
	Py_DECREF(PathString);
	if (!(NewMod = PyImport_ImportModule(name))) {
		PyErr_Print();
		FlushAll();
		free(fullname);
		return SCRIPT_ERR_CODE;
	}

	CommonDict = PyObject_GetAttrString(CommonModule, name);
	if (!CommonDict) {
		CommonDict = PyDict_New();
		PyModule_AddObject(CommonModule, name, CommonDict);   /* CommonModule steals a reference to CommonDict */
	} else {
		Py_DECREF(CommonDict);
	}
	PyDict_SetItemString(CommonDict, "module", NewMod);
	/* XXX: We just leaked a reference to the new module.
	 * I guess we should save this to do something with it at
	 * a later time but i don't have a solid idea, what ... */
	PyList_SetSlice(SysPath, 0, 0, 0);
	free(fullname);
	return SCRIPT_OK;
}

/* This function creates the Python <-> C variable linkage on reads, writes, and dels. */
static int my_link_var(void *ignore, script_linked_var_t *var) {
	CallableObject *O;

	O = PyObject_New(CallableObject, &Callable_Type);       /* new reference */
	if (!O) {
		PyErr_Print();
		FlushAll();
		return -1;
	}

	O->type = PYTHON_VAR;
	O->client_data = var;
	PyModule_AddObject(EggdropModule, var->name, (PyObject *) O); /* EggdropModule steals a reference to O */
	return 0;
}

/* This function deletes Python <-> C variable links. */
static int my_unlink_var(void *ignore, script_linked_var_t *var) {
	PyObject *Dict = *_PyObject_GetDictPtr(EggdropModule);
	CallableObject *Callable;

	Callable = (CallableObject *) PyDict_GetItemString(Dict, var->name); /* Warning! Borrowed reference */
	if (!Callable) return 0;                   /* It never existed, that's the same as deleting it */
	Callable->client_data = 0;
	PyDict_DelItemString(Dict, var->name);
	/* at this point the Callable object might (should!) be gone. Don't try to access it */
	return 0;
}

/* When you use a script_callback_t's callback() function, this gets executed.
	It converts the C variables to Python variables and executes the Python script. */
static int my_python_callbacker(script_callback_t *me, ...)
{
	script_var_t var;
	int i, n, retval;
	PyObject *cmd, *param, *arg, *RetObj;
	va_list va;

	cmd = me->callback_data;

	if (me->syntax) n = strlen(me->syntax);
	else n = 0;

	if (n) param = PyTuple_New(n);
	else param = 0;

	va_start(va, me);
	for (i = 0; i < n; i++) {
		var.type = me->syntax[i];
		if (var.type == SCRIPT_UNSIGNED || var.type == SCRIPT_INTEGER) var.value = (void *) (va_arg(va, int));
		else var.value = va_arg(va, void *);
		var.len = -1;
		arg = c_to_python_var(&var);       /* New reference */
		if (!arg) {
			Py_DECREF(param);
			PyErr_Print();
			FlushAll();
			return 0;
		}
		PyTuple_SET_ITEM(param, i, arg);   /* param steals the reference from arg */
	}
	va_end(va);

	/* If it's a one-time callback, delete it. */
	if (me->flags & SCRIPT_CALLBACK_ONCE) me->del(me);

	RetObj = PyObject_CallObject(cmd, param);
	FlushAll();

	if (!RetObj) {
		PyErr_Print();
		FlushAll();
		return 0;
	}
	if (PyInt_Check(RetObj)) retval = PyInt_AS_LONG(RetObj);
	else retval = !PyObject_Not(RetObj);
	
	Py_DECREF(RetObj);

	return retval;
}

/* This implements the delete() member of Python script callbacks. */
static int my_python_cb_delete(script_callback_t *me)
{
	PyObject *Callback = me->callback_data;

	Py_DECREF(Callback);
	if (me->syntax) free(me->syntax);
	if (me->name) free(me->name);
	free(me);
	return 0;
}

/* Create Python commands with a C callback. */
static int my_create_command(void *ignore, script_raw_command_t *info) {
	CallableObject *O = PyObject_New(CallableObject, &Callable_Type);

	O->type = PYTHON_FUNC;
	O->client_data = info;
	if (PyModule_AddObject(EggdropModule, info->name, (PyObject *)O)) {
		PyErr_Print();
		FlushAll();
		return 1;
	}
	return 0;
}

/* Delete Tcl commands. */
static int my_delete_command(void *ignore, script_raw_command_t *info)
{
	PyObject *Dict = *_PyObject_GetDictPtr(EggdropModule);
	CallableObject *Callable;

	Callable = (CallableObject *) PyDict_GetItemString(Dict, info->name);
	                         /* Warning! Borrowed reference */
	if (!Callable) return 0;                   /* It never existed, that's the same as deleting it */
	Callable->client_data = 0;
	PyDict_DelItemString(Dict, info->name);
	return 0;
}

/* Convert a C variable to a Python variable.
 * Returns: Borrowed reference */
PyObject *c_to_python_var(script_var_t *v) {
	PyObject *result;

	result = NULL;
	/* If it's an array, we call ourselves recursively. */
	if (v->type & SCRIPT_ARRAY) {
		PyObject *element;
		int i;

		result = PyTuple_New(v->len);
		/* If it's an array of script_var_t's, then it's easy. */
		if ((v->type & SCRIPT_TYPE_MASK) == SCRIPT_VAR) {
			script_var_t **v_list;

			v_list = (script_var_t **)v->value;
			for (i = 0; i < v->len; i++) {
				element = c_to_python_var(v_list[i]);
				if (!element) {
					Py_DECREF(result);
					return 0;
				}
				PyTuple_SET_ITEM(result, i, element);
			}
		}
		else {
			/* Otherwise, we have to turn them into fake script_var_t's. */
			script_var_t v_sub;
			void **values;

			values = (void **)v->value;
			for (i = 0; i < v->len; i++) {
				v_sub.type = v->type & (~SCRIPT_ARRAY);
				v_sub.value = values[i];
				v_sub.len = -1;
				element = c_to_python_var(&v_sub);
				if (!element) {
					Py_DECREF(result);
					return 0;
				}
				PyTuple_SET_ITEM(result, i, element);
			}
		}
		/* Whew */
		if (v->type & SCRIPT_FREE) free(v->value);
		if (v->type & SCRIPT_FREE_VAR) free(v);
		return result;
	}

	/* Here is where we handle the basic types. */
	switch (v->type & SCRIPT_TYPE_MASK) {
		case SCRIPT_INTEGER:
			result = PyInt_FromLong((int) v->value);
			break;
		case SCRIPT_UNSIGNED:
			/* Python has no "unsigned" type. A Python "int" is a C "long".
			 * So, if sizeof(int) == sizeof(long) like on IA32 a Python "int"
			 * might be too short to store a C "unsigned".
			 * Use a Python "long" in that case */
			if ((unsigned) v->value <= PyInt_GetMax()) {
				result = PyInt_FromLong((unsigned) v->value);
			} else {
				result = PyLong_FromUnsignedLong((unsigned) v->value);
			}
			break;
		case SCRIPT_STRING: {
			char *str = v->value;

			if (!str) str = "";
			if (v->len == -1) v->len = strlen(str);
			result = PyString_FromStringAndSize(str, v->len);
			if (v->value && v->type & SCRIPT_FREE) free(v->value);
			break;
		}
		case SCRIPT_STRING_LIST: {
			char **str = v->value;
			PyObject *element;

			result = PyList_New(0);
			for (str = v->value; str && *str; ++str) {
				element = PyString_FromString(*str);
				PyList_Append(result, element);
			}
			break;
		}
		case SCRIPT_BYTES: {
			byte_array_t *bytes = v->value;
			result = PyString_FromStringAndSize(bytes->bytes, bytes->len);
			if (bytes->do_free) free(bytes->bytes);
			if (v->type & SCRIPT_FREE) free(bytes);
			break;
		}
#if 0
		case SCRIPT_POINTER: {
			char str[32];

			sprintf(str, "#%u", (unsigned int) v->value);
			result = Tcl_NewStringObj(str, -1);
			break;
		}
		case SCRIPT_PARTIER: {
			partymember_t *p = v->value;
			int pid;

			if (p) pid = p->pid;
			else pid = -1;
			result = Tcl_NewIntObj(pid);
			break;
		}
#endif
		case SCRIPT_USER: {
			/* An eggdrop user record (struct userrec *). */
			user_t *u = v->value;

			if (u) {
				result = PyInt_FromLong(u->uid);
			} else {
				Py_INCREF(Py_None);
				result = Py_None;
			}
			break;
		}
		default:
			/* Default: just "None". */
			Py_INCREF(Py_None);
			result = Py_None;
	}
	if (v->type & SCRIPT_FREE_VAR) free(v);
	return result;
}

/* Here we convert Python variables into C variables.
 * Don't set exceptions here, it will be done later with a better error message.
 * When we return, var->type may have the SCRIPT_FREE bit set, in which case
 * you should free(var->value) when you're done with it. */
int python_to_c_var(PyObject *obj, script_var_t *var, int type) {
	var->type = type;
	var->len = -1;
	var->value = 0;

	switch (type & SCRIPT_TYPE_MASK) {
		case SCRIPT_STRING: {
			char *Str;

			Str = PyString_AsString(obj);
			if (!Str) return 1;
			var->value = strdup(Str);
			var->len = strlen(var->value);
			return 0;
		}
		case SCRIPT_CALLBACK: {
			script_callback_t *cback; /* Callback struct */
			PyObject *FunctionName, *ModuleName;
			char *CFuncName = 0, *CModName = 0;

			if (!PyCallable_Check(obj)) return 1;
			cback = malloc(sizeof(*cback));
			cback->callback = (Function) my_python_callbacker;
			cback->callback_data = obj;
			Py_INCREF(obj);

			ModuleName = PyObject_GetAttrString(obj, "__module__");
			FunctionName = PyObject_GetAttrString(obj, "__name__");
			if (ModuleName) CModName = PyString_AsString(ModuleName);
			if (FunctionName) CFuncName = PyString_AsString(FunctionName);
			if (!CModName) CModName = "python";
			if (CFuncName) {
				cback->name = malloc(strlen(CModName) + strlen(CFuncName) + 2);
				sprintf(cback->name, "%s.%s", CModName, CFuncName);
			} else {
				cback->name = malloc(strlen(CModName) + 12);
				sprintf(cback->name, "%s.0x%08x", CModName, ((int) obj) & 0xFFFFFFFF);
			}
				
			cback->del = my_python_cb_delete;
			cback->delete_data = 0;
			cback->syntax = 0;
			cback->flags = 0;

			var->value = cback;
			return 0;
		}
		case SCRIPT_BYTES: {
			byte_array_t *byte_array;

			byte_array = malloc(sizeof(*byte_array));

			var->value = byte_array;
			var->type |= SCRIPT_FREE;

			return PyString_AsStringAndSize(obj, (char **) &byte_array->bytes, &byte_array->len);
		}
		case SCRIPT_UNSIGNED:
		case SCRIPT_INTEGER: {
			if (!PyInt_Check(obj) && !PyLong_Check(obj)) {
				PyErr_SetString(PyExc_TypeError, "an integer is required");
				return 1;
			}
			PyErr_Clear();
			if ((((type & SCRIPT_TYPE_MASK) == SCRIPT_UNSIGNED) && (PyObject_Compare(obj, MinUInt) < 0 ||
			                                                        PyObject_Compare(obj, MaxUInt) > 0)) ||
			    (((type & SCRIPT_TYPE_MASK) == SCRIPT_INTEGER) && (PyObject_Compare(obj, MinInt) < 0 ||
			                                                       PyObject_Compare(obj, MaxInt) > 0))) {
				PyErr_SetString(PyExc_OverflowError, "Python value does not fit in C type");
				return 1;
			}

			if (PyInt_Check(obj)) var->value = (void *) PyInt_AsLong(obj);
			else var->value = (void *) PyLong_AsUnsignedLong(obj);
			return 0;
		}
#if 0
		case SCRIPT_PARTIER: {
			int pid = -1;

			err = Tcl_GetIntFromObj(myinterp, obj, &pid);
			if (!err) var->value = partymember_lookup_pid(pid);
			else var->value = NULL;
			break;
		}
#endif
		case SCRIPT_USER: {
			user_t *u;
			int uid;

			uid = PyInt_AsLong(obj);
			u = user_lookup_by_uid(uid);
			var->value = u;
			if (!u) {
				PyErr_SetString(PyExc_LookupError, "User not found");
				return 1;
			}
			return 0;
		}
	}
	return 1;
}

static int my_get_arg(void *ignore, script_args_t *args, int num, script_var_t *var, int type) {
	PyObject *ParameterTuple, *Parameter;

	ParameterTuple = args->client_data;
	Parameter = PyTuple_GetItem(ParameterTuple, num);
	if (!Parameter) {
		// XXX: write some kind of error log?
		PyErr_Clear();
		return 1;
	}

	return python_to_c_var(Parameter, var, type);
}

static int mls(partymember_t *p, char *nick, user_t *u, char *cmd, char *text) {
	LogTarget = p;
	my_load_script(0, text);
	LogTarget = 0;
	return 0;
}

static int party_python(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	PyObject *ret;

	if (!u || !egg_isowner(u->handle)) {
		partymember_write(p, _("You must be a permanent owner (defined in the config file) to use this command.\n"), -1);
		return BIND_RET_LOG;
	}

	if (!text) {
		partymember_write(p, _("Syntax: .python <pythonexpression>"), -1);
		return 0;
	}

	LogTarget = p;
	ret = PyRun_String(text, Py_single_input, PartylineDict, PartylineDict);
	if (!ret) PyErr_Print();
	FlushAll();
	LogTarget = 0;
	return 0;
}

static PyObject *Help(PyObject *self, PyObject *Args) {
	PyObject *O = 0, *Stdout, *Write, *DocStr, *HelpStr;

	PyArg_ParseTuple(Args, "|O", &O);
	if (!O || !Callable_Check(O)) return PyObject_CallObject(RealHelper, Args);
	Stdout = PyObject_GetAttrString(SysMod, "stdout");
	if (!Stdout || !(Write = PyObject_GetAttrString(Stdout, "write"))) return 0;
	Py_DECREF(Stdout);
	DocStr = PyObject_GetAttrString(O, "__doc__");
	if (!(HelpStr = Py_BuildValue("(O)", DocStr))) {
		Py_DECREF(Write);
		return 0;
	}
	Py_DECREF(DocStr);
	PyObject_CallObject(Write, HelpStr);
	Py_DECREF(HelpStr);
	FlushAll();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef HelpMethod = {
	"help", Help, METH_VARARGS, 0
};

static bind_list_t party_commands[] = {
	{"n", "python", (Function) party_python},
	{"n", "load", (Function) mls},
	{0}
};

static int pythonscript_close(int why)
{
	Py_Finalize();

	bind_rem_list("party", party_commands);

	script_unregister_module(&my_script_interface);
	return 0;
}

static PyMethodDef *methods = {
	0
};

EXPORT_SCOPE int pythonscript_LTX_start(egg_module_t *modinfo);

int pythonscript_LTX_start(egg_module_t *modinfo) {
	PyObject *builtinModule, *Stdout, *Stderr, *Para, *Helper;

	modinfo->name = "pythonscript";
	modinfo->author = "eggdev";
	modinfo->version = "0.0.1";
	modinfo->description = "provides python scripting support";
	modinfo->close_func = pythonscript_close;

	/* Create the interpreter and let tcl load its init.tcl */
	Py_Initialize();

	MyModule_Init();
	PyType_Ready(&MyDict_Type);
	PyType_Ready(&Callable_Type);
	PyType_Ready(&Stdio_Type);
	
	MinInt = PyInt_FromLong(INT_MIN);
	MaxInt = PyInt_FromLong(INT_MAX);
	MinUInt = PyInt_FromLong(0);
	MaxUInt = PyInt_FromLong(UINT_MAX);
	if (PyObject_Compare(MinUInt, MaxUInt) >= 0) MaxUInt = PyLong_FromUnsignedLong(UINT_MAX);

	SysMod = PyImport_ImportModule("sys");
	if (!SysMod || !(SysPath = PyObject_GetAttrString(SysMod, "path"))) {
		putlog(LOG_MISC, "*", "Module 'pythonscript' could not be initialized: Couldn't get sys.path");
		Py_Finalize();
		return -4;
	}

	EggdropModule = MyModule_Add("eggdrop", "this module contains the interface to the eggdrop bot");
	CommonModule = Py_InitModule3("common", methods, "this module is a kind of semipersistent storage space for scripts");
	PartylineModule = Py_InitModule3("partyline", methods, "this is the partyline mamespace");
	PartylineDict = PyModule_GetDict(PartylineModule);

	if (!EggdropModule || !CommonModule || !PartylineModule|| !PartylineDict) {
		putlog(LOG_MISC, "*", "Module 'pythonscript' could not be initialized: Failed to create modules");
		Py_Finalize();
		return -4;
	}

	Para = Py_BuildValue("(i)", 1);
	Stdout = Stdio_Type.tp_new(&Stdio_Type, Para, 0);
	Py_DECREF(Para);
	Para = Py_BuildValue("(i)", 2);
	Stderr = Stdio_Type.tp_new(&Stdio_Type, Para, 0);
	Py_DECREF(Para);
	PyModule_AddObject(SysMod, "stdout", Stdout);
	PyModule_AddObject(SysMod, "stderr", Stderr);
	
	builtinModule = PyImport_ImportModule("__builtin__");
	if (!builtinModule || PyModule_AddObject(PartylineModule, "__builtins__", builtinModule)) {
		putlog(LOG_MISC, "*", "Error inserting '__buildin__' module. builtin commands will not be available from the partyline");
	} else {
		Helper = PyCFunction_NewEx(&HelpMethod, 0, 0);
		RealHelper = PyObject_GetAttrString(builtinModule, "help");
		PyModule_AddObject(builtinModule, "help", Helper);
	}
	Py_XDECREF(builtinModule);
	PyErr_Clear();

	script_register_module(&my_script_interface);
	script_playback(&my_script_interface);

	bind_add_list("party", party_commands);

	return 0;
}

