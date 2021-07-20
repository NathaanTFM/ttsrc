"""
UI for object property control
"""
import wx
import os

from wx.lib.scrolledpanel import ScrolledPanel
from wx.lib.agw.cubecolourdialog import *
from direct.wxwidgets.WxSlider import *
from pandac.PandaModules import *
import ObjectGlobals as OG

class AnimFileDrop(wx.FileDropTarget):
    def __init__(self, editor):
        wx.FileDropTarget.__init__(self)
        self.editor = editor

    def OnDropFiles(self, x, y, filenames):
        obj = self.editor.objectMgr.findObjectByNodePath(base.direct.selected.last)
        if obj is None:
            return

        objDef = obj[OG.OBJ_DEF]
        if not objDef.actor:
            return

        objNP = obj[OG.OBJ_NP]

        for filename in filenames:
            name = os.path.basename(filename)
            animName = Filename.fromOsSpecific(filename).getFullpath()
            if name.endswith('.mb') or\
               name.endswith('.ma'):
                self.editor.convertMaya(animName, obj, isAnim=True)
                return

            if animName not in objDef.anims:
                objDef.anims.append(animName)

            objNP.loadAnims({name:animName})
            objNP.loop(name)
            obj[OG.OBJ_ANIM] = animName
            self.editor.ui.objectPropertyUI.updateProps(obj)
            
class ObjectPropUI(wx.Panel):
    """
    Base class for ObjectPropUIs,
    It consists of label area and ui area.
    """
    def __init__(self, parent, label):
        wx.Panel.__init__(self, parent)
        self.parent = parent
        self.label = wx.StaticText(self, label=label)
        self.uiPane = wx.Panel(self)
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.label)
        sizer.Add(self.uiPane, 1, wx.EXPAND, 0)
        self.SetSizer(sizer)

    def setValue(self, value):
        self.ui.SetValue(value)

    def getValue(self):
        return self.ui.GetValue()

    def bindFunc(self, inFunc, outFunc, valFunc = None):
        self.ui.Bind(wx.EVT_ENTER_WINDOW, inFunc)
        self.ui.Bind(wx.EVT_LEAVE_WINDOW, outFunc)
        if valFunc:
            self.ui.Bind(self.eventType, valFunc)

class ObjectPropUIEntry(ObjectPropUI):
    """ UI for string value properties """
    def __init__(self, parent, label):
        ObjectPropUI.__init__(self, parent, label)
        self.ui = wx.TextCtrl(self.uiPane, -1)
        self.eventType = wx.EVT_TEXT_ENTER
        self.Layout()

    def setValue(self, value):
        self.ui.SetValue(str(value))

class ObjectPropUISlider(ObjectPropUI):
    """ UI for float value properties """
    def __init__(self, parent, label, value, minValue, maxValue):
        ObjectPropUI.__init__(self, parent, label)
        self.ui = WxSlider(self.uiPane, -1, value, minValue, maxValue,
                           pos = (0,0), size=(140, -1),
                           style=wx.SL_HORIZONTAL | wx.SL_LABELS)
        self.ui.Enable()
        self.Layout()

    def bindFunc(self, inFunc, outFunc, valFunc = None):
        self.ui.Bind(wx.EVT_ENTER_WINDOW, inFunc)
        self.ui.Bind(wx.EVT_LEAVE_WINDOW, outFunc)
        self.ui.textValue.Bind(wx.EVT_ENTER_WINDOW, inFunc)
        self.ui.textValue.Bind(wx.EVT_LEAVE_WINDOW, outFunc)

        if valFunc:
            self.ui.bindFunc(valFunc)


class ObjectPropUISpinner(ObjectPropUI):
    """ UI for int value properties """
    def __init__(self, parent, label, value, minValue, maxValue):
        ObjectPropUI.__init__(self, parent, label)
        self.ui = wx.SpinCtrl(self.uiPane, -1, "", min=minValue, max=maxValue, initial=value)
        self.eventType = wx.EVT_SPIN
        self.Layout()


class ObjectPropUICheck(ObjectPropUI):
    def __init__(self, parent, label, value):
        ObjectPropUI.__init__(self, parent, label)
        self.ui = wx.CheckBox(self.uiPane, -1, "", size=(50, 30))
        self.setValue(value)
        self.eventType = wx.EVT_CHECKBOX
        self.Layout()


