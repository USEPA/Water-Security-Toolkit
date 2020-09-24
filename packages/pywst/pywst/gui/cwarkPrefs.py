
from PyQt4 import QtGui
import yaml, os, sys
from setLocationDialog import Ui_Dialog


class Preferences(QtGui.QDialog):
    defLocs = { 'dakota': 'dakota',
                'tevasim': 'tevasim',
                'tso2Impact': 'tso2Impact' }
    
    def __init__(self):
        QtGui.QDialog.__init__(self)
        self.ui=Ui_Dialog()
        self.ui.setupUi(self)
        if os.name in ['nt','win','win32','win64','dos']:
            rcPath = os.path.join(os.path.abspath(os.environ['APPDATA']),
                                  '.cwarkrc')
        else:
            rcPath = os.path.join(os.path.abspath(os.environ['HOME']),
                                  '.cwarkrc')
        if os.path.exists(rcPath) and os.path.isfile(rcPath):
            fid = open(rcPath,'r')
            defLocs = yaml.load(fid)
            fid.close()
            self.defLocs = defLocs
        self.ui.labelDakota.setText(self.defLocs['dakota'])
        self.ui.labelTevasim.setText(self.defLocs['tevasim'])
        self.ui.labelTso2Impact.setText(self.defLocs['tso2Impact'])
        return
        
    def on_setDefaultDakota_clicked(self, checked=None):
        if checked is None: return
        fileInfo = os.path.split(self.defLocs['dakota'])
        filename=QtGui.QFileDialog.getOpenFileName(self, 
                "Choose dakota executable", fileInfo[0], "*")
        if str(filename) == '': return
        self.defLocs['dakota'] = str(filename)
        self.ui.labelDakota.setText(self.defLocs['dakota'])
        return
    
    def on_setDefaultTevasim_clicked(self, checked=None):
        if checked is None: return
        fileInfo = os.path.split(self.defLocs['tevasim'])
        filename=QtGui.QFileDialog.getOpenFileName(self, 
                "Choose tevasim executable", fileInfo[0], "*")
        if str(filename) == '': return
        self.defLocs['tevasim'] = str(filename)
        self.ui.labelTevasim.setText(self.defLocs['tevasim'])
        return
    
    def on_setDefaultTso2Impact_clicked(self, checked=None):
        if checked is None: return
        fileInfo = os.path.split(self.defLocs['tso2Impact'])
        filename=QtGui.QFileDialog.getOpenFileName(self, 
                "Choose tso2Impact executable", fileInfo[0], "*")
        if str(filename) == '': return
        self.defLocs['tso2Impact'] = str(filename)
        self.ui.labelTso2Impact.setText(self.defLocs['tso2Impact'])
        return
    
    def on_buttonBox_accepted(self):
        if os.name in ['nt','win','win32','win64','dos']:
            rcPath = os.path.join(os.path.abspath(os.environ['APPDATA']),
                                  '.cwarkrc')
        else:
            rcPath = os.path.join(os.path.abspath(os.environ['HOME']),
                                  '.cwarkrc')
        fid = open(rcPath,'w')
        fid.write(yaml.dump(self.defLocs))
        fid.close()
        return
        