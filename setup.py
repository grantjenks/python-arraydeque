from setuptools import setup, Extension
import re

def get_version():
    """
    Extract the version number from arraydeque.c.
    Looks for a line of the form:
        #define ARRAYDEQUE_VERSION "0.1.0"
    """
    with open("arraydeque.c", "r", encoding="utf-8") as f:
        content = f.read()
    match = re.search(r'#define\s+ARRAYDEQUE_VERSION\s+"([^"]+)"', content)
    if match:
        return match.group(1)
    raise RuntimeError("Unable to find version string in arraydeque.c.")

module = Extension('arraydeque', sources=['arraydeque.c'])

setup(
    name='arraydeque',
    version=get_version(),
    description='Array-backed deque implementation',
    ext_modules=[module],
)
