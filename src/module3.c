/* pymountboot -- (re)mount /boot (for eclean-kernel)
 * (c) 2012 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#include "module.h"

#if PY_MAJOR_VERSION >= 3

static PyModuleDef pymountboot_module = {
	PyModuleDef_HEAD_INIT, /* m_base */

	module_name, /* m_name */
	module_doc, /* m_doc */
	-1, /* m_size */
	NULL /* m_methods */
};

PyObject* PyInit_pymountboot()
{
	PyObject* m;

	if (!types_preinit())
		return NULL;

	m = PyModule_Create(&pymountboot_module);

	if (!m)
		return NULL;

	types_postinit(m);

	return m;
}

#endif
