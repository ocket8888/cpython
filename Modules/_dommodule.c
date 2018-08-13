#include "Paradisi.h"
#include "structmember.h"

// Module Docstring.
PyDoc_STRVAR(_dom_doc,
"This module contains the Paradisi implementation of the DOM structure.");


////////////////////////////////////////////////
////            'node' BASE CLASS           ////
////////////////////////////////////////////////

// This is the base class from which all nodes eventually inherit.
// It should not be instantiated directly, instead use one of the
// primary subtypes textNode and elementNode.
typedef struct {
	PyObject_HEAD
	PyListObject* children;
} node;

// This declares node.__del__
static void node_dealloc(node* self) {
	Py_XDECREF(self->children);
	Py_TYPE(self)->tp_free((PyObject*) self);
}

// Generates a base node; node.__new__
static PyObject* node_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	node* self;
	self = (node*) type->tp_alloc(type, 0);
	if (self != NULL) {
		self->children = (PyListObject*) PyList_New(0);
		if (self->children == NULL) {
			Py_DECREF(self);
			return NULL;
		}
	}
	return (PyObject *) self;
}

// Initializer for a node with passed arguments; node.__init__
static int node_init(node* self, PyObject* args, PyObject* kwds) {
	static char* kwlist[] = {"children", NULL};
	PyObject* children = NULL;
	PyListObject* tmp;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!:node()", kwlist, &PyList_Type, &children))
		return -1;

	if (children) {
		tmp = self->children;
		Py_INCREF(children);
		self->children = (PyListObject*) children;
		Py_XDECREF(tmp);
	}
	return 0;
}

// Getter for node.children. Necessary just because a setter must be defined
static PyListObject* node_getchildren(node* self, void* unused_closure) {
	Py_INCREF(self->children);
	return self->children;
}

// Setter for node.children. This is used to ensure a node's children are always
// stored in a list - or some subclass thereof.
static int node_setchildren(node* self, PyObject* value, void* unused_closure) {
	PyListObject* tmp;
	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete the children attribute");
		return -1;
	}
	if (!PyList_Check(value)) {
		PyErr_Format(PyExc_TypeError, "'children' of a textNode must be a list, not %.200s", value->ob_type->tp_name);
		return -1;
	}
	tmp = self->children;
	Py_INCREF(value);
	self->children = (PyListObject*) value;
	Py_DECREF(tmp);
	return 0;
}

// Sets a node's attributes that have getters/setters.
static PyGetSetDef node_getsetters[] = {
	{"children", (getter) node_getchildren, (setter) node_setchildren,
	 "Child nodes of this node", NULL},
	{NULL}  /* Sentinel */
};

// Full definition of a node
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
	.tp_getset = node_getsetters,
	// .tp_methods = Custom_methods,
};


////////////////////////////////////////////////
////            'textNode' CLASS            ////
////////////////////////////////////////////////

// the _dom.textNode class inherits from _dom.node,
// with one additional member: data - which is a
// Paradisi `str` instance
typedef struct {
	node node;
	PyUnicodeObject* data;
} textNode;

// Deallocates the memory dedicated to this textNode's data.
static void textNode_dealloc(textNode* self) {
	Py_XDECREF(self->data);
	Py_TYPE(self)->tp_free((PyObject*) self);
}

// Allocates a textNode's data and sets it to an empty string.
static PyObject* textNode_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
	textNode* self;
	self = (textNode*) type->tp_alloc(type, 0);
	if (self != NULL) {
		self -> data = (PyUnicodeObject*) PyUnicode_FromString("");
		if (self->data == NULL) {
			Py_DECREF(self);
			return NULL;
		}
		self->node.children = (PyListObject*) PyList_New(0);
		if (self->node.children == NULL) {
			Py_DECREF(self);
			return NULL;
		}
	}
	return (PyObject*) self;
}

// Initializes the data of a text node.
static int textNode_init(textNode* self, PyObject* args, PyObject* kwargs) {
	static char* kwlist[] = {"data", NULL};
	PyObject* data = NULL;
	PyUnicodeObject* tmp;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!:textNode", kwlist, &PyUnicode_Type, &data)) {
		return -1;
	}

	if (data) {
		tmp = self->data;
		Py_INCREF(data);
		self->data = (PyUnicodeObject*) data;
		Py_XDECREF(tmp);
	}
	return 0;
}