class ObjectPropUIRadio(ObjectPropUI):
    def __init__(self, parent, label, value, valueList):
        ObjectPropUI.__init__(self, parent, label)
        self.ui = wx.RadioBox(self.uiPane, -1, "", choices=valueList, majorDimension=1, style=wx.RA_SPECIFY_COLS)
        self.setValue(value)
        self.eventType = wx.EVT_RADIOBOX
        self.Layout()

    def setValue(self, value):
        self.ui.SetStringSelection(value)

    def getValue(self):
        return self.ui.GetStringSelection()


class ObjectPropUICombo(ObjectPropUI):
    def __init__(self, parent, label, value, valueList):
        ObjectPropUI.__init__(self, parent, label)
        self.ui = wx.Choice(self.uiPane, -1, choices=valueList)
        self.setValue(value)
        self.eventType = wx.EVT_CHOICE
        self.Layout()

    def setValue(self, value):
        self.ui.SetStringSelection(value)

    def getValue(self):
        return self.ui.GetStringSelection()

class ColorPicker(CubeColourDialog):
    def __init__(self, parent, colourData=None, style=CCD_SHOW_ALPHA, alpha = 255, updateCB=None, exitCB=None):
        self.updateCB=updateCB
        CubeColourDialog.__init__(self, parent, colourData, style)
        self.okButton.Hide()
        self.cancelButton.Hide()
        self._colour.alpha = alpha
        self.alphaSpin.SetValue(self._colour.alpha)
        self.DrawAlpha()
        if exitCB:
            self.Bind(wx.EVT_CLOSE, exitCB)

    def SetPanelColours(self):
        self.oldColourPanel.RefreshColour(self._oldColour)
        self.newColourPanel.RefreshColour(self._colour)
        if self.updateCB:
            self.updateCB(self._colour.r, self._colour.g, self._colour.b, self._colour.alpha)

