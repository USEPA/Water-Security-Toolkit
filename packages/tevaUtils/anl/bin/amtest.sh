#
# amtest.sh
#
# Author: Tom Taxon, ANL
#
# usage:   amtest.sh <values.xml> <config.xml> <output_dir> <tso_file> [<tso_pattern>]
#
#!/bin/bash

if [ -z $JAVAHOME ]; then
JAVAHOME=/Net/proj/opt/packages/LINUX/usr/java/j2sdk1.4.1_02
fi

if [ -z $TEVAMODULEHOME ]; then
TEVAMODULEHOME=$HOME/teva-sp/packages/tevalibs/TEVAModule
fi

CP=.
CP=$CP:$JAVAHOME/lib/tools.jar
CP=$CP:$TEVAMODULEHOME/jar/teva.jar
CP=$CP:$TEVAMODULEHOME/jar/anlutils.jar
CP=$CP:$TEVAMODULEHOME/jar/client.jar

PROPS=-Dteva.log.dir=logs
PROPS="$PROPS -Dteva.analysis.server.level=FINE"
PROPS="$PROPS -Dteva.amtest.console.log.level=ALL"
PROPS="$PROPS -Djava.library.path=$TEVAMODULEHOME/libs/linux"
PROPS="$PROPS -Danl.teva.modules.definitions=$2"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TEVAMODULEHOME/libs/linux

$JAVAHOME/bin/java -classpath $CP -Xmx512M $PROPS anl.teva.analysis.module.TestExternalAnalysisModules $1 $3 $4 $5