// function to retrieve a reference to the node's data
static PyUnicodeObject* textNode_getdata(textNode* self, void* unused_closure) {
	Py_INCREF(self->data);
	return self->data;
}

// Sets the node's data to the value provided. Does NOT perform string coercion
// (instead raises TypeError), but WILL set the data to an empty string if
// `del <textNode instance>.data` is attempted
static int textNode_setdata(textNode* self, PyObject* value, void* unused_closure) {
	PyUnicodeObject* tmp;
	if (value == NULL) {
		// Rather than allow setting to None (or deletion), we set the text value to
		// an empty string.
		value = PyUnicode_FromString("");
	}

	else if (!PyUnicode_Check(value)) {
		PyErr_Format(PyExc_TypeError, "'data' of a textNode must be string, not %.200s", value->ob_type->tp_name);
		return -1;
	}

	tmp = self-> data;
	Py_INCREF(value);
	self->data = (PyUnicodeObject*) value;
	Py_DECREF(tmp);
	return 0;
}

// Definition of the getters and setters for the node's attributes
static PyGetSetDef textNode_getsetters[] = {
	{"data", (getter) textNode_getdata, (setter) textNode_setdata, "The textual data contained in this node.", NULL},
	{NULL} /* Sentinel */
};

// Full definition of a textNode
static PyTypeObject TextNode = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "_dom.textNode",
	.tp_doc = "A node that holds only plaintext",
	.tp_basicsize = sizeof(textNode),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DEFAULT,
	.tp_new = textNode_new,
	.tp_init = (initproc) textNode_init,
	.tp_dealloc = (destructor) textNode_dealloc,
	.tp_getset = textNode_getsetters
};

////////////////////////////////////////////////
////          'elementNode' CLASS           ////
////////////////////////////////////////////////

// This structure inherits from `node`, with two
// additional attributes: attributes - a `dict` of attributes and values
//                        tagName   - the name of the tag, as a `str`
typedef struct {
	node node;
	PyDictObject* attributes;
	PyUnicodeObject* tagName;
} elementNode;

// Deallocates the memory dedicated to this node.
static void elementNode_dealloc(elementNode* self) {
	Py_XDECREF(self->attributes);
	Py_XDECREF(self->tagName);
	Py_TYPE(self)->tp_free((PyObject*) self);
}

// Allocates an elementNode's data and sets it's tag to an empty string
// and its attributes to an empty dict.
static PyObject* elementNode_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
	elementNode* self;
	self = (elementNode*) type->tp_alloc(type, 0);
	if (self != NULL) {
		self -> tagName = (PyUnicodeObject*) PyUnicode_FromString("");
		if (self->tagName == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self -> attributes = (PyDictObject*) PyDict_New();
		if (self->attributes == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self -> node.children = (PyListObject*) PyList_New(0);
		if (self->node.children == NULL) {
			Py_DECREF(self);
			return NULL;
		}
	}
	return (PyObject*) self;
}

// Initializes the tag name and attributes of an elementNode
static int elementNode_init(elementNode* self, PyObject* args, PyObject* kwargs) {
	static char* kwlist[] = {"tagName", "attributes"};
	PyObject* tagName = NULL;
	PyObject* attributes = NULL;
	PyObject* tmp;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO:elementNode()", kwlist, &tagName, &attributes)) {
		return -1;
	}

	if (tagName) {
		tmp = (PyObject*) self->tagName;
		Py_INCREF(tagName);
		self->tagName = (PyUnicodeObject*) tagName;
		Py_XDECREF(tmp);
	}

	if (attributes) {
		tmp = (PyObject*) self->attributes;
		Py_INCREF(attributes);
		self->attributes = (PyDictObject*) attributes;
		Py_XDECREF(tmp);
	}
	return 0;
}

// function to retrieve a reference to the element's tagname
static PyUnicodeObject* elementNode_gettagName(elementNode* self, void* unused_closure) {
	Py_INCREF(self->tagName);
	return self->tagName;
}

