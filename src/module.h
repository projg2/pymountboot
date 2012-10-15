/* pymountboot -- (re)mount /boot (for eclean-kernel)
 * (c) 2012 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#pragma once

#ifndef _MODULE_H
#define _MODULE_H 1

#include <Python.h>

static const char module_name[] = "pymountboot";
static const char module_doc[] = "Module used to handle /boot auto(re)mounting.";

int types_preinit();
void types_postinit(PyObject* m);

#endif /*_MODULE_H*/
