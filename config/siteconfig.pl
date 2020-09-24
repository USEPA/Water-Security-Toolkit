#!/usr/bin/perl
#
# This PERL package defines site-specific values.  These are used in 
# Acro to automatically customize the configuration process (when
# using the acro/setup command), and for customizing the testing process.
#
my %hash = (
  'DEFAULT' => {
	siteconfig => '',
	pexec => '',
	mpi => '--with-mpi=/usr/local',
    debug => '--with-debugging --with-cxxflags="-W -Wall" --with-ldflags="-rdynamic"',
    coverage => '--with-debugging --enable-gcov --with-optimization="-O0" --with-ldflags="-rdynamic"',
    static => '--enable-static-executables',
	cplex => '',
	npsol => ''
  	},
# DEFAULT must list each variable and default value.
# Otherwise, the variable will not be set (even if specified below).
# The calling program can include these variables with
#
#   local ($siteconfig, $pexec, $amplflag, $cplexflag [etc] );
#   require siteconfig.pl (this file)
#

  'zamboni' => {
	pexec => 'mpirun -np ',
	},

  'bobo.sandia.gov' => {
	pexec => 'mpirun -np ',
	mpi => '--with-mpi=/Net/proj/opt/packages/LINUX/mpich --with-mpi-libs=-lmpich',
	cplex => '--with-cplex-libs="-lcplex -lpthread" \
                  --with-cplex-includes="-I/Net/proj/opt/packages/LINUX/ilm/cplex90" \
                  --with-cplex-ldflags="-L/Net/proj/opt/packages/LINUX/ilm/cplex90/lib/i86_linux2_glibc2.2_gcc3.0/static_pic_mt"'
	},

  'expo' => {
	pexec => 'mpirun -np ',
	cplex => '--with-cplex-libs="-lcplex -lpthread" \
                  --with-cplex-includes="-I/Net/proj/opt/packages/LINUX/ilm/cplex90" \
                  --with-cplex-ldflags="-L/Net/proj/opt/packages/LINUX/ilm/cplex90/lib/i86_linux2_glibc2.2_gcc3.0/static_pic_mt"'
	},

  'reddish' => {
	siteconfig => '--with-compiler=pgi --without-ampl',
	},

  'sahp5117' => {
	siteconfig => 'CXXFLAGS="-g" CFLAGS="-g"'
	},

  'S884292' => {
      siteconfig => '--with-compiler=mingw --target=i686-pc-mingw32 --host=i686-pc-mingw32 --disable-mpi --enable-static-executables'
      },

  'sasn100' => {
	},

  'software.sandia.gov' => {
	},

  'snuffles' => {
	},

  'tofu.sandia.gov' => {
	siteconfig => '--with-compiler=gcc64',
	mpi => '--with-mpi=/usr/local --with-mpi-libs=-lmpich',
	npsol => '--with-npsol=/home/wehart/lib',
	cplex => '--with-cplex-libs="-lcplex -lpthread" \
                  --with-cplex-includes="-I/usr/ilog/ilm/cplex80" \
                  --with-cplex-ldflags="-L/usr/ilog/ilm/cplex80/lib/i86_linux2_glibc2.2_gcc3.0/static_pic_mt"'
	},

  'qed.sandia.gov' => {
	mpi => '--with-mpi=/usr/local/mpich-1.2.6-eth --with-mpi-libs="-lmpich"'
	},

  );

# use Data::Dumper;
# print Dumper %hash;
# print Dumper %{$hash{DEFAULT}};

chomp(my $host=`hostname`);

sub config_value {
  my $var = shift @_;
  $value = defined($hash{$host}{$var}) 
	? $hash{$host}{$var}
	: $hash{DEFAULT}{$var};
  return $value;
}

1;
