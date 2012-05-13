/* pymountboot -- (re)mount /boot (for eclean-kernel)
 * (c) 2012 Michał Górny <mgorny@gentoo.org>
 * Released under the terms of the 2-clause BSD license.
 */

#include <Python.h>

#include <libmount.h>

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
static PyObject *BootMountpoint_rwmount(PyObject *args, PyObject *kwds);
static PyObject *BootMountpoint_umount(PyObject *args, PyObject *kwds);

static PyMethodDef BootMountpoint_methods[] = {
	{ "mount", BootMountpoint_mount, METH_NOARGS,
		"Mount /boot (r/o) if necessary" },
	{ "rwmount", BootMountpoint_rwmount, METH_NOARGS,
		"Mount or remount /boot r/w if necessary" },
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
	BootMountpoint* const b = (BootMountpoint*) o;

	if (b->status != MOUNTPOINT_NONE) /* clean up! */
		BootMountpoint_umount(o, NULL);

	mnt_free_context(b->mnt_context);
}

static void* BootMountpoint_reset(struct libmnt_context *ctx) {
	if (mnt_reset_context(ctx))
		return PyErr_Format(PyExc_RuntimeError,
				"unable to reset mount context");
	if (mnt_context_set_target(ctx, "/boot"))
		return PyErr_Format(PyExc_RuntimeError,
				"unable to set mountpoint to /boot");

	return ctx; /* some non-NULL value */
}

static PyObject *BootMountpoint_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	struct libmnt_context* const ctx = mnt_new_context();

	if (!ctx)
		return PyErr_Format(PyExc_RuntimeError,
				"unable to create libmount context");

	PyObject *self = type->tp_alloc(type, 0);

	if (self != NULL) {
		BootMountpoint* const b = (BootMountpoint*) self;

		b->mnt_context = ctx;
		b->status = MOUNTPOINT_NONE;
	}

	return self;
}

static PyObject *BootMountpoint_mount(PyObject *args, PyObject *kwds) {
	BootMountpoint* const self = (BootMountpoint*) args;
	struct libmnt_context* const ctx = self->mnt_context;

	BootMountpoint_reset(ctx);

	struct libmnt_table *mtab;
	if (mnt_context_get_mtab(ctx, &mtab))
		return PyErr_Format(PyExc_RuntimeError,
				"unable to get mtab");

	/* backward -> find the top-most mount */
	struct libmnt_fs* const fs = mnt_table_find_target(mtab, "/boot",
			MNT_ITER_BACKWARD);

	if (!fs) { /* boot not mounted, mount it? */
		struct libmnt_table *fstab;
		if (mnt_context_get_fstab(ctx, &fstab))
			return PyErr_Format(PyExc_RuntimeError,
					"unable to get fstab");

		/* try to mount only if in fstab */
		if (mnt_table_find_target(fstab, "/boot", MNT_ITER_FORWARD)) {
			if (mnt_context_set_options(ctx, "ro"))
				return PyErr_Format(PyExc_RuntimeError,
						"unable to set mount options (to 'ro')");
			if (mnt_context_mount(ctx))
				return PyErr_Format(PyExc_RuntimeError,
						"mount failed");
			self->status = MOUNTPOINT_MOUNTED;
		}
	}

	return Py_None;
}

static PyObject *BootMountpoint_rwmount(PyObject *args, PyObject *kwds) {
	BootMountpoint* const self = (BootMountpoint*) args;
	struct libmnt_context* const ctx = self->mnt_context;

	BootMountpoint_reset(ctx);

	struct libmnt_table *mtab;
	if (mnt_context_get_mtab(ctx, &mtab))
		return PyErr_Format(PyExc_RuntimeError,
				"unable to get mtab");

	/* backward -> find the top-most mount */
	struct libmnt_fs* const fs = mnt_table_find_target(mtab, "/boot",
			MNT_ITER_BACKWARD);

	if (!fs) { /* boot not mounted, mount it? */
		struct libmnt_table *fstab;
		if (mnt_context_get_fstab(ctx, &fstab))
			return PyErr_Format(PyExc_RuntimeError,
					"unable to get fstab");

		/* try to mount only if in fstab */
		if (mnt_table_find_target(fstab, "/boot", MNT_ITER_FORWARD)) {
			if (mnt_context_set_options(ctx, "rw"))
				return PyErr_Format(PyExc_RuntimeError,
						"unable to set mount options (to 'rw')");
			if (mnt_context_mount(ctx))
				return PyErr_Format(PyExc_RuntimeError,
						"mount failed");
			self->status = MOUNTPOINT_MOUNTED;
		}
	} else if (mnt_fs_match_options(fs, "ro")) { /* boot mounted r/o, remount r/w */
		if (mnt_context_set_options(ctx, "remount,rw"))
			return PyErr_Format(PyExc_RuntimeError,
					"unable to set mount options (to 'remount,rw')");
		if (mnt_context_mount(ctx))
			return PyErr_Format(PyExc_RuntimeError,
					"remount r/w failed");
		if (self->status != MOUNTPOINT_MOUNTED)
			self->status = MOUNTPOINT_REMOUNTED_RW;
	}

	return Py_None;
}

static PyObject *BootMountpoint_umount(PyObject *args, PyObject *kwds) {
	BootMountpoint* const self = (BootMountpoint*) args;
	struct libmnt_context* const ctx = self->mnt_context;

	switch (self->status) {
		case MOUNTPOINT_NONE:
			break;
		case MOUNTPOINT_MOUNTED:
			BootMountpoint_reset(ctx);

			if (mnt_context_enable_lazy(ctx, 1))
				return PyErr_Format(PyExc_RuntimeError,
						"unable to enable lazy umount");
			if (mnt_context_enable_rdonly_umount(ctx, 1))
				return PyErr_Format(PyExc_RuntimeError,
						"unable to enable rdonly umount-fallback");
			if (mnt_context_umount(ctx))
				return PyErr_Format(PyExc_RuntimeError,
						"unmount failed");

			break;
		case MOUNTPOINT_REMOUNTED_RW:
			BootMountpoint_reset(ctx);

			if (mnt_context_set_options(ctx, "remount,ro"))
				return PyErr_Format(PyExc_RuntimeError,
						"unable to set mount options (to 'remount,ro')");
			if (mnt_context_mount(ctx))
				return PyErr_Format(PyExc_RuntimeError,
						"remount r/o failed");

			break;
		default:
			return PyErr_Format(PyExc_RuntimeError,
					"Invalid mountpoint_status value");
	}

	self->status = MOUNTPOINT_NONE;
	return Py_None;
}
