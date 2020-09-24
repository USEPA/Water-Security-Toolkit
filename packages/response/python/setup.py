#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

"""
Script to generate the installer for TEVA_RESPONSE.
"""

__maintainer__ = "David Hart"
__maintainer_email__ = "dbhart@sandia.gov"
__copyright__ = "Copyright 2009 Sandia Corporation"
__license__ = "BSD"
__url__ = "https://software.sandia.gov/trac/spot"
__version__ = "0.1"
__date__ = "2009-09-29"
__doc__="""
TEVA_RESPONSE: A Python package for the TEVA-SPOT Toolkit

TEVA_RESPONSE provides a collection of Python tools that support response
planning support for the TEVA-SPOT Toolkit.
"""


classifiers = """\
Development Status :: 4 - Beta
Intended Audience :: End Users/Desktop
Intended Audience :: Science/Research
License :: OSI Approved :: BSD License
Natural Language :: English
Operating System :: Microsoft :: Windows
Operating System :: Unix
Programming Language :: Python
Programming Language :: Unix Shell
Topic :: Scientific/Engineering :: Mathematics
Topic :: Software Development :: Libraries :: Python Modules
"""

import glob
import os

def _find_packages(path):
    """
    Generate a list of nested packages
    """
    pkg_list=[]
    if not os.path.exists(path):
        return []
    if not os.path.exists(path+os.sep+"__init__.py"):
        return []
    else:
        pkg_list.append(path)
    for root, dirs, files in os.walk(path, topdown=True):
      if root in pkg_list and "__init__.py" in files:
         for name in dirs:
           if os.path.exists(root+os.sep+name+os.sep+"__init__.py"):
              pkg_list.append(root+os.sep+name)
    return map(lambda x:x.replace(os.sep,"."), pkg_list)

try:
    from setuptools import setup
    packages = _find_packages('tevaresponse')
except:
    from distutils.core import setup
    packages = _find_packages('tevaresponse')

scripts = glob.glob("scripts/*")
doclines = __doc__.split("\n")

setup(name="tevaresponse",
      version=__version__,
      maintainer=__maintainer__,
      maintainer_email=__maintainer_email__,
      url = __url__,
      license = __license__,
      platforms = ["any"],
      description = doclines[0],
      classifiers = filter(None, classifiers.split("\n")),
      long_description = "\n".join(doclines[2:]),
      packages=packages,
      keywords=['software testing'],
      scripts=scripts
      )

