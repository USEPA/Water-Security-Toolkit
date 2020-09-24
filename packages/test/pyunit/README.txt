This directory contains acceptance tests that must pass
before a teva-spot release is created.  They are based
on the test1* scripts that used to be run for this purpose.

The solvers are tested indivdually, as are tevasim and
tso2Impact.  The sp script is not tested here.  That is tested
in test/sp.  By testing these components individually we
can better pinpoint problems, and also if we experience and
error in one component we are not hampered in testing the
others.
