#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

'''Take in a sampling results file and then compares
those sampling results to the remaining possible scenarios,
found in an additional input file, to eliminate those scenarios
that do not match the results. When the program is run with an
input file containing results of initial incidence of
contamination, the program goes through the file containing all
incidences of contamination in all of the pre-calculated
scenarios to determine the remaining possible scenarios.
It then uses a node-selecting algorithm, Decorrelation,
to determine the nodes to be sampled in the next time step. 
'''

import sys
from sys import stdin, stdout, stderr
if sys.version_info[1] < 6: from sets import Set as set
import numpy as np
import gui, tui

def merlionBool(str):
    str = str.lower()
    str = str.strip()
    c = str[0]
    if c == '1' or c == 'y' or c == 't': return True
    elif c == '0' or c == 'n' or c == 'f': return False
    return None

class ScenarioDB:
    '''Provides scenaraio matching and manual sample placement for 
    inversion purposes
    '''
    verbosity = 0
    # Filenames
    dbFilename = None
    nodeLibFilename = None
    scenarioLibFilename = None
    resultsFilename = None
    remainScenariosFilename = None
    # Main database objects
    nodesByScenario = dict() # firstTime = nodesByScenario[sID][node]
    scenariosByNode = dict() # firstTime = scenariosByNode[node][sID]
    remainingScenarios = []
    possibleInjectionPoints = dict()
    sampleResults = dict() # bool_detect = sampleResults[sID = node ',' time]
    hasBeenSampled = None
    nextToSample = None
    # Renaming numbers to indices
    nodeNameToID = dict() # nodeNameToID[node] = nidx
    nodeIdToName = dict() # nodeIdToName[nidx] = node
    scenarioIdToNode = dict() # scenarioIdToNode[sidx] = node_inserted
    scenarioIdToTime = dict() # scenarioIdToTime[sidx] = time_inserted
    scenarioNameToID = dict() # scenarioNameToID[ sID = node ',' time ] = sidx
    # Counts and booleans
    nNodes = 0 
    nScenarios = 0
    init = False
    start = None
    samenode = True
    algChoice = 'decorrel'
    outputResultsFile = True
    initialDetectTime = None
    initialDetectNode = None
    currentTime = None
    interactive = False
    
    def __init__(self, options=None):
        self.load_options(options);
        return

    def __str__(self):
        return (('<ScenarioDB: %d scenarios remaining' +
                 ' at %d possible injection nodes>') % 
                (len(self.remainingScenarios),
                 len(slef.possibleInjectionPoints)))
    
    def load_options( self , options ):
        '''Load from the command line options'''
        if options is None: return
        self.dbFilename = options.timedat
        self.nodeLibFilename = options.nlibfile
        self.scenarioLibFilename = options.slibfile
        self.resultsFilename = options.resfileout
        self.remainScenariosFilename = options.pnfileout
        self.interactive = options.interactive
        return
    
    def load_libraries( self ):
        '''Read in the scenario and node name identity libraries'''
        if self.nodeLibFilename is not None:
            nNodes = 0
            try:
                hashf = open(self.nodeLibFilename,'r')
                for ln in hashf:
                    ln = ln.strip()
                    ln_s = ln.split(',')
                    nodeName = ln_s[0].lower().strip()
                    nodeID = ln_s[1].lower().strip()
                    self.nodeNameToID[nodeName] = nodeID
                    self.nodeIdToName[nodeID] = nodeName
                    nNodes = nNodes + 1
            except IOError:
                print "Unable to read file: %s" % (self.nodeLibFilename)
                sys.exit(1)
            finally:
                self.nNodes = nNodes
                hashf.close()
        if self.verbosity > 0:
            print " Read %d nodes from %s" % \
                  (self.nNodes, self.nodeLibFilename)
        self.hasBeenSampled = np.zeros(self.nNodes, dtype=np.bool)
        if self.scenarioLibFilename is not None:
            nScenarios = 0
            try:
                shash = open(self.scenarioLibFilename,'r')
                for ln in shash:
                    ln = ln.strip()
                    ln_s = ln.split(',')
                    nId = ln_s[0].lower()
                    tId = ln_s[1].lower()
                    sId = ln_s[2].lower()
                    sName = ','.join([nId, tId])
                    self.scenarioIdToNode[sId] = nId
                    self.scenarioIdToTime[sId] = tId
                    self.scenarioNameToID[sName] = sId
                    nScenarios = nScenarios + 1
            except IOError:
                print "Unable to read file: %s" % (self.scenarioLibFilename)
                sys.exit(1)
            finally:
                self.nScenarios = nScenarios
                shash.close()
        if self.verbosity > 0:
            print (" Read %d scenarios from %s" %
                   (self.nScenarios, self.scenarioLibFilename))
        self.nodesByScenario = dict.fromkeys(self.scenarioNameToID.keys(),None)
        self.scenariosByNode = dict.fromkeys(self.nodeNameToID.keys(),None)
        for key in self.nodesByScenario.keys():
            self.nodesByScenario[key] = dict()
        for key in self.scenariosByNode.keys():
            self.scenariosByNode[key] = dict()
        return
        
    def make_database( self , bookshelf=None ):
        '''Load the database of scenario/node/contamination times
        information into a shelve object
        '''
        try:
            if self.scenariosByNode is None:
                self.scenariosByNode = dict.fromkeys(
                    self.nodeNameToID.keys(), None)
                for key in self.scenariosByNode.keys():
                    self.scenariosByNode[key] = dict()
            if self.nodesByScenario is None:
                self.nodesByScenario = dict.fromkeys(
                    self.scenarioNameToID.keys(), None)
                for key in self.nodesByScenario.keys():
                    self.nodesByScenario[key] = dict()
            tfile = open(self.dbFilename,'r')
            sCount = 0
            lastScenario = None
            lastSName = None
            print 'Starting to read: %s' % (self.dbFilename)
            for line in tfile:
                line = line.strip()
                line_s = line.split(',')
                curScenario = line_s[1].strip()
                curNode = line_s[0].strip()
                firstTime = int(line_s[2].strip())
                curSName = (self.scenarioIdToNode[curScenario] + ',' +
                            self.scenarioIdToTime[curScenario])
                curNName = self.nodeIdToName[curNode]
                if not (lastScenario == curScenario):
                    sCount = sCount + 1
                lastScenario = curScenario
                lastSName = curSName
                self.nodesByScenario[curSName][curNName] = firstTime
                self.scenariosByNode[curNName][curSName] = firstTime
                if sCount % 1000 == 0:
                    if self.verbosity < 3:
                        print ('\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b' +
                               '\b\b\b\b\b\b\b\b\b\b\b\b\b'),
                    print ("[ %8d: %3d%% ]" %
                           (sCount,
                            int(float(100.0*sCount)/self.nScenarios))),
                    print
            print " Completed reading from %s" % ( self.dbFilename )
        except KeyboardInterrupt:
            print " User requested cancel in database read. Stopping read"
            print " Cancelled reading from %s" % (self.dbFilename)
        except IOError:
            print "Unable to read file: %s" % (self.dbFilename)
            sys.exit(1)
        finally:
            tfile.close()
        return
    
    def set_possible_nodes_times(self):
        self.possibleInjectionPoints.clear()
        for sid in self.remainingScenarios:
            (nid, tid) = sid.split(',')
            if self.possibleInjectionPoints.has_key(nid):
                self.possibleInjectionPoints[nid].append(int(tid))
            else: self.possibleInjectionPoints[nid] = [ int(tid) ]
        return

    def print_possible_nodes_times(self, filename=None):
        fout = sys.stdout
        if filename is not None:
            try:
                fout = open(filename,'w')
            except IOError:
                filename = None
                fout = sys.stdout
        print >> fout, " Possible scenarios:"
        for (nde , times) in self.possibleInjectionPoints.items():
            print >> fout, '  ' + nde + ' @ ' + times.__str__()
        if filename is not None: fout.close()
        return

    def add_result(self , nde, time, detect=False, ct=0):
        detid = "%s,%d" % (nde, time)
        nidx = int(self.nodeNameToID[nde])
        self.sampleResults[detid] = detect
        self.hasBeenSampled[nidx] = True
        if self.currentTime is None: self.currentTime = time
        if self.currentTime < time: self.currentTime = time
        if ct == 0:
            self.remainingScenarios = self.scenariosByNode[nde].keys()
        return

    def get_remaining_scenarios( self ):
        '''Cycle through the list of remaining scenarios and find all those
        that still apply.
        '''
        remain = dict.fromkeys(self.remainingScenarios,False)
        for (detid, detect) in self.sampleResults.items():
            (nde, time) = detid.split(',')
            sTime = int(time)
            remain2 = remain.keys()
            for sid in remain2:
                if not self.scenariosByNode[nde].has_key(sid) and detect:
                    del(remain[sid])
            for sid in self.scenariosByNode[nde].keys():
                (inj, iTime) = sid.split(',')
                iTime = int(iTime)
                fTime = self.nodesByScenario[sid][nde]
                if self.verbosity > 1: print sid, nde, fTime, detect
                if remain.has_key(sid):
                    if iTime > self.currentTime or iTime > sTime:
                        del(remain[sid])
                    elif sTime < fTime and detect:
                        del(remain[sid])
                    elif sTime < fTime and not detect:
                        remain[sid] = True
                    elif sTime >= fTime and detect:
                        remain[sid] = True
                    else:
                        del(remain[sid])
            if len(remain) < 1:
                print ' *** ERROR in get_remaining_scenarios ***'
                print '   Are you sure that all samples were entered correctly?'
        if len(remain) == 1:
            print ' *** SCENARIO IDENTIFIED ***'
            print remain.keys()
        self.remainingScenarios = remain.keys()
        if self.verbosity > 1: print self.remainingScenarios
        return 
        
    def __load_samples_file( self , resultsFilename=None ):
        if resultsFilename is None:
            resultsFilename = self.resultsFilename
        try:
            ct = 0
            print 'Reading sample results from: %s' % (self.resultsFilename)
            rfile = open(resultsFilename,'r')
            for line in rfile:
                line = line.strip()
                line_s = line.split(',')
                nodeName = line_s[0].strip().lower()
                if not self.nodeNameToID.has_key(nodeName) and \
                        not self.nodeIdToName.has_key(nodeName):
                    print '  *** Location "%s" does not exist!' % nodeName
                    continue
                nde = nodeName
                detect = merlionBool(line_s[1])
                time = int(line_s[2].strip())
                if self.currentTime is None:
                    self.currentTime = time
                elif self.currentTime < time:
                    self.currentTime = time
                self.add_result(nde, time, detect, ct)
                ct = ct + 1
            print 'Completed reading from: %s' % (self.resultsFilename)
        except IOError:
            print "Unable to read result-set file: %s" % (resultsFilename)
        except TypeError:
            pass
        finally:
            rfile.close()
        return

    def load_samples(self, mode=None):
        if mode is None:
            if self.interactive: mode = 'user'
            else: mode = 'file'
        else: mode = mode.lower()
        if mode == 'file':
            self.__load_samples_file()
        elif mode == 'user':
            gui.load_samples(self)
        else:
            print ' *** Unknown mode: %s' % ( mode )
        return

    def find_next_samples(self, numNodes=6):
        nScenarios = len(self.remainingScenarios)
        nNodes = self.nNodes
        tMat = np.zeros(shape=(nScenarios,nNodes),
                          dtype=np.int8)
        ct = 0
        for sid in self.remainingScenarios:
            for node in self.nodesByScenario[sid].keys():
                nidx = int(self.nodeNameToID[node])
                tMat[ct,nidx] = (self.nodesByScenario[sid][node] <=
                                 self.currentTime + 1)
            ct = ct + 1
        tMat = tMat.T # convert so that corMat[node,scenario]
        hasNotSamp = np.logical_not(self.hasBeenSampled)
        numDetect = np.sum(tMat,1)
        # numDetect[self.hasBeenSampled] = 0
        numDetect = numDetect / float(ct)
        pScen = 4 * (numDetect) * (1 - numDetect)
        possibleNodes = np.where(pScen > 0)[0]
        order = np.argsort(pScen)
        order = order[-1::-1]
        order = order[0:np.sum(pScen>0)]
        if len(order) < 1:
            print ' *** No further sampling required ***'
            self.nextToSample = []
            return
        pMat = tMat[order]
        toSelect = [0]
        nodeSelect = [order[0]]
        while len(nodeSelect) < np.min(int(numNodes),len(order)):
            if self.verbosity > 1: print "Selecting"
            xcov = np.ones(len(order))
            for ii in range(len(order)):
                curCov = np.corrcoef(pMat[ii],pMat[toSelect])[0,1:]
                xcov[ii] = np.sum(curCov,0)
            xcov[toSelect] = 999
            nextSen = np.where(xcov == np.min(xcov))[0][0]
            toSelect.append(nextSen)
            nodeSelect.append(order[nextSen])
        self.nextToSample = []
        for nidx in nodeSelect:
            self.nextToSample.append(self.nodeIdToName[nidx.__str__()])
        return

    def print_next_samples(self, filename=None):
        fout = sys.stdout
        if filename is not None:
            try:
                fout = open(filename,'w')
            except IOError:
                filename = None
                fout = sys.stdout
        print >> fout, 'At timestep %d, sample at:' % (self.currentTime + 1)
        for id in self.nextToSample:
            print >> fout, '   %s' % (id)
        if filename is not None: fout.close()
        return
