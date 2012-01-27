#include <Python.h>

#include <stdexcept>

extern "C" {
#define new new_ /* cheap workaround for <= 2.20.1 */
#include <libmount.h>
#undef new
};

enum mountpoint_status {
	MOUNTPOINT_NONE,
	MOUNTPOINT_MOUNTED,
	MOUNTPOINT_REMOUNTED_RW
};

typedef struct {
	PyObject_HEAD

	struct libmnt_context *mnt_context;
	enum mountpoint_status status;
} BootMountpoint;

static void BootMountpoint_dealloc(PyObject *o);
static PyObject *BootMountpoint_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static PyObject *BootMountpoint_mount(PyObject *args, PyObject *kwds);
static PyObject *BootMountpoint_umount(PyObject *args, PyObject *kwds);

static PyMethodDef BootMountpoint_methods[] = {
	{ "mount", BootMountpoint_mount, METH_NOARGS,
		"Mount (or remount r/w) /boot if necessary" },
	{ "umount", BootMountpoint_umount, METH_NOARGS,
		"Unmount (or remount r/o) previously mounted /boot" },
	{ NULL }
};

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
	BootMountpoint_methods, /* tp_methods */
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

	if (b->status != MOUNTPOINT_NONE) /* clean up! */
		BootMountpoint_umount(o, NULL);

	mnt_free_context(b->mnt_context);
}

static PyObject *BootMountpoint_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	try {
		struct libmnt_context *ctx = mnt_new_context();

		if (!ctx)
			throw std::runtime_error("unable to create libmount context");

		try {
			if (mnt_context_set_target(ctx, "/boot"))
				throw std::runtime_error("unable to set mountpoint to /boot");

			PyObject *self = type->tp_alloc(type, 0);

			if (self != NULL) {
				BootMountpoint *b = reinterpret_cast<BootMountpoint*>(self);

				b->mnt_context = ctx;
				b->status = MOUNTPOINT_NONE;
			}

			return self;
		} catch (std::exception e) {
			mnt_free_context(ctx);
			throw e;
		}
	} catch (std::runtime_error err) {
		PyErr_SetString(PyExc_RuntimeError, err.what());
		return NULL;
	} catch (std::exception e) {
		PyErr_SetString(PyExc_Exception, e.what());
		return NULL;
	}
}

static PyObject *BootMountpoint_mount(PyObject *args, PyObject *kwds) {
	try {
		BootMountpoint *self = reinterpret_cast<BootMountpoint*>(args);
		struct libmnt_context* const ctx = self->mnt_context;

		struct libmnt_table *mtab;
		if (mnt_context_get_mtab(ctx, &mtab))
			throw std::runtime_error("unable to get mtab");

		/* backward -> find the top-most mount */
		struct libmnt_fs *fs = mnt_table_find_target(mtab, "/boot",
				MNT_ITER_BACKWARD);

		if (!fs) { /* boot not mounted, mount it? */
		} else if (mnt_fs_match_options(fs, "ro")) { /* boot mounted r/o, remount r/w */
			if (mnt_context_set_options(ctx, "remount,rw"))
				throw std::runtime_error("unable to set mount options (to 'remount,rw')");
			if (mnt_context_mount(ctx))
				throw std::runtime_error("remount r/w failed");
			self->status = MOUNTPOINT_REMOUNTED_RW;
		}

		return Py_None;
	} catch (std::runtime_error err) {
		PyErr_SetString(PyExc_RuntimeError, err.what());
		return NULL;
	} catch (std::exception e) {
		PyErr_SetString(PyExc_Exception, e.what());
		return NULL;
	}
}

static PyObject *BootMountpoint_umount(PyObject *args, PyObject *kwds) {
	try {
		BootMountpoint *self = reinterpret_cast<BootMountpoint*>(args);
		struct libmnt_context* const ctx = self->mnt_context;

		switch (self->status) {
			case MOUNTPOINT_NONE:
				break;
			case MOUNTPOINT_REMOUNTED_RW:
				if (mnt_reset_context(ctx))
					throw std::runtime_error("unable to reset mount context");
				if (mnt_context_set_target(ctx, "/boot"))
					throw std::runtime_error("unable to set mountpoint to /boot");

				if (mnt_context_set_options(ctx, "remount,ro"))
					throw std::runtime_error("unable to set mount options (to 'remount,ro')");
				if (mnt_context_mount(ctx))
					throw std::runtime_error("remount r/o failed");

				break;
			default:
				throw std::runtime_error("Invalid mountpoint_status value");
		}

		self->status = MOUNTPOINT_NONE;
		return Py_None;
	} catch (std::runtime_error err) {
		PyErr_SetString(PyExc_RuntimeError, err.what());
		return NULL;
	} catch (std::exception e) {
		PyErr_SetString(PyExc_Exception, e.what());
		return NULL;
	}
}