class ObjectPropertyUI(ScrolledPanel):
    def __init__(self, parent, editor):
        self.editor = editor
        self.colorPicker = None
        self.lastColorPickerPos = None
        ScrolledPanel.__init__(self, parent)

        parentSizer = wx.BoxSizer(wx.VERTICAL)
        parentSizer.Add(self, 1, wx.EXPAND, 0)
        parent.SetSizer(parentSizer); parent.Layout()

        self.SetDropTarget(AnimFileDrop(self.editor))

    def clearPropUI(self):
        sizer = self.GetSizer()
        if sizer is not None:
            sizer.Remove(self.propPane)
            self.propPane.Destroy()
            self.SetSizer(None)
        self.Layout()
        self.SetupScrolling(self, scroll_y = True, rate_y = 20)

    def colorPickerExitCB(self, evt=None):
        self.lastColorPickerPos = self.colorPicker.GetPosition()
        self.colorPicker.Destroy()
        self.colorPicker = None

    def colorPickerUpdateCB(self, rr, gg, bb, aa):
        r = rr / 255.0
        g = gg / 255.0
        b = bb / 255.0
        a = aa / 255.0
        self.propCR.setValue(r)
        self.propCG.setValue(g)
        self.propCB.setValue(b)
        self.propCA.setValue(a)        

        self.editor.objectMgr.updateObjectColor(r, g, b, a)

    def onColorSlider(self, evt):
        r = float(self.editor.ui.objectPropertyUI.propCR.getValue())
        g = float(self.editor.ui.objectPropertyUI.propCG.getValue())
        b = float(self.editor.ui.objectPropertyUI.propCB.getValue())
        a = float(self.editor.ui.objectPropertyUI.propCA.getValue())

        if self.colorPicker:
            evtObj = evt.GetEventObject()
            if evtObj == self.propCR.ui or\
               evtObj == self.propCR.ui.textValue:
                self.colorPicker.redSpin.SetValue(r * 255)
                self.colorPicker.AssignColourValue('r', r * 255, 255, 0)
            elif evtObj == self.propCG.ui or\
                 evtObj == self.propCG.ui.textValue:
                self.colorPicker.greenSpin.SetValue(g * 255)
                self.colorPicker.AssignColourValue('g', g * 255, 255, 0)
            elif evtObj == self.propCB.ui or\
                 evtObj == self.propCB.ui.textValue:
                self.colorPicker.blueSpin.SetValue(b * 255)
                self.colorPicker.AssignColourValue('b', b * 255, 255, 0)
            else:
                self.colorPicker._colour.alpha = a * 255
                self.colorPicker.alphaSpin.SetValue(self.colorPicker._colour.alpha)
                self.colorPicker.DrawAlpha()

        self.editor.objectMgr.updateObjectColor(r, g, b, a)
        
    def openColorPicker(self, evt, colourData, alpha):
        if self.colorPicker:
            self.lastColorPickerPos = self.colorPicker.GetPosition()
            self.colorPicker.Destroy()

        self.colorPicker = ColorPicker(self, colourData, alpha=alpha, updateCB=self.colorPickerUpdateCB, exitCB=self.colorPickerExitCB)
        self.colorPicker.GetColourData().SetChooseFull(True)
        self.colorPicker.Show()
        if self.lastColorPickerPos:
            self.colorPicker.SetPosition(self.lastColorPickerPos)
        
    def updateProps(self, obj):
        self.clearPropUI()
        
        self.propPane = wx.Panel(self)
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(self.propPane, 1, wx.EXPAND, 0)
        self.SetSizer(mainSizer)

        self.nb = wx.Notebook(self.propPane, style=wx.NB_BOTTOM)
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.nb, 1, wx.EXPAND)
        self.propPane.SetSizer(sizer)

        self.transformPane = wx.Panel(self.nb, -1)
        self.nb.AddPage(self.transformPane, 'Transform')

        self.propX = ObjectPropUIEntry(self.transformPane, 'X')
        self.propY = ObjectPropUIEntry(self.transformPane, 'Y')
        self.propZ = ObjectPropUIEntry(self.transformPane, 'Z')

        self.propH = ObjectPropUISlider(self.transformPane, 'H', 0, 0, 360)
        self.propP = ObjectPropUISlider(self.transformPane, 'P', 0, 0, 360)
        self.propR = ObjectPropUISlider(self.transformPane, 'R', 0, 0, 360)

        self.propSX = ObjectPropUIEntry(self.transformPane, 'SX')
        self.propSY = ObjectPropUIEntry(self.transformPane, 'SY')
        self.propSZ = ObjectPropUIEntry(self.transformPane, 'SZ')

        transformProps = [self.propX, self.propY, self.propZ, self.propH, self.propP, self.propR,
                       self.propSX, self.propSY, self.propSZ]

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.AddMany(transformProps)
        self.transformPane.SetSizer(sizer)
        for transformProp in transformProps:
            transformProp.bindFunc(self.editor.objectMgr.onEnterObjectPropUI,
                                   self.editor.objectMgr.onLeaveObjectPropUI,
                                   self.editor.objectMgr.updateObjectTransform)

        self.lookPane = wx.Panel(self.nb, -1)
        self.nb.AddPage(self.lookPane, 'Look')

        objNP = obj[OG.OBJ_NP]
        objRGBA = obj[OG.OBJ_RGBA]
        self.propCR = ObjectPropUISlider(self.lookPane, 'CR', objRGBA[0], 0, 1)
        self.propCG = ObjectPropUISlider(self.lookPane, 'CG', objRGBA[1], 0, 1)
        self.propCB = ObjectPropUISlider(self.lookPane, 'CB', objRGBA[2], 0, 1)        
        self.propCA = ObjectPropUISlider(self.lookPane, 'CA', objRGBA[3], 0, 1) 
        colorProps = [self.propCR, self.propCG, self.propCB, self.propCA]

        for colorProp in colorProps:
            colorProp.ui.bindFunc(self.onColorSlider)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.AddMany(colorProps)
        button = wx.Button(self.lookPane, -1, 'Color Picker', (0,0), (140, 20))
        _colourData = wx.ColourData()
        _colourData.SetColour(wx.Colour(objRGBA[0] * 255, objRGBA[1] * 255, objRGBA[2] * 255))
        button.Bind(wx.EVT_BUTTON, lambda p0=None, p1=_colourData, p2=objRGBA[3] * 255: self.openColorPicker(p0, p1, p2))

        sizer.Add(button)

        if self.colorPicker:
            self.openColorPicker(None, _colourData, objRGBA[3] * 255)

        objDef = obj[OG.OBJ_DEF]

        if objDef.model is not None:
            propUI = ObjectPropUICombo(self.lookPane, 'model', obj[OG.OBJ_MODEL], objDef.models)
            sizer.Add(propUI)            

            propUI.bindFunc(self.editor.objectMgr.onEnterObjectPropUI,
                            self.editor.objectMgr.onLeaveObjectPropUI,
                            lambda p0=None, p1=obj: self.editor.objectMgr.updateObjectModelFromUI(p0, p1))

        if len(objDef.anims) > 0:
            propUI = ObjectPropUICombo(self.lookPane, 'anim', obj[OG.OBJ_ANIM], objDef.anims)
            sizer.Add(propUI)            

            propUI.bindFunc(self.editor.objectMgr.onEnterObjectPropUI,
                            self.editor.objectMgr.onLeaveObjectPropUI,
                            lambda p0=None, p1=obj: self.editor.objectMgr.updateObjectAnimFromUI(p0, p1))

        self.lookPane.SetSizer(sizer)

        self.propsPane = wx.Panel(self.nb, -1)
        self.nb.AddPage(self.propsPane, 'Props')
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        for key in objDef.properties.keys():
            propDef = objDef.properties[key]
            propType = propDef[OG.PROP_TYPE]
            propDataType = propDef[OG.PROP_DATATYPE]
            value = obj[OG.OBJ_PROP].get(key)

            if propType == OG.PROP_UI_ENTRY:
                propUI = ObjectPropUIEntry(self.propsPane, key)
                propUI.setValue(value)
                sizer.Add(propUI)

            elif propType == OG.PROP_UI_SLIDE:
                if len(propDef) <= OG.PROP_RANGE:
                    continue
                propRange = propDef[OG.PROP_RANGE]

                if value is None:
                    continue

                propUI = ObjectPropUISlider(self.propsPane, key, value, propRange[OG.RANGE_MIN], propRange[OG.RANGE_MAX])
                sizer.Add(propUI)

            elif propType == OG.PROP_UI_SPIN:
                if len(propDef) <= OG.PROP_RANGE:
                    continue
                propRange = propDef[OG.PROP_RANGE]

                if value is None:
                    continue

                propUI = ObjectPropUISpinner(self.propsPane, key, value, propRange[OG.RANGE_MIN], propRange[OG.RANGE_MAX])
                sizer.Add(propUI)                

            elif propType == OG.PROP_UI_CHECK:
                if value is None:
                    continue

                propUI = ObjectPropUICheck(self.propsPane, key, value)
                sizer.Add(propUI)                  

            elif propType == OG.PROP_UI_RADIO:
                if len(propDef) <= OG.PROP_RANGE:
                    continue
                propRange = propDef[OG.PROP_RANGE]
                
                if value is None:
                    continue

                if propDataType != OG.PROP_STR:
                    for i in range(len(propRange)):
                        propRange[i] = str(propRange[i])

                    value = str(value)

                propUI = ObjectPropUIRadio(self.propsPane, key, value, propRange)
                sizer.Add(propUI)

            elif propType == OG.PROP_UI_COMBO:
                if len(propDef) <= OG.PROP_RANGE:
                    continue
                propRange = propDef[OG.PROP_RANGE]                

                if value is None:
                    continue

                if propDataType != OG.PROP_STR:
                    for i in range(len(propRange)):
                        propRange[i] = str(propRange[i])

                    value = str(value)

                propUI = ObjectPropUICombo(self.propsPane, key, value, propRange)
                sizer.Add(propUI)

            else:
                # unspported property type
                continue

            propUI.bindFunc(self.editor.objectMgr.onEnterObjectPropUI,
                            self.editor.objectMgr.onLeaveObjectPropUI,
                            lambda p0=None, p1=obj, p2=key: self.editor.objectMgr.updateObjectProperty(p0, p1, p2))


        self.propsPane.SetSizer(sizer);
        self.Layout()
        self.SetupScrolling(self, scroll_y = True, rate_y = 20)
