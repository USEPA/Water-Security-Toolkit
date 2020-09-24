
import sys
import os.path
import optparse
from os.path import abspath, dirname
import pyutilib.dev.runtests

def runSpotTests():
    parser = optparse.OptionParser(usage='test.pywst [options] <dirs>')

    parser.add_option('-d','--dir',
        action='store',
        dest='dir',
        default=None,
        help='Top-level source directory where the tests are applied.')
    parser.add_option('--all',
        action='store_true',
        dest='all_cats',
        default=False,
        help='All tests are executed.')
    parser.add_option('--cat','--category',
        action='append',
        dest='cats',
        default=[],
        help='Specify test categories.')
    parser.add_option('--cov','--coverage',
        action='store_true',
        dest='coverage',
        default=False,
        help='Indicate that coverage information is collected')
    parser.add_option('-v','--verbose',
        action='store_true',
        dest='verbose',
        default=False,
        help='Verbose output')
    parser.add_option('-o','--output',
        action='store',
        dest='output',
        default=None,
        help='Redirect output to a file')

    _options, args = parser.parse_args(sys.argv)

    if _options.output:
        outfile = os.path.abspath(_options.output)
    else:
        outfile = None
    if _options.dir is None:
        os.chdir( os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))) )
    else:
        os.chdir( _options.dir )

    print "Running tests in directory",os.getcwd()
    if _options.all_cats is True:
        _options.cats = []
    elif len(_options.cats) == 0:
        _options.cats = ['smoke']
    elif 'all' in _options.cats:
        _options.cats = []
    if len(_options.cats) > 0:
        os.environ['PYUTILIB_UNITTEST_CATEGORIES'] = ",".join(_options.cats)
        print " ... for test categories: "+ os.environ['PYUTILIB_UNITTEST_CATEGORIES']
    options=[]
    if _options.coverage:
        options.append('--coverage')
    if _options.verbose:
        options.append('-v')
    if outfile:
        options.append('-o')
        options.append(outfile)
    if len(args) == 1:
        dirs=['pywst']
    else:
        dirs=[]
        for dir in args:
            if dir.startswith('-'):
                options.append(dir)
            if dir.startswith('pywst'):
                dirs.append(dir)
            else:
                dirs.append(dirname(dirname(abspath(__file__)))+os.sep+dir)
        if len(dirs) == 0:
            dirs = ['pywst']

    pyutilib.dev.runtests.run('pywst', ['runtests']+options+['-p','pywst']+dirs)

