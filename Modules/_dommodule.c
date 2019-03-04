#include "Paradisi.h"
#include "structmember.h"

// Module Docstring.
PyDoc_STRVAR(_dom_doc,
"This module contains the Paradisi implementation of the DOM structure.");

#define RAW_NODE 0 // This is non-standard; used for polymorphism
#define ELEMENT_NODE 1
#define ATTRIBUTE_NODE 2
#define TEXT_NODE 3
#define CDATA_SECTION_NODE 4
// ENTITY_REFERENCE_NODE = 5; // historical, no need to implement
// ENTITY_NODE = 6; // historical, no need to implement
#define PROCESSING_INSTRUCTION_NODE 7
#define COMMENT_NODE 8
#define DOCUMENT_NODE 9
#define DOCUMENT_TYPE_NODE 10
#define DOCUMENT_FRAGMENT_NODE 11
// NOTATION_NODE = 12; // historical, no need to implement


////////////////////////////////////////////////
////            'node' BASE CLASS           ////
////////////////////////////////////////////////

// This is the base class from which all nodes eventually inherit.
// It should not be instantiated directly, instead use one of the
// primary subtypes textNode and elementNode.
typedef struct {
	PyObject_HEAD
	PyListObject* children;
	PyObject* weakrefs;
	unsigned short nodeType;
	PyUnicodeObject* nodeName;
} node;

// This will help support cyclic garbage collection by defining a way to traverse the child node list
static int node_traverse(node* self, visitproc visit, void* arg) {
	Py_VISIT(self->children);
	return 0;
}

// Supports cyclic garbage collection via manual CLEAR on object destruction
static int node_clear(node* self) {
	Py_CLEAR(self->children);
	return 0;
}

// This declares node.__del__
static void node_dealloc(node* self) {
	PyObject_GC_UnTrack(self);
	node_clear(self);
	if (self->weakrefs != NULL) {
		PyObject_ClearWeakRefs((PyObject*) self);
	}
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

		self->nodeType = RAW_NODE
		self->nodeName = (PyUnicodeObject*) PyUnicode_FromString("RAW_NODE");
		if (self->nodeName == NULL) {
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
		PyErr_Format(PyExc_TypeError, "'children' of a node must be a list, not %.200s", value->ob_type->tp_name);
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
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	.tp_new = node_new,
	.tp_init = (initproc) node_init,
	.tp_dealloc = (destructor) node_dealloc,
	.tp_traverse = (traverseproc) node_traverse,
	.tp_clear = (inquiry) node_clear,
	.tp_getset = node_getsetters,
	.tp_weaklistoffset = offsetof(node, weakrefs)
};

// W3C standard appendChild method - appends a child to this node, checking
// first that the object to be added is actually a node
static PyObject* node_appendChild(PyObject* self, PyObject* args) {
	if (!PyObject_IsInstance(args, (PyObject*) &Node)) {
		PyErr_Format(PyExc_TypeError, "child must be a node object, not %.200s", args->ob_type->tp_name);
		return NULL;
	}
	Py_INCREF(args);
	PyList_Append((PyObject*)(((node*)(self))->children), args);
	Py_RETURN_NONE;
}

// Method definitions for a node object
static PyMethodDef node_methods[] = {
	{"appendChild", (PyCFunction) node_appendChild, METH_O, "Append a child node to this node's children"},
	{NULL}
};

// Readonly attributes that enumerates the type of the node
static PyMemberDef node_members[] = {{
	"nodeType",
	T_USHORT,
	offsetof(node, nodeType),
	READONLY,
	"The type of this node. (W3C-defined integer constant)"
},
{
	"nodeName",
	T_OBJECT,
	offsetof(node, nodeName),
	READONLY,
	"The name of the type of this node."
},
{NULL}};

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
	self->node.nodeType = TEXT_NODE;
	if (self != NULL) {
		self -> data = (PyUnicodeObject*) PyUnicode_FromString("");
		if (self->data == NULL) {
			Py_DECREF(self);
			return NULL;
		}
		self->node.nodeName = (PyUnicodeObject*) PyUnicode_FromString("#text");
		if (self->node.nodeName == NULL) {
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

// Renders the textNode as a string
static PyUnicodeObject* textNode_str(textNode* self) {
	return self->data;
}

// Representation of a textNode
static PyUnicodeObject* textNode_repr(textNode* self) {
	return (PyUnicodeObject*)(PyUnicode_Concat(PyUnicode_Concat(PyUnicode_FromString("#text: '"), (PyObject*)(self->data)), PyUnicode_FromString("'")));
}

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
	.tp_str = (reprfunc) textNode_str,
	.tp_repr = (reprfunc) textNode_repr,
	.tp_getset = textNode_getsetters,
	.tp_members = node_members
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
		self->node.nodeType = ELEMENT_NODE;
		self->node.nodeName = (PyUnicodeObject*) PyUnicode_FromString("");
		if (self->node.nodeName == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self -> tagName = (PyUnicodeObject*) PyUnicode_FromString("");
		if (self->tagName == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->node.nodeName = (PyUnicodeObject*) PyUnicode_FromString("");
		if (self->node.nodeName == NULL) {
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
	static char* kwlist[] = {"tagName", "attributes", NULL};
	PyObject* tagName = NULL;
	PyObject* attributes = NULL;
	PyObject* tmp;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!O!:elementNode()", kwlist, &PyUnicode_Type, &tagName, &PyDict_Type, &attributes)) {
		return -1;
	}

	if (tagName) {
		tmp = (PyObject*) self->tagName;
		Py_INCREF(tagName);
		self->tagName = (PyUnicodeObject*) tagName;
		Py_XDECREF(tmp);
		tmp = (PyObject*) self->node.nodeName;
		Py_INCREF(tagName);
		self->node.nodeName = (PyUnicodeObject*) tagName;
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
static PyDictObject* elementNode_getattrs(elementNode* self, void* unused_closure) {
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
		self->attributes = (PyDictObject*) PyDict_New();
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
	.tp_getset = elementNode_getsetters,
	.tp_members = node_members
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
	Node.tp_methods = node_methods;
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

	// Constants
	PyModule_AddIntConstant(m, "ELEMENT_NODE", ELEMENT_NODE);
	PyModule_AddIntConstant(m, "ATTRIBUTE_NODE", ATTRIBUTE_NODE);
	PyModule_AddIntConstant(m, "TEXT_NODE", TEXT_NODE);
	PyModule_AddIntConstant(m, "CDATA_SECTION_NODE", CDATA_SECTION_NODE);
	PyModule_AddIntConstant(m, "PROCESSING_INSTRUCTION_NODE", PROCESSING_INSTRUCTION_NODE);
	PyModule_AddIntConstant(m, "COMMENT_NODE", COMMENT_NODE);
	PyModule_AddIntConstant(m, "DOCUMENT_NODE", DOCUMENT_NODE);
	PyModule_AddIntConstant(m, "DOCUMENT_TYPE_NODE", DOCUMENT_TYPE_NODE);
	PyModule_AddIntConstant(m, "DOCUMENT_FRAGMENT_NODE", DOCUMENT_FRAGMENT_NODE);


	Py_INCREF(&Node);
	PyModule_AddObject(m, "node", (PyObject*) &Node);
	Py_INCREF(&TextNode);
	PyModule_AddObject(m, "textNode", (PyObject*) &TextNode);
	Py_INCREF(&ElementNode);
	PyModule_AddObject(m, "elementNode", (PyObject*) &ElementNode);
	return m;
}
