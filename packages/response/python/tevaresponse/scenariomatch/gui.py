#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

from formlayout import fedit

def get_options(options):
    datalist = [('Event database','events.dat'),
                ('Scenario library','scenarios.dat'),
                ('Node name library','nodes.dat'),
                ('Sample entry mode',[0, 'From File','Interactive']),
                ('Sample results','samples.dat'),
                ('Output possible scenarios','possible_scenarios.dat'),
                ('Output next sample locs','sample_next.dat'),
                ('Number of sample teams',6)]
    feedback = fedit(datalist, title="Configure Scenario Match",
                     comment="Please configure scenario match tool")
    options.timedat = feedback[0]
    options.slibfile = feedback[1]
    options.nlibfile = feedback[2]
    if feedback[3] == 'Interactive': options.interactive = True
    else: options.interactive = False
    options.resfileout = feedback[4]
    options.psfileout = feedback[5]
    options.pnfileout = feedback[6]
    options.nodes = feedback[7]
    return options

def load_samples(scenarioDatabase):
    try:
        if scenarioDatabase.currentTime is None:
            defaultTime = 1
        else: defaultTime = scenarioDatabase.currentTime + 1
        while True:
            nodeKeys = scenarioDatabase.nodeNameToID.keys()
            datalist = [('Location',''),
                        ('Time',defaultTime),
                        ('Anomaly Detected',[0,'No','Yes'])]
            myComment = 'Please enter the sample information and press ' + \
                        '"Cancel" when complete'
            feedback = fedit(datalist, title='Enter sample',
                             comment=myComment)
            if feedback is None: break
            # nodeName = nodeIdToName[feedback[0].__str__()]
            nodeName = feedback[0].lower()
            if not scenarioDatabase.nodeNameToID.has_key(nodeName) and \
                   not scenarioDatabase.nodeIdToName.has_key(nodeName):
                print '  *** Location "%s" does not exist!' % nodeName
                continue
            nde = nodeName
            time = feedback[1]
            if feedback[2] == 'No': detect = False
            else: detect = True
            ct = len(scenarioDatabase.sampleResults.keys())
            scenarioDatabase.add_result(nde, time, detect, ct)
            if scenarioDatabase.verbosity > 0:
                print scenarioDatabase.sampleResults
                scenarioDatabase.get_remaining_scenarios()
    except (KeyboardInterrupt, EOFError):
        print '\n Done reading data from user.'
    return
