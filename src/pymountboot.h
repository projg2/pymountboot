/* pymountboot -- (re)mount /boot (for eclean-kernel)
 * (c) 2012 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#pragma once

#ifndef PYMOUNTBOOT_H
#define PYMOUNTBOOT_H 1

#include <libmount.h>

#include <Python.h>

enum mountpoint_status
{
	MOUNTPOINT_NONE,
	MOUNTPOINT_MOUNTED,
	MOUNTPOINT_REMOUNTED_RW
};

typedef struct
{
	PyObject_HEAD

	struct libmnt_context *mnt_context;
	enum mountpoint_status status;
} BootMountpoint;

extern PyTypeObject BootMountpointType;

#endif /*PYMOUNTBOOT_H*/
