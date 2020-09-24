#!/usr/local/bin/python
#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

'''This program takes in .csv files of a particular format
containing simulated water network information and extracts 
all incidents of contamination that are higher than a defined
threshold. Output files that can be used in the program
scenmatch.py are created in the process. Scenarios are also
assumed to have unique source-of-contamination node and initial
time of contamination combinaitons.
'''

import sys
import os
from sys import stdin, stdout, stderr
from optparse import OptionParser

#Create options:
parser = OptionParser()
parser.add_option('--dlim','-d', dest = 'dlim', default = 0) #Option that specifies the contamination threshold
(options, args) = parser.parse_args()

#Make a list of all files in the directory that have the extension ".csv"
#!!!This is why all ".csv" files must be the correct format!!!
lst = [os.path.normcase(f) for f in os.listdir(".")]
lst = [os.path.join(".", f) for f in lst if os.path.splitext(f)[1] in ".csv"]
cnt = 0 #Used to index the nodes
hash = dict() #Dictionary to keep track of nodes and their indices
shash = dict()#Dictionary to keep track of scenarios and their indices
dlim = float(parser.values.dlim) #Set contamination threshold
#Open files for output.
timef = open('events.dat','w')
hashf = open('nodes.dat','w')
scenf = open('scenarios.dat','w')

hflag = 0 #Used to see if the node dictionary has been created or not
fcount = 1 #Used to keep track of how many files have been read
scount = 0 #Used to index the scenarios

#Go through each ".csv" file and extract the important information
for n in lst:
    
    print fcount #Output how many files have been read.
    fcount = fcount + 1
    file = open(n, 'r') #Open ".csv" file
    
    ln1 = file.readline() #Extract the first line
    ln1 = ln1.rstrip() #Remove the return character
    ln1_s = ln1.split(',') #Split items that are separated by a comma
    tnum = int((float(ln1_s[2]) - float(ln1_s[1])) / float(ln1_s[3]) + 1) #Determine number of timesteps in the file

    ln2 = file.readline() #Extract the second line
    ln2 = ln2.rstrip() #Remove the return character
    ln2_s = ln2.split(',') #Split items that are separated by a comma
    snname = ln2_s[0].lower() #Save the name of the node where contamination was inserted into the network in the simulation
    stime = ln2_s[2] #Save the initial time where contamination was inserted into the network in the simulation
    shash[','.join([snname,stime])] = str(scount) #Index the scenario
    scenf.write(','.join([snname,stime,str(scount)])) #Write the scenario and index to file
    scenf.write('\n') #End line in file
    scount = scount + 1 #Increase scenario count
    dumln = file.readline() #Third line in file not needed
    
    #Create the dictionary for the nodes if not already created
    if hflag == 0:
        hflag = 1 #Flag that the file has been created
        for line in file.readlines():
            line = line.rstrip()
            line_s = line.split(',')
            nname = line_s[0].lower()
            hash[nname] = cnt
            tp = ','.join([nname, str(cnt)]) #Create line to write to file
            hashf.write(tp) #Write index information to file
            hashf.write('\n') #End line in file
            cnt += 1 #Increase index

    file.seek(0) #Go back to the beginning of the file
    for j in range(3):
        file.readline() #Read in three "dummy" lines

    for line in file.readlines():
        line = line.rstrip()
        line_s = line.split(',')
        nname = line_s[0].lower() 
        flg = 0
        to_printt = ','.join([str(hash[nname]), str(shash[','.join([snname,stime])])]) #Create line to write
        #Go through each time step in the line
        for i in range(tnum):
            if float(line_s[i+1]) > dlim:
                flg = 1 #Mark that the line will be written to file
                to_printt = ','.join([to_printt, str(i-1)]) #If the reading is above the threshold, append to line
        #If the line has instances of contamination in it, print it to a file
        if flg == 1:
            timef.write(to_printt) 
            timef.write('\n')
            
            
#Create a readme file that shows the formatting of the files created
readf = open('readme.txt','w')
readf.write('Format for events.dat:\n')
readf.write('Node Number, Scenario Number, Time Steps for Each Concentration Reading > 0 at Node (Separated by Commas)\n\n')
readf.write('Format for nodes.dat:\n')
readf.write('Node Name, Node Number\n\n')        
readf.write('Format for senarios.dat:\n')
readf.write('Node Name, Initial Time of Contamination, Scenario Number\n\n')        
