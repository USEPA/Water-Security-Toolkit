
TEST DIRECTORIES

Subdirectories of this directory contain the following types of tests:

. functionality/integration tests

  These tests exercise sensor placement components in isolation, using
  the Python unit testing framework.  Test modules have names like
  "test_{name}.py".

  Directories: acceptance, sp

. exact studies

  The exact studies run series of tests varying important parameters to
  increase test coverage.  The exact studies are defined by XML
  files with the name "{name}.study.xml".

  Directories: studies, bigtests

. C++ unit tests

  These tests directly exercise C++ classes to validate their API.
  C++ unit tests leverage the CxxTest testing framework, which builds
  a single test runner:  unit/runner.

  Directories: unit

Other test directories are retained for historical reasons:

. data

  Data sets used by various tests.

. test1

  This directory contains the original linux shell and windows batch
  scripts that set up problems to run with the sp script.

. Robs

  This directory contains a python script called "compare.py" which can
  be used to run different solvers with different parameters.  The results
  are output to a csv file which can be viewed in a spreadsheet.  We want
  to run this more often and look for anomalies of the type that Rob
  Janke brought to our attention recently.  By editing the top of the
  compare.py file, you can change the values of parameters over which
  the tests are run.

. obsolete, OLD

  Depricated tests.

RUNNING TESTS

TODO: this will be depricated as we move to 'nose'.

Before running compare.py or any of the unit tests, run "runtests.py"
at least once.  It creates "path.py" which the other tests need to find
supporting python code.  You can also use runtests.py to select and run
the unit tests.
