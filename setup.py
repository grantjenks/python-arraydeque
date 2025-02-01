from setuptools import setup, Extension

module = Extension(
    'arraydeque',
    sources=['arraydeque.c']
)

setup(
    name='arraydeque',
    version='0.1',
    description='Array-backed deque implementation',
    ext_modules=[module],
)
