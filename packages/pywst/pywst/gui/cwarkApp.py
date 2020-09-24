import os,sys

# Import Qt modules
from PyQt4 import QtCore,QtGui

import yaml

# Import the compiled UI module
from cwark.gui.cwarkUi import Ui_MainWindow
from cwark.gui.cwarkPrefs import Preferences
import cwark.flushing.flushingData as data

# Create a class for our main window
class Main(QtGui.QMainWindow):
    filelabel = None
    modified = False
    
    def __init__(self, options=None):
        QtGui.QMainWindow.__init__(self)
        self.cfg = data.Problem()
        if options is not None:
            if options.dakotaexe is not None:
                self.cfg.setOptimizationOption('dakotaExecutable',options.dakotaexe)
            if options.tso2Impactexe is not None:
                self.cfg.setOptimizationOption('tso2ImpactExecutable',options.tso2Impactexe)
            if options.tevasimexe is not None:
                self.cfg.setOptimizationOption('tevasimExecutable',options.tevasimexe)
        self.ui=Ui_MainWindow()
        self.ui.setupUi(self)
        self.filelabel = QtGui.QLabel()
        self.filelabel.setText("No file loaded")
        self.ui.statusBar.addPermanentWidget(self.filelabel)
        self.ui.displayProjectDirectory.setText(os.path.abspath('.'))
        if options is not None:
            if options.projectfile is not None:
                filename = options.projectfile
                self.cfg.setProjectOption('projectName',filename)
                if self.cfg.load(filename):
                    self.loadProjectData()
                    self.filelabel.setText(filename)
                    self.ui.displayProjectName.setText(self.cfg.getProjectOption('projectName'))
        return

    def on_actionSet_Defaults_triggered(self, checked=None):
        if checked is None: return
        self.setPreferences()
        return
        
    def on_setExecutableLocations_clicked(self, checked=None):
        if checked is None: return
        self.setPreferences()
        return
        
    def setPreferences(self):
        a = Preferences()
        a.exec_()
        if os.name in ['nt','win','win32','win64','dos']:
            # Need to add checks for APPDATA and LOCALAPPDATA and USERHOME
            rcPath = os.path.join(os.path.abspath(os.environ['LOCALAPPDATA']),
                                  '.cwarkrc')
        else:
            rcPath = os.path.join(os.path.abspath(os.environ['HOME']),
                                  '.cwarkrc')
        if os.path.exists(rcPath) and os.path.isfile(rcPath):
            fid = open(rcPath,'r')
            defLocs = yaml.load(fid)
            fid.close()
            self.cfg.setOptimizationOption('dakotaExecutable',defLocs['dakota'])
            self.cfg.setOptimizationOption('tevasimExecutable',defLocs['tevasim'])
            self.cfg.setOptimizationOption('tso2ImpactExecutable',defLocs['tso2Impact'])
        return
        

    # GUI Menu Bar Functions
    def on_actionOpen_Project_triggered(self, checked=None):
        if checked is None: return
        self.loadProject()
        return

    def on_actionSave_Project_triggered(self, checked=None):
        if checked is None: return
        self.saveProject()
        
    def on_actionNew_Project_triggered(self, checked=None):
        if checked is None: return
        self.newProject()
        
    def on_actionRun_triggered(self, checked=None):
        if checked is None: return
        self.runProject()

    # GUI Non-tabbed Functions
    def on_toolButtonFindProject_clicked(self,checked=None):
        if checked is None: return
        directory = QtGui.QFileDialog.getExistingDirectory(self,
            "Open project directory")
        if str(directory) is not "":
            self.updateProjectDirectory(str(directory))
        return
        
    def on_toolButtonFindDakota_clicked(self,checked=None):
        if checked is None: return
        filename=QtGui.QFileDialog.getOpenFileName(self, 
                "Choose dakota executable", "*", "*")
        if str(filename) is not "":
            self.ui.displayDakotaExecutable.setText(filename)
            dakotaExec= os.path.abspath(str(filename))
            self.cfg.setOptimizationOption('dakotaExecutable',dakotaExec)
        return


    # GUI General Settings Tab Functions
    def on_toolButtonFindInp_clicked(self,checked=None):
        if checked is None: return
        self.modified = True
        filename=QtGui.QFileDialog.getOpenFileName(self, 
                "Choose EPANET input file", "*", "*.inp")
        if str(filename) is not "":
            self.updateInpFile(str(filename))
        return
        
    def on_toolButtonFindTsg_clicked(self,checked=None):
        if checked is None: return
        self.modified = True
        filename=QtGui.QFileDialog.getOpenFileName(self, 
                "Choose scenario file", "*", "*.tsg;*.tsi")
        if str(filename) is not "":
            self.updateTsgFile(str(filename))
        return
    
    def on_toolButtonFindTai_clicked(self,checked=None):
        if checked is None: return
        self.modified = True
        filename=QtGui.QFileDialog.getOpenFileName(self, 
                "Choose health-impacts file", "*", "*.tai")
        if str(filename) is not "":
            self.updateTaiFile(str(filename))
        return
        
    def on_chooseImpactMetric_currentIndexChanged(self,value):
        if type(value) is not int: return
        self.modified = True
        self.ui.displayImpactMetric.setText(data.impactMeasures[value][1])
        metric = data.impactMeasures[value][0]
        self.cfg.setScenarioOption('impactMetric',metric)
        if value > data.healthImpacts:
            self.activateHealthImpacts()
        else:
            self.deactivateHealthImpacts()
        return
        
    def on_editTimeDetect_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.cfg.setScenarioOption('timeWhenDetected',value)
        return

    def on_editDelayFlush_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.cfg.setScenarioOption('prepTimeFlushing',value)
        return

    def on_editDelayClose_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.cfg.setScenarioOption('prepTimeClosing',value)
        return

    def on_editDurationFlush_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.cfg.setFlushingOption('duration',value)
        return

    def on_editDurationSim_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.cfg.setScenarioOption('simulationDuration',value)
        return

    def on_editFlushingRate_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.cfg.setFlushingOption('rate',value)
        return

    def on_editMaxFlush_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.cfg.setFlushingOption('maxNodesFlushed',value)
        return

    def on_editMaxClose_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.cfg.setFlushingOption('maxLinksClosed',value)
        return
    
    # GUI Network Settings Tab Functions
    def on_radioNodesAll_toggled(self,checked=None):
        if checked is None: return
        self.modified = True
        self.updateNodesGroup()
        self.updateNodeList()

    def on_radioNodesNZD_toggled(self,checked=None):
        if checked is None: return
        self.modified = True
        self.updateNodesGroup()
        self.updateNodeList()

    def on_radioNodesList_toggled(self,checked=None):
        if checked is None: return
        self.modified = True
        self.updateNodesGroup()
        self.updateNodeList()

    def on_radioNodesNone_toggled(self,checked=None):
        if checked is None: return
        self.modified = True
        self.updateNodesGroup()
        self.updateNodeList()

    def on_radioPipesAll_toggled(self,checked=None):
        if checked is None: return
        self.modified = True
        self.updatePipesGroup()
        self.updatePipeList()
    
    def on_radioPipesByDiameter_toggled(self,checked=None):
        if checked is None: return
        self.modified = True
        self.updatePipesGroup()
        self.updatePipeList()
    
    def on_radioPipesList_toggled(self,checked=None):
        if checked is None: return
        self.modified = True
        self.updatePipesGroup()
        self.updatePipeList()
    
    def on_radioPipesNone_toggled(self,checked=None):
        if checked is None: return
        self.modified = True
        self.updatePipesGroup()
        self.updatePipeList()
    
    def on_btnUpdateNodes_clicked(self, checked=None):
        if checked is None: return
        self.modified = True
        self.updateNodeList()

    def on_btnUpdatePipes_clicked(self, checked=None):
        if checked is None: return
        self.modified = True
        self.updatePipeList()

    def on_btnLoadNetData_clicked(self,checked=None):
        if checked is None: return
        self.modified = True
        self.loadNetworkData()
        
    def on_editMinPipeDiam_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.modified = True
        self.cfg.setCloseableLinks('diam',minDiam=value)
        self.updatePipeList()
        return

    def on_editMaxPipeDiam_valueChanged(self,value):
        if type(value) is not float and type(value) is not int: return
        self.modified = True
        self.cfg.setCloseableLinks('diam',maxDiam=value)
        self.updatePipeList()
        return

    def on_editNodeList_textChanged(self):
        self.modified = True
        
    def on_editPipeList_textChanged(self):
        self.modified = True
    
    # GUI Dakota Settings Tab Functions
    def on_btnRestoreDakotaDefaults_clicked(self, checked=None):
        if checked is None: return
        self.modified = True
        self.cfg.setOptimizationOption('dakotaMethod',data.dakota_method_default)
        self.ui.textEditDakotaFile.setPlainText(data.dakota_method_default)
        return
        
    def on_textEditDakotaFile_textChanged(self):
        self.modified = True
        self.cfg.setOptimizationOption('dakotaMethod',
                                       str(self.ui.textEditDakotaFile.toPlainText()))
        return
        
    def on_checkBoxVerbose_clicked(self,checked=None):
        if checked is None: return
        if checked:
            self.cfg.setOptimizationOption('verboseOutput',True)
        else:
            self.cfg.setOptimizationOption('verboseOutput',True)
        return
    
    # GUI Results Display Tab Functions
    
    
    # Processing Functions
        
    def updateProjectDirectory(self, directory):
        self.ui.displayProjectDirectory.setText(directory)
        os.chdir(str(directory))
        self.cfg.setProjectOption('runDirectory',str(directory))
        self.updateInpFile(self.cfg.getNetworkOption('epanetInputFile'))
        self.updateTsgFile(self.cfg.getScenarioOption('scenarioFile'))
        return

    def loadProject(self):
        filename=QtGui.QFileDialog.getOpenFileName(self, 
                "Choose project configuration file", "*", "*.yaml")
        if str(filename) is not "":
            directory = os.path.dirname(str(filename))
            os.chdir(directory)
            if self.cfg.load(str(filename)):
                self.cfg.validate()
                self.filelabel.setText(filename)
                self.loadProjectData()
                self.ui.displayProjectName.setText(self.cfg.getProjectOption('projectName'))
        return
    
    def saveProject(self):
        filename = str(self.filelabel.text())
        if filename == "No file loaded":
            filename = "new_flushing_project"
        filename = QtGui.QFileDialog.getSaveFileName(self,
                "Save project as", filename, "*.yaml")
        if str(filename) is "": return
        self.cfg.filename = filename
        self.cfg.setProjectOption('projectName',str(filename))
        self.saveProjectData()
        self.cfg.validate()
        self.cfg.save()
        self.cfg.createDakotaInput()
        self.cfg.createDriverScripts()
        self.modified = False
        self.ui.displayProjectName.setText(self.cfg.getProjectOption('projectName'))
        return
        
    def newProject(self):
        self.cfg = data.Problem()
        self.epanetData.ENclose()
        self.filelabel.setText('No file loaded')
        self.loadProjectData()
        self.modified = True
        return
    
    def runProject(self):
        filename = str(self.filelabel.text())
        if filename == 'No file loaded':
            filename = ''
        self.saveProjectData()
        self.ui.plainTextEditResultsFile.setPlainText('Running ...')
        self.ui.statusBar.showMessage('Running Dakota')
        self.cfg.run(addDate=True)
        self.ui.plainTextEditResultsFile.setPlainText(yaml.dump(self.cfg.results, default_flow_style=False))
        self.ui.statusBar.clearMessage()
    
    def updateInpFile(self, filename):
        if filename is '':
            self.ui.displayInpFile.setText('')
            self.cfg.setNetworkOption('epanetInputFile','')
            return
        self.ui.displayInpFile.setText(os.path.relpath(filename))
        self.cfg.setNetworkOption('epanetInputFile',os.path.relpath(filename))
        self.ui.displayEpanetFile.setText('<unloaded> '+filename)
        self.ui.btnLoadNetData.setEnabled(True)
        self.loadNetworkData()
        return
        
    def updateTsgFile(self, filename):
        if filename is '':
            self.ui.displayTsgFile.setText('')
            self.cfg.setScenarioOption('scenarioFile','')
            return
        self.ui.displayTsgFile.setText(os.path.relpath(filename))
        self.cfg.setScenarioOption('scenarioFile',os.path.relpath(filename))
        return

    def updateTaiFile(self, filename):
        if filename is '': 
            self.ui.displayTaiFile.setText('')
            self.cfg.setScenarioOption('healthImpactsFile','')
            return
        self.ui.displayTaiFile.setText(os.path.relpath(filename))
        self.cfg.setScenarioOption('healthImpactsFile',os.path.relpath(filename))
        return
        
    def activateHealthImpacts(self):
        self.ui.toolButtonFindTai.setEnabled(True)
        # self.ui.lbl_toolButtonFileTai.setEnabled(True)
        self.ui.displayTaiFile.setEnabled(True)
        return
        
    def deactivateHealthImpacts(self):
        self.ui.toolButtonFindTai.setDisabled(True)
        # self.ui.lbl_toolButtonFileTai.setDisabled(True)
        self.ui.displayTaiFile.setDisabled(True)
        return
        
    def updateNodesGroup(self):
        if self.ui.radioNodesList.isChecked():
            self.ui.editNodeList.setEnabled(True)
        else:
            self.ui.editNodeList.setDisabled(True)
        if self.ui.radioNodesList.isChecked():
            self.cfg.setFlushableNodes('list')
        elif self.ui.radioNodesAll.isChecked():
            self.cfg.setFlushableNodes('all')
        elif self.ui.radioNodesNZD.isChecked():
            self.cfg.setFlushableNodes('nzd')
        else:
            self.cfg.setFlushableNodes('none')
        return
        
    def updatePipesGroup(self):
        if self.ui.radioPipesList.isChecked():
            self.ui.editPipeList.setEnabled(True)
        else:
            self.ui.editPipeList.setDisabled(True)
        if self.ui.radioPipesByDiameter.isChecked():
            self.ui.editMinPipeDiam.setEnabled(True)
            self.ui.editMaxPipeDiam.setEnabled(True)
            self.ui.lbl_editMinPipeDiam.setEnabled(True)
            self.ui.lbl_editMaxPipeDiam.setEnabled(True)
        else:
            self.ui.editMinPipeDiam.setDisabled(True)
            self.ui.editMaxPipeDiam.setDisabled(True)
            self.ui.lbl_editMinPipeDiam.setDisabled(True)
            self.ui.lbl_editMaxPipeDiam.setDisabled(True)
        if self.ui.radioPipesList.isChecked():
            self.cfg.setCloseableLinks('list')
        elif self.ui.radioPipesAll.isChecked():
            self.cfg.setCloseableLinks('all')
        elif self.ui.radioPipesByDiameter.isChecked():
            self.cfg.setCloseableLinks('diam')
        else:
            self.cfg.setCloseableLinks('none')
        return
        
    def loadNetworkData(self,autoUpdate=True):
        filename = self.cfg.getNetworkOption('epanetInputFile')
        try:
            fid = open(filename,'r')
            fid.close()
        except:
            self.ui.displayEpanetFile.setText("<failed to locate> "+filename)
            return
        self.ui.displayEpanetFile.setText("<loaded> "+filename)
        if autoUpdate:
            self.updatePipeList()
            self.updateNodeList()
            self.setPipeGroup()
            self.setNodeGroup()
        self.ui.btnUpdateNodes.setEnabled(True)
        self.ui.btnUpdatePipes.setEnabled(True)
        return
    
    def setPipeGroup(self):
        mthdPipes = self.cfg.getCloseableLinksMethod()
        if mthdPipes.lower() == 'list': 
            self.ui.radioPipesList.setChecked(True)
        elif mthdPipes.lower() == 'none':
            self.ui.radioPipesNone.setChecked(True)
        elif mthdPipes.lower() == 'all':
            self.ui.radioPipesAll.setChecked(True)
        elif mthdPipes.lower() == 'diam':
            self.ui.radioPipesByDiameter.setChecked(True)
        return
    
    def updatePipeList(self):
        linklist = self.cfg.getCloseableLinksList()
        (pMin, pMax) = self.cfg.getCloseableLinksDiameters()
        self.ui.editMinPipeDiam.setValue(pMin)
        self.ui.editMaxPipeDiam.setValue(pMax)
        self.ui.editPipeList.clear()
        for item in linklist:
            self.ui.editPipeList.appendPlainText(str(item))
        return

    def setNodeGroup(self):
        mthdPipes = self.cfg.getFlushableNodesMethod()
        if mthdPipes.lower() == 'list':
            self.ui.radioNodesList.setChecked(True)
        elif mthdPipes.lower() == 'none':
            self.ui.radioNodesNone.setChecked(True)
        elif mthdPipes.lower() == 'all':
            self.ui.radioNodesAll.setChecked(True)
        elif mthdPipes.lower() == 'nzd':
            self.ui.radioNodesNZD.setChecked(True)
        return

    def updateNodeList(self):
        list = self.cfg.getFlushableNodesList()
        self.ui.editNodeList.clear()
        for item in list:
            self.ui.editNodeList.appendPlainText(str(item))
        return
        
    def saveProjectData(self):
        self.cfg.validate()
        self.cfg.save()
        return
        
    def loadProjectData(self):
        #self.ui.displayProjectFilename.setText(self.cfg.filename)
        directory = self.cfg.getProjectOption('runDirectory')
        if not os.path.isdir(directory): 
            pass # TODO: Ask to update/create run directory if does not exist
        self.updateProjectDirectory(str(directory))
        self.updateInpFile(self.cfg.getNetworkOption('epanetInputFile'))
        self.ui.displayTsgFile.setText(self.cfg.getScenarioOption('scenarioFile'))
        self.ui.displayTaiFile.setText(self.cfg.getScenarioOption('healthImpactsFile'))
        metric = self.cfg.getScenarioOption('impactMetric')
        idx = 0
        for line in data.impactMeasures:
            if line[0] == metric:
                self.ui.chooseImpactMetric.setCurrentIndex(idx)
                self.ui.displayImpactMetric.setText(line[1])
            idx = idx+1
        self.ui.editTimeDetect.setValue(self.cfg.getScenarioOption('timeWhenDetected'))
        self.ui.editDelayFlush.setValue(self.cfg.getScenarioOption('prepTimeFlushing'))
        self.ui.editDelayClose.setValue(self.cfg.getScenarioOption('prepTimeClosing'))
        self.ui.editDurationFlush.setValue(self.cfg.getFlushingOption('duration'))
        self.ui.editDurationSim.setValue(self.cfg.getScenarioOption('simulationDuration'))
        self.ui.editFlushingRate.setValue(self.cfg.getFlushingOption('rate'))
        self.ui.editMaxFlush.setValue(self.cfg.getFlushingOption('maxNodesFlushed'))
        self.ui.editMaxClose.setValue(self.cfg.getFlushingOption('maxLinksClosed'))
        
        self.loadNetworkData()
        self.ui.textEditDakotaFile.setPlainText(self.cfg.getOptimizationOption('dakotaMethod'))
        
def main(options=None):
    # Again, this is boilerplate, it's going to be the same on
    # almost every app you write
    app = QtGui.QApplication(sys.argv)
    window=Main(options)
    window.show()
    
    # It's exec_ because exec is a reserved word in Python
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
