import os
import sys
import pyutilib.services

if os.path.sep in sys.argv[0]:
    WST_BIN = os.path.dirname(os.path.abspath(sys.argv[0]))
else:
    # WST was found on the path
    pyutilib.services.register_executable('wst')
    command = pyutilib.services.registered_executable('wst')
    if not command is None:
        WST_BIN = command.get_path()
    else:
        WST_BIN = None
        
if not WST_BIN is None:
    WST_HOME = os.path.dirname(WST_BIN)
    os.environ['PATH'] = os.pathsep.join((WST_BIN, os.environ['PATH']))
    # While the ZIP installer puts all binaries in one directory, 
    # if we are running in source, there are two bin directories:
    #    wst/bin
    #    wst/python/bin
    if os.path.basename(WST_HOME).lower() == 'python':
        WST_HOME = os.path.dirname(WST_HOME)
        os.environ['PATH'] = os.pathsep.join(( os.path.join(WST_HOME,'bin'),
                                            os.environ['PATH'] ))

pyutilib.services.register_executable('wst')
pyutilib.services.register_executable('dakota')
pyutilib.services.register_executable('coliny')
pyutilib.services.register_executable('ampl')
pyutilib.services.register_executable('ampl64')
pyutilib.services.register_executable('tevasim')
pyutilib.services.register_executable('tso2Impact')
pyutilib.services.register_executable('sp')
pyutilib.services.register_executable('boostersim')
pyutilib.services.register_executable('boosterimpact')
pyutilib.services.register_executable('eventDetection')
pyutilib.services.register_executable('inversionsim')
pyutilib.services.register_executable('erddiff')

