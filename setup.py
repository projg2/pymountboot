#!/usr/bin/python
#	vim:fileencoding=utf-8
# (c) 2012 Michał Górny <mgorny@gentoo.org>
# Released under the terms of the 2-clause BSD license.

from distutils.core import setup, Extension

import subprocess, sys

def pkgconfig_get(*args):
	subp = subprocess.Popen(('pkg-config',) + args,
			stdout = subprocess.PIPE)
	output = subp.communicate()[0]
	if subp.returncode != 0:
		sys.exit('\npkg-config failed to find libmount, please install util-linux.')
	return output.decode('utf8').strip().split()

cflags, libs = [pkgconfig_get(x, 'mount') for x in ('--cflags', '--libs')]

setup(
		name = 'pymountboot',
		version = '0.1',
		author = 'Michał Górny',
		author_email = 'mgorny@gentoo.org',
		url = 'https://bitbucket.org/mgorny/pymountboot',

		ext_modules = [
			Extension('pymountboot',
				extra_compile_args = cflags,
				extra_link_args = libs,
				sources = ['pymountboot.c'])
		],

		classifiers = [
			'Development Status :: 4 - Beta',
			'Environment :: Console',
			'Intended Audience :: System Administrators',
			'License :: OSI Approved :: BSD License',
			'Operating System :: POSIX :: Linux',
			'Programming Language :: C++',
			'Topic :: System :: Filesystems'
		]
)
