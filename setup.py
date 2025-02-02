#!/usr/bin/env python
from setuptools import setup, Extension
import re
import os


def read_file(filename, encoding='utf-8'):
    with open(filename, 'r', encoding=encoding) as f:
        return f.read()


def get_version():
    """
    Extract the version number from arraydeque.c.
    It looks for a line of the form:
        #define ARRAYDEQUE_VERSION "x.y.z"
    """
    with open('arraydeque.c', 'r', encoding='utf-8') as f:
        content = f.read()
    match = re.search(r'#define\s+ARRAYDEQUE_VERSION\s+"([^"]+)"', content)
    if match:
        return match.group(1)
    raise RuntimeError('Unable to find version string in arraydeque.c.')


here = os.path.abspath(os.path.dirname(__file__))

setup(
    name='arraydeque',
    version=get_version(),
    description='Array-backed deque implementation written in C for fast double-ended operations.',
    long_description=read_file('README.md'),
    long_description_content_type='text/markdown',
    author='Grant Jenks',
    author_email='grant.jenks@gmail.com',
    url='https://github.com/grantjenks/python-arraydeque',
    license='Apache 2.0',
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: Implementation :: CPython',
    ],
    ext_modules=[Extension('arraydeque', sources=['arraydeque.c'])],
    python_requires='>=3.8',
)