// Sets the node's tagname to the value provided. Does NOT perform string coercion
// (instead raises TypeError), but WILL set the tagname to an empty string if
// `del <elementNode instance>.tagName` is attempted
static int elementNode_settagName(elementNode* self, PyObject* value, void* unused_closure) {
	PyUnicodeObject* tmp;
	if (value == NULL) {
		// Rather than allow setting to None (or deletion), we set the text value to
		// an empty string.
		value = PyUnicode_FromString("");
	}

	else if (!PyUnicode_Check(value)) {
		PyErr_Format(PyExc_TypeError, "'tagName' of an elementNode must be string, not %.200s", value->ob_type->tp_name);
		return -1;
	}

	tmp = self-> tagName;
	Py_INCREF(value);
	self->tagName = (PyUnicodeObject*) value;
	Py_DECREF(tmp);
	return 0;
}

// function to retrieve a reference to the element's attributes
static PyUnicodeObject* elementNode_getattrs(elementNode* self, void* unused_closure) {
	Py_INCREF(self->attributes);
	return self->attributes;
}

// Sets the node's attributes to the value provided. Does NOT perform dict coercion
// (instead raises TypeError), but WILL set the tagname to an empty dict if
// `del <elementNode instance>.attributes` is attempted
static int elementNode_setattrs(elementNode* self, PyObject* value, void* unused_closure) {
	PyDictObject* tmp;
	if (value == NULL) {
		// Rather than allowing deletion, we set the `attributes` value to
		// an empty dict.
		tmp = self->attributes;
		self->attributes = PyDict_New();
		Py_DECREF(tmp);
		return 0;
	}

	if (!PyDict_Check(value)) {
		PyErr_Format(PyExc_TypeError, "'attributes' of an elementNode must be a dictionary, not %.200s", value->ob_type->tp_name);
		return -1;
	}

	tmp = self-> attributes;
	Py_INCREF(value);
	self->attributes = (PyDictObject*) value;
	Py_DECREF(tmp);
	return 0;
}

// Definition of the getters and setters for the node's attributes
static PyGetSetDef elementNode_getsetters[] = {
	{"tagName", (getter) elementNode_gettagName, (setter) elementNode_settagName, "The name of this element's tag.", NULL},
	{"attributes", (getter) elementNode_getattrs, (setter) elementNode_setattrs, "A dictionary that maps the element's attributes to their values.", NULL},
	{NULL} /* Sentinel */
};

static PyTypeObject ElementNode = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "_dom.elementNode",
	.tp_doc = "A node that represents an element of some kind.",
	.tp_basicsize = sizeof(elementNode),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DEFAULT,
	.tp_new = elementNode_new,
	.tp_init = (initproc) elementNode_init,
	.tp_dealloc = (destructor) elementNode_dealloc,
	.tp_getset = elementNode_getsetters
};

////////////////////////////////////////////////
////              '_dom' MODULE             ////
////////////////////////////////////////////////

static struct PyModuleDef _dommodule = {
	PyModuleDef_HEAD_INIT,
	"_dom",   /* name of module */
	_dom_doc, /* module documentation, may be NULL */
	-1,       /* size of per-interpreter state of the module,
				 or -1 if the module keeps state in global variables. */
};

PyMODINIT_FUNC
PyInit__dom(void)
{
	PyObject* m;
	if (PyType_Ready(&Node) < 0) {
		return NULL;
	}

	TextNode.tp_base = &Node;
	if (PyType_Ready(&TextNode) < 0) {
		return NULL;
	}

	ElementNode.tp_base = &Node;
	if (PyType_Ready(&ElementNode) < 0) {
		return NULL;
	}

	m = PyModule_Create(&_dommodule);
	if (m == NULL)
		return NULL;

	Py_INCREF(&Node);
	PyModule_AddObject(m, "node", (PyObject*) &Node);
	Py_INCREF(&TextNode);
	PyModule_AddObject(m, "textNode", (PyObject*) &TextNode);
	Py_INCREF(&ElementNode);
	PyModule_AddObject(m, "elementNode", (PyObject*) &ElementNode);
	return m;
}
