#!/usr/bin/python
#	vim:fileencoding=utf-8
# (c) 2012 Michał Górny <mgorny@gentoo.org>
# Released under the terms of the 2-clause BSD license.

from distutils.core import setup, Extension

import subprocess

def pkgconfig_get(*args):
	output = subprocess.check_output(('pkg-config',) + args)
	return output.decode('utf8').strip().split()

cflags, libs = [pkgconfig_get(x, 'mount') for x in ('--cflags', '--libs')]

setup(
		name = 'pyremountboot',
		version = '0',
		author = 'Michał Górny',
		author_email = 'mgorny@gentoo.org',
		url = 'http://github.com/mgorny/pyremountboot',

		ext_modules = [
			Extension('pymountboot',
				extra_compile_args = cflags,
				extra_link_args = libs,
				sources = ['pymountboot.cxx'])
		],

		classifiers = [
			'Development Status :: 3 - Alpha',
			'Environment :: Console',
			'Intended Audience :: System Administrators',
			'License :: OSI Approved :: BSD License',
			'Operating System :: POSIX :: Linux',
			'Programming Language :: C++',
			'Topic :: System :: Filesystems'
		]
)
