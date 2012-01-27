#include <Python.h>

extern "C" {
#define new new_ /* cheap workaround for <= 2.20.1 */
#include <libmount.h>
#undef new
};

typedef struct {
	PyObject_HEAD

	struct libmnt_context *mnt_context;
} BootMountpoint;

static void BootMountpoint_dealloc(PyObject *o);
static PyObject *BootMountpoint_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyTypeObject BootMountpointType = {
	PyObject_HEAD_INIT(NULL)
	0, /* ob_size */
	"pymountboot.BootMountpoint", /* tp_name */
	sizeof(BootMountpoint), /* tp_basicsize */
	0, /* tp_itemsize */
	BootMountpoint_dealloc, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_compare */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	0, /* tp_hash  */
	0, /* tp_call */
	0, /* tp_str */
	0, /* tp_getattro */
	0, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"/boot mount point", /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	0, /* tp_methods */
	0, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	0, /* tp_init */
	0, /* tp_alloc */
	BootMountpoint_new /* tp_new */
};

static PyMethodDef pymountboot_methods[] = {
	{ NULL }
};

#ifndef PyMODINIT_FUNC /* declarations for DLL import/export */
#	define PyMODINIT_FUNC void
#endif

extern "C" {
	PyMODINIT_FUNC initpymountboot(void);
};

PyMODINIT_FUNC initpymountboot(void) {
	PyObject *m;

	if (PyType_Ready(&BootMountpointType) < 0)
		return;

	m = Py_InitModule3("pymountboot", pymountboot_methods,
		"Module used to handle /boot auto(re)mounting.");

	Py_INCREF(&BootMountpointType);
	PyModule_AddObject(m, "BootMountpoint", (PyObject *)&BootMountpointType);
}

static void BootMountpoint_dealloc(PyObject *o) {
	BootMountpoint *b = reinterpret_cast<BootMountpoint*>(o);

	mnt_free_context(b->mnt_context);
}

static PyObject *BootMountpoint_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyObject *self;
	struct libmnt_context *ctx = mnt_new_context();

	if (!ctx) {
		PyErr_SetString(PyExc_RuntimeError, "unable to create libmount context");
		return NULL;
	}

	self = type->tp_alloc(type, 0);

	if (self != NULL) {
		BootMountpoint *b = reinterpret_cast<BootMountpoint*>(self);

		b->mnt_context = ctx;
	} else
		mnt_free_context(ctx);

	return self;
}
