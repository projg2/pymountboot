/* pymountboot -- (re)mount /boot (for eclean-kernel)
 * (c) 2012 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#include "module.h"
#include "pymountboot.h"

int types_preinit()
{
	return PyType_Ready(&BootMountpointType) >= 0;
}

void types_postinit(PyObject* m)
{
	Py_INCREF(&BootMountpointType);
	PyModule_AddObject(m, "BootMountpoint", (PyObject *)&BootMountpointType);
}
