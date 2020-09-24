Windows
-------

This is the last binary build of the WST software, since it is no longer in active development. The source code is available along with the "NMAKE" files in the appropriate package directory. The build is fragile, since it might require older versions of Python and MSVS runtime libraries. Due to these limitations, the best method to compile and build WST is on a Linux machine. Associated projects under active development include the following: Water Network Tool for Resilience (WNTR) available at https://github.com/USEPA/WNTR, Chama available at https://github.com/sandialabs/chama and Pecos available at https://github.com/sandialabs/pecos.

**Installation instructions:**

Download Anaconda 2.7 from
https://www.anaconda.com/distribution/#download-section.
Get the “Anaconda 2.7” version – not the Anaconda 3.x.

Download Dakota/Coliny from 
https://dakota.sandia.gov/downloads.
Get the “Windows”, “6.8”, “command line only”, “unsupported” version (``dakota-6.8-release-public-Windows.x86.zip``).

Download CBC from 
https://www.coin-or.org/download/binary/CoinAll.
Get the “Windows 1.7.4” version build date “2013-12-26 18:14” (``COIN-OR-1.7.4-win32-msvc11.zip``).

Download WST.
Get the “Release 2019” from the files below (``wst-2019.zip``).

Perform the following steps:
- Install Anaconda2 as “Just Me” so you do not need administrator access
- Unzip ``wst-2019.zip`` into the directory where you want it to be kept
- Unzip the ``dakota-6.8-release-public-Windows.x86.zip`` file (should create a directory called the same thing, without the .zip extension, in the same directory as the zip file)
- Copy all the files from the ``dakota-6.8-release-public-Windows.x86\bin`` directory into the ``wst-2019\bin`` directory
- Unzip the ``COIN-OR-1.7.4-win-21-msvc11.zip`` file (should create a directory called the same thing, without the .zip extension, in the same directory as the zip file)
- Copy all the files from the ``COIN-OR-1.7.4-win-21-msvc11\win32-msvc11\bin`` directory into the ``wst-2019\bin`` directory
- Open an Anaconda Prompt from the “Start -> Anaconda2” menu
- Change directory into the ``wst-2019`` directory 
- Type “install”
- Change directory into the ``wst-2019\examples`` directory
- Run the examples (e.g., ``wst sp sp_ex1.yml``)

