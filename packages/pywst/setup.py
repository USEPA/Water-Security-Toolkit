"""
Copyright 2011 Sandia Corporation Under the terms of Contract
DE-AC04-94AL85000 there is a non-exclusive license for use of this work by
or on behalf of the U.S. Government. Export of this program may require
a license from the United States Government.

This software is licensed under the BSD license. See README.txt for
details.
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

def read(*rnames):
    return open(os.path.join(os.path.dirname(__file__), *rnames)).read()

from setuptools import setup

packages = _find_packages('pywst')

if __name__ == '__main__':
    setup(name='pywst',
      version='1.1',
      description='Python Utilities for the Water Security Toolkit',
      author='David Hart',
      author_email='dbhart.sandia@gmail.com',
      url='https://software.sandia.gov/trac/wst',
      keywords=['EPANET','water distribution system','water','contamination',
                'hydraulics','water quality','water network'],
      packages=packages,
      namespace_packages=['pywst'],
      license='BSD',
      classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: Console',
        'Environment :: X11 Applications :: Qt',
        'Environment :: Win32 (MS Windows)',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'Intended Audience :: End Users/Desktop',
        'License :: OSI Approved :: BSD License',
        'Topic :: Scientific/Engineering'
      ],
          requires=['yaml', 'PyUtilib', 'argparse', 'Coopr','numpy','texttable'],
      scripts=glob.glob('scripts/*'),
      include_package_data = True,
      package_data = {
        '': ['*.mod'],
        },
      entry_points = """
        [console_scripts]
        wst                 = pywst.common.wst:main
        test.pywst          = pywst.spot.runtests:runSpotTests
        responseGui         = pywst.gui.pywstApp:main
        flushing_response   = pywst.flushing.driver:main
        booster_response    = pywst.booster.driver:main
        sp                  = pywst.sp.main:main
        [pywst.common]
        flushing            = pywst.flushing.driver
        booster_msx         = pywst.booster_msx.driver
        booster_mip         = pywst.booster_mip.driver
        booster_quality     = pywst.booster_quality.driver
        inversion           = pywst.inversion.driver
        tevasim             = pywst.tevasim.driver
        sim2Impact          = pywst.sim2Impact.driver
        sp_cmd              = pywst.sp.driver
        grabsample          = pywst.grabsample.driver
        uq                  = pywst.uq.driver
        visualization       = pywst.visualization.driver
      """
     )

