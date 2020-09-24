#!python

"""
Python extensions for the EPANET Programmers Toolkit DLLs

Copyright 2011 Sandia Corporation Under the terms of Contract 
DE-AC04-94AL85000 there is a non-exclusive license for use of this 
work by or on behalf of the U.S. Government. Export of this program
may require a license from the United States Government.

This software is licensed under the BSD license. See README.txt for
details.

"""

# Define the EPANET toolkit constants
# last modified:
#  - January, 2011, David Hart, SNL

from setuptools import setup, find_packages
import os
data_files = []
if os.name == 'nt':
    data_files = {'': ['data/Windows/*.dll']}
elif os.name == 'posix':
    if os.uname()[0] == 'Linux':
        data_files = {'': ['data/Linux/*.so']}
    elif os.uname()[0] == 'Darwin':
        data_files = {'': ['data/Darwin/*.dylib']}
    else:
        print 'Unsupported OS - download EPANET2 DLL yourself'
        data_files = []
else:
    print 'Unsupported OS - download EPANET2 DLL yourself'
    data_files = []

setup(name='PyEPANET',
      version='0.1.1',
      description='Python-style wrappers for the EPANET toolkit functions',
      author='David Hart',
      author_email='dbhart.sandia@gmail.com',
      url='http://www.sourceforge.net/projects/epanet',
      packages=find_packages('.'),
      provides=['pyepanet'],
      include_package_data=True,
      zip_safe=False,
      package_data = data_files,
      requires=['ctypes'],
      license='BSD',
      classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: Console',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'Intended Audience :: End Users/Desktop',
        'License :: OSI Approved :: BSD License',
        'Topic :: Scientific/Engineering'
      ],
      keywords=['EPANET','water distribution system','water',
                'hydraulics','water quality','water network'],
      entry_points="""
        [console_scripts]
        epanet=pyepanet.epanet2:main
      """
     )
