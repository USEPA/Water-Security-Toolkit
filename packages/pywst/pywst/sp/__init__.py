#  _________________________________________________________________________
#
#  WST: the Water Security Toolkit
#  Copyright (c) 2012 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the WST README.txt file.
#  _________________________________________________________________________

import solvers

#
# Register external executables
#
import pyutilib.services
pyutilib.services.register_executable('PICO')
pyutilib.services.register_executable('glpsol')
pyutilib.services.register_executable('ampl')
pyutilib.services.register_executable('evalsensor')
pyutilib.services.register_executable('cplexamp')
pyutilib.services.register_executable('cbc')
pyutilib.services.register_executable('memmon')
pyutilib.services.register_executable('valgrind')
pyutilib.services.register_executable('grasp')
pyutilib.services.register_executable('heuristic')
pyutilib.services.register_executable('aggregateImpacts')
pyutilib.services.register_executable('createIPData')
pyutilib.services.register_executable('createLagData')
pyutilib.services.register_executable('ufl')
pyutilib.services.register_executable('imperfect')
pyutilib.services.register_executable('new_imperfect')
pyutilib.services.register_executable('randomsample')
pyutilib.services.register_executable('new_randomsample')
pyutilib.services.register_executable('sideconstraints')
pyutilib.services.register_executable('preprocessImpacts')
pyutilib.services.register_executable('runef')
pyutilib.services.register_executable('runph')

