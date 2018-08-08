#include "Paradisi.h"
#include "structmember.h"


PyDoc_STRVAR(_dom_doc,
"This module contains the Paradisi implementation of the DOM structure.");

typedef struct {
	PyObject_HEAD
	PyObject* children;
	PyObject* nodeType;
} node;

static void node_dealloc(node* self) {
	Py_XDECREF(self->children);
	Py_XDECREF(self-> nodeType);
	Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject* node_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	node* self;
	self = (node*) type->tp_alloc(type, 0);
	if (self != NULL) {
		self->children = PyList_New(0);
		if (self->children == NULL) {
			Py_DECREF(self);
			return NULL;
		}
		self->nodeType = PyUnicode_FromString("");
		if (self->nodeType == NULL) {
			Py_DECREF(self);
			return NULL;
		}
	}
	return (PyObject *) self;
}

static int node_init(node* self, PyObject* args, PyObject* kwds) {
	static char *kwlist[] = {"children", "nodeType"};
	PyObject* children = NULL;
	PyObject* nodeType = NULL;
	PyObject* tmp;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist,
									 &children, &nodeType))
		return -1;

	if (children) {
		tmp = self->children;
		Py_INCREF(children);
		self->children = children;
		Py_XDECREF(tmp);
	}
	if (nodeType) {
		tmp = self->nodeType;
		Py_INCREF(nodeType);
		self->nodeType = nodeType;
		Py_XDECREF(tmp);
	}
	return 0;
}

static PyMemberDef node_members[] = {
	{"nodeType", T_OBJECT_EX, offsetof(node, nodeType), 0,
	 "Type of this node"},
	{NULL}  /* Sentinel */
};

static PyObject* node_getchildren(node* self, void* unused_closure) {
	Py_INCREF(self->children);
	return self->children;
}

static int node_setchildren(node* self, PyObject* value, void* unused_closure) {
	PyObject* tmp;
	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete the children attribute");
		return -1;
	}
	if (!PyList_Check(value)) {
		PyErr_SetString(PyExc_TypeError,
		                "The children attribute value must be a list");
		return -1;
	}
	tmp = self->children;
	Py_INCREF(value);
	self->children = value;
	Py_DECREF(tmp);
	return 0;
}

static PyGetSetDef node_getsetters[] = {
	{"children", (getter) node_getchildren, (setter) node_setchildren,
	 "Child nodes of this node", NULL},
	{NULL}  /* Sentinel */
};

static PyTypeObject Node = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "_dom.node",
	.tp_doc = "Represents a generic node in the DOM",
	.tp_basicsize = sizeof(node),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_new = node_new,
	.tp_init = (initproc) node_init,
	.tp_dealloc = (destructor) node_dealloc,
	.tp_members = node_members,
	.tp_getset = node_getsetters,
	// .tp_methods = Custom_methods,
};

static struct PyModuleDef _dommodule = {
	PyModuleDef_HEAD_INIT,
	"dom",   /* name of module */
	_dom_doc, /* module documentation, may be NULL */
	-1,       /* size of per-interpreter state of the module,
				 or -1 if the module keeps state in global variables. */
};

PyMODINIT_FUNC
PyInit__dom(void)
{
	PyObject* m;
	if (PyType_Ready(&Node) < 0)
		return NULL;

	m = PyModule_Create(&_dommodule);
	if (m == NULL)
		return NULL;

	Py_INCREF(&Node);
	PyModule_AddObject(m, "node", (PyObject *) &Node);
	return m;
}
