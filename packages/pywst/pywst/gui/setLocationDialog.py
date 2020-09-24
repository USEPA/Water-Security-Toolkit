# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'setLocationDialog.ui'
#
# Created: Tue Feb 15 08:13:52 2011
#      by: PyQt4 UI code generator 4.8.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_Dialog(object):
    def setupUi(self, Dialog):
        Dialog.setObjectName(_fromUtf8("Dialog"))
        Dialog.resize(400, 300)
        Dialog.setModal(True)
        self.buttonBox = QtGui.QDialogButtonBox(Dialog)
        self.buttonBox.setGeometry(QtCore.QRect(30, 240, 341, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.gridLayoutWidget = QtGui.QWidget(Dialog)
        self.gridLayoutWidget.setGeometry(QtCore.QRect(10, 80, 381, 151))
        self.gridLayoutWidget.setObjectName(_fromUtf8("gridLayoutWidget"))
        self.gridLayout = QtGui.QGridLayout(self.gridLayoutWidget)
        self.gridLayout.setMargin(0)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.setDefaultTevasim = QtGui.QPushButton(self.gridLayoutWidget)
        self.setDefaultTevasim.setObjectName(_fromUtf8("setDefaultTevasim"))
        self.gridLayout.addWidget(self.setDefaultTevasim, 1, 0, 1, 1)
        self.setDefaultTso2Impact = QtGui.QPushButton(self.gridLayoutWidget)
        self.setDefaultTso2Impact.setObjectName(_fromUtf8("setDefaultTso2Impact"))
        self.gridLayout.addWidget(self.setDefaultTso2Impact, 2, 0, 1, 1)
        self.setDefaultFuture = QtGui.QPushButton(self.gridLayoutWidget)
        self.setDefaultFuture.setEnabled(False)
        self.setDefaultFuture.setText(_fromUtf8(""))
        self.setDefaultFuture.setObjectName(_fromUtf8("setDefaultFuture"))
        self.gridLayout.addWidget(self.setDefaultFuture, 3, 0, 1, 1)
        self.setDefaultDakota = QtGui.QPushButton(self.gridLayoutWidget)
        self.setDefaultDakota.setObjectName(_fromUtf8("setDefaultDakota"))
        self.gridLayout.addWidget(self.setDefaultDakota, 0, 0, 1, 1)
        self.labelDakota = QtGui.QLabel(self.gridLayoutWidget)
        font = QtGui.QFont()
        font.setPointSize(8)
        self.labelDakota.setFont(font)
        self.labelDakota.setText(_fromUtf8(""))
        self.labelDakota.setWordWrap(True)
        self.labelDakota.setObjectName(_fromUtf8("labelDakota"))
        self.gridLayout.addWidget(self.labelDakota, 0, 1, 1, 1)
        self.labelTevasim = QtGui.QLabel(self.gridLayoutWidget)
        font = QtGui.QFont()
        font.setPointSize(8)
        self.labelTevasim.setFont(font)
        self.labelTevasim.setText(_fromUtf8(""))
        self.labelTevasim.setWordWrap(True)
        self.labelTevasim.setObjectName(_fromUtf8("labelTevasim"))
        self.gridLayout.addWidget(self.labelTevasim, 1, 1, 1, 1)
        self.labelTso2Impact = QtGui.QLabel(self.gridLayoutWidget)
        font = QtGui.QFont()
        font.setPointSize(8)
        self.labelTso2Impact.setFont(font)
        self.labelTso2Impact.setText(_fromUtf8(""))
        self.labelTso2Impact.setWordWrap(True)
        self.labelTso2Impact.setObjectName(_fromUtf8("labelTso2Impact"))
        self.gridLayout.addWidget(self.labelTso2Impact, 2, 1, 1, 1)
        self.labelFuture = QtGui.QLabel(self.gridLayoutWidget)
        self.labelFuture.setObjectName(_fromUtf8("labelFuture"))
        self.gridLayout.addWidget(self.labelFuture, 3, 1, 1, 1)
        self.label_5 = QtGui.QLabel(Dialog)
        self.label_5.setGeometry(QtCore.QRect(10, 10, 381, 41))
        self.label_5.setWordWrap(True)
        self.label_5.setObjectName(_fromUtf8("label_5"))

        self.retranslateUi(Dialog)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), Dialog.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), Dialog.reject)
        QtCore.QMetaObject.connectSlotsByName(Dialog)

    def retranslateUi(self, Dialog):
        Dialog.setWindowTitle(QtGui.QApplication.translate("Dialog", "Dialog", None, QtGui.QApplication.UnicodeUTF8))
        self.setDefaultTevasim.setText(QtGui.QApplication.translate("Dialog", "tevasim", None, QtGui.QApplication.UnicodeUTF8))
        self.setDefaultTso2Impact.setText(QtGui.QApplication.translate("Dialog", "tso2Impact", None, QtGui.QApplication.UnicodeUTF8))
        self.setDefaultDakota.setText(QtGui.QApplication.translate("Dialog", "Dakota", None, QtGui.QApplication.UnicodeUTF8))
        self.labelFuture.setText(QtGui.QApplication.translate("Dialog", "                                                               ", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("Dialog", "Please set the locations for the different executables used by CWaRK", None, QtGui.QApplication.UnicodeUTF8))

