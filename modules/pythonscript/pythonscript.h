/* pythonscript.h: header for the pythonscript module
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: pythonscript.h,v 1.3 2006/01/05 20:42:42 sven Exp $
 */

#define PYTHON_VAR 1
#define PYTHON_FUNC 2

#define Callable_Check(op) PyObject_TypeCheck(op, &Callable_Type)
#define Egguser_Check(op) PyObject_TypeCheck(op, &Egguser_Type)

#define FlushAll() do {\
  Flush(0);\
  Flush(1);\
} while (0)

typedef struct {
	PyObject_HEAD
	int type;
	void *client_data;
} CallableObject;

typedef struct {
	PyObject_HEAD
	user_t *user;
} EgguserObject;

typedef struct {
	PyDictObject dict;
} MyDictObject;

partymember_t *LogTarget;

script_module_t my_script_interface;

void Flush(unsigned Target);
PyObject *c_to_python_var(script_var_t *v);
int python_to_c_var(PyObject *obj, script_var_t *var, int type);
int MyModule_Init(void);
PyObject *GetVar(script_linked_var_t *var);
int SetVar(script_linked_var_t *var, PyObject *Value);
PyObject *MyModule_Add(char *name, char *doc);
PyObject *MyDict_New(PyTypeObject *Type, PyObject *args, PyObject *kwds);

PyTypeObject Callable_Type, MyDict_Type, Stdio_Type, Egguser_Type;
PyObject *EggdropModule;
