
"""
Script to generate the installer for wst_admin.
"""

import glob
import os

def read(*rnames):
    return open(os.path.join(os.path.dirname(__file__), *rnames)).read()

from setuptools import setup

setup(name='wst_admin',
      version='1.0',
      maintainer='William Hart',
      maintainer_email='wehart@sandia.gov',
      license = 'BSD',
      platforms = ["any"],
      description = 'Administrative scripts for WST',
      long_description = read('README.txt'),
      classifiers = [
            'Development Status :: 4 - Beta',
            'Intended Audience :: End Users/Desktop',
            'Intended Audience :: Science/Research',
            'License :: OSI Approved :: BSD License',
            'Natural Language :: English',
            'Operating System :: Microsoft :: Windows',
            'Operating System :: Unix',
            'Programming Language :: Python',
            'Programming Language :: Unix Shell',
            'Topic :: Software Development :: Libraries :: Python Modules'
        ],
      packages=['wst_admin'],
      keywords=['utility'],
      entry_points="""
        [console_scripts]
        test.wst = wst_admin.scripts:runWSTTests
      """
      )

