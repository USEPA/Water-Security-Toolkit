#
import copy
import os
import sys
import yaml

import pywst.common.wst_config as wst_config
from pyutilib.misc.config import ConfigValue, ConfigBlock, ConfigList
import pyutilib.subprocess

wst = os.path.join( os.path.dirname(os.path.abspath(__file__)),
                    '..','..','..','python','bin','wst' )
if os.path.exists(wst):
    # fall-back on the system path
    wst = [sys.executable, wst]
else:
    sys.stdout.write('WARNING: wst executable not found in %s;\n'
                     '   falling back on system path\n' % (wst,) )
    wst = ['wst']
def run(args):
    rc, output = pyutilib.subprocess.run(args)
    if rc:
        sys.stdout.write("ERROR running wst\n\tcommand: %s\n\toutput:\n%s" 
                         % ( args, output))
        sys.exit(-1)
    sys.stdout.write(output)

targets = set(os.path.basename(x) for x in sys.argv[1:])
if targets:
    targets.add(None)

# master
config = wst_config.master_config()
target = 'master_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing template file %s\n" % target)
    fid = open(target, 'w')
    fid.write(config.generate_yaml_template(indent_spacing=2, width=85))
    fid.close()

target = 'master_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    fid.write(config.generate_documentation())
    fid.close()

# tevasim 
target = 'tevasim_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['tevasim','--template',target])

target = 'tevasim_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    tevasim = ConfigBlock('tevasim configuration template')
    for k in ('network', 'scenario', 'configure'):
        tevasim.declare( k, config[k]() )
    fid.write(tevasim.generate_documentation())
    fid.close()

# sim2Impact
target = 'sim2Impact_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['sim2Impact','--template',target])

target = 'sim2Impact_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    sim2Impact = ConfigBlock('sim2Impact configuration template')
    for k in ('impact', 'configure'):
        sim2Impact.declare( k, config[k]() )
    fid.write(sim2Impact.generate_documentation())
    fid.close()

# sp
target = 'sp_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['sp','--template',target])

target = 'sp_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    sp = ConfigBlock('sp configuration template')
    for k in ('impact data', 'cost', 'objective', 'constraint', 'aggregate', 
              'imperfect', 'sensor placement', 'solver', 'configure'):
        sp.declare( k, config[k]() )
    sp['cost'].add()
    sp['objective'].add()
    sp['constraint'].add()
    sp['aggregate'].add()
    sp['imperfect'].add()
    sp['sensor placement'].add()
    fid.write(sp.generate_documentation())
    fid.close()

# flushing
target = 'flushing_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['flushing','--template',target])

target = 'flushing_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    flushing = ConfigBlock('flushing configuration template')
    for k in ('network', 'scenario', 'impact', 'flushing', 'solver', 
              'configure'):
        flushing.declare( k, config[k]() )
    fid.write(flushing.generate_documentation())
    fid.close()

# booster msx
target = 'booster_msx_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['booster_msx','--template',target])

target = 'booster_msx_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    booster_msx = ConfigBlock('booster msx configuration template')
    for k in ('network', 'scenario', 'impact', 'booster msx', 'solver', 
              'configure'):
        booster_msx.declare( k, config[k]() )
    fid.write(booster_msx.generate_documentation())
    fid.close()

# booster mip
target = 'booster_mip_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['booster_mip','--template',target])

target = 'booster_mip_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    booster_mip = ConfigBlock('booster mip configuration template')
    for k in ('network', 'scenario', 'booster mip', 'solver', 'configure'):
        booster_mip.declare( k, config[k]() )
    fid.write(booster_mip.generate_documentation())
    fid.close()

# source inversion
target = 'inversion_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['inversion','--template',target])

target = 'inversion_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    inversion = ConfigBlock('source inversion template')
    for k in ('network', 'measurements', 'inversion', 'solver', 'configure'):
        inversion.declare(k, config[k]())
    fid.write(inversion.generate_documentation())
    fid.close()

# grabsample
target = 'grabsample_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['grabsample','--template',target])

target = 'grabsample_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    grabsample = ConfigBlock('grabsample template')
    for k in ('network', 'scenario', 'grabsample', 'solver', 'configure'):
        grabsample.declare(k, config[k]())
    fid.write(grabsample.generate_documentation())
    fid.close()

# uq
target = 'uq_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    uq = ConfigBlock('uq template')
    for k in ('scenario', 'uq','measurements', 'configure'):
        uq.declare(k, config[k]())
    fid.write(uq.generate_documentation())
    fid.close()

    
# visualization
target = 'visualization_config.yml'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    run(wst+['visualization','--template',target])

target = 'visualization_config.tex'
if target in targets or not targets:
    if target in targets: targets.remove(target)
    sys.stdout.write("Writing documentation file %s\n" % target)
    fid = open(target, 'w')
    visualization = ConfigBlock('visualization template')
    for k in ('network', 'visualization', 'configure'):
        visualization.declare(k, config[k]())
    fid.write(visualization.generate_documentation())
    fid.close()
    
if None in targets:
    targets.remove(None)
if targets:
    sys.stderr.write("ERROR: Unknown targets for generate_config.py:\n\t"
                     "[ %s ]\n" % ', '.join("%s" %x for x in targets))
    sys.exit(-1)
