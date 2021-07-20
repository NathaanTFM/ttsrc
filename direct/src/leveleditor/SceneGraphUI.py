"""
Defines Scene Graph tree UI
"""
import wx
import cPickle as pickle
from pandac.PandaModules import *
from ActionMgr import *

import ObjectGlobals as OG

class SceneGraphUIDropTarget(wx.TextDropTarget):
    def __init__(self, editor):
        print "in SceneGraphUIDropTarget::init..."
        wx.TextDropTarget.__init__(self)
        self.editor = editor

    def OnDropText(self, x, y, text):
        print "in SceneGraphUIDropTarget::OnDropText..."
        self.editor.ui.sceneGraphUI.changeHierarchy(text, x, y)
        
class SceneGraphUI(wx.Panel):
    def __init__(self, parent, editor):
        wx.Panel.__init__(self, parent)

        self.editor = editor
        self.tree = wx.TreeCtrl(self, id=-1, pos=wx.DefaultPosition,
                  size=wx.DefaultSize, style=wx.TR_MULTIPLE|wx.TR_DEFAULT_STYLE,
                  validator=wx.DefaultValidator, name="treeCtrl")
        self.root = self.tree.AddRoot('render')
        self.tree.SetItemPyData(self.root, "render")

        self.shouldShowPandaObjChildren = False

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.tree, 1, wx.EXPAND, 0)
        self.SetSizer(sizer); self.Layout()

        parentSizer = wx.BoxSizer(wx.VERTICAL)
        parentSizer.Add(self, 1, wx.EXPAND, 0)
        parent.SetSizer(parentSizer); parent.Layout()

        parent.SetDropTarget(SceneGraphUIDropTarget(self.editor))

        self.tree.Bind(wx.EVT_TREE_SEL_CHANGED, self.onSelected)
        self.tree.Bind(wx.EVT_TREE_BEGIN_DRAG, self.onBeginDrag)

    def reset(self):
        #import pdb;set_trace()
        itemList = list()
        item, cookie = self.tree.GetFirstChild(self.root)
        while item:
             itemList.append(item)
             item, cookie = self.tree.GetNextChild(self.root, cookie)

        for item in itemList:
            self.tree.Delete(item)

    def traversePandaObjects(self, parent, objNodePath):
        itemId = self.tree.GetItemPyData(parent)
        i = 0
        for child in objNodePath.getChildren():
            namestr = "%s.%s"%(child.node().getType(), child.node().getName())
            newItem = self.tree.PrependItem(parent, namestr)
            newItemId = "%s.%s"%(itemId, i)
            self.tree.SetItemPyData(newItem, newItemId)

            # recursing...
            self.traversePandaObjects(newItem, child)
            i = i + 1

    def addPandaObjectChildren(self, parent):
        # first, find Panda Object's NodePath of the item
        itemId = self.tree.GetItemPyData(parent)
        if itemId == "render":
           return
        obj = self.editor.objectMgr.findObjectById(itemId)
        if obj is None:
           return

        objNodePath = obj[OG.OBJ_NP]
        self.traversePandaObjects(parent, objNodePath)

        item, cookie = self.tree.GetFirstChild(parent)
        while item:
             # recursing...
             self.addPandaObjectChildren(item)
             item, cookie = self.tree.GetNextChild(parent, cookie)

    def removePandaObjectChildren(self, parent):
        # first, find Panda Object's NodePath of the item
        itemId = self.tree.GetItemPyData(parent)
        if itemId == "render":
           return
        obj = self.editor.objectMgr.findObjectById(itemId)
        if obj is None:
           self.tree.Delete(parent)
           return
        item, cookie = self.tree.GetFirstChild(parent)
        while item:
           # recurse...
           itemToRemove = item
           # continue iteration to the next child
           item, cookie = self.tree.GetNextChild(parent, cookie)
           self.removePandaObjectChildren(itemToRemove)

    def add(self, item):
        #import pdb;pdb.set_trace()
        if item is None:
           return
        obj = self.editor.objectMgr.findObjectByNodePath(NodePath(item))
        if obj is None:
           return

        parentNodePath = obj[OG.OBJ_NP].getParent()
        parentObj = self.editor.objectMgr.findObjectByNodePath(parentNodePath)

        #import pdb;pdb.set_trace()
        if parentObj is None:
            parent = self.root
        else:
            parent = self.traverse(self.root, parentObj[OG.OBJ_UID])

        namestr = "%s_%s"%(obj[OG.OBJ_DEF].name, obj[OG.OBJ_UID])
        newItem = self.tree.AppendItem(parent, namestr)
        self.tree.SetItemPyData(newItem, obj[OG.OBJ_UID])
        
        # adding children of PandaObj
        if self.shouldShowPandaObjChildren:
           self.addPandaObjectChildren(newItem)
        self.tree.Expand(self.root)

    def traverse(self, parent, itemId):
        # prevent from traversing into self
        if itemId == self.tree.GetItemPyData(parent):
           return None

        # main loop - serching for an item with an itemId
        item, cookie = self.tree.GetFirstChild(parent)
        while item:
              # if the item was found - return it
              if itemId == self.tree.GetItemPyData(item):
                 return item

              # the tem was not found - checking if it has children
              if self.tree.ItemHasChildren(item):
                 # item has children - delving into it
                 child = self.traverse(item, itemId)
                 if child is not None:
                    return child
                    
              # continue iteration to the next child
              item, cookie = self.tree.GetNextChild(parent, cookie)
        return None

    def reParentTree(self, parent, newParent):
        # main loop - iterating over item's children
        item, cookie = self.tree.GetFirstChild(parent)
        while item:
           data = self.tree.GetItemText(item)
           itemId = self.tree.GetItemPyData(item)
           newItem = self.tree.AppendItem(newParent, data)
           self.tree.SetItemPyData(newItem, itemId)

           # if an item had children, we need to re-parent them as well
           if self.tree.ItemHasChildren(item):
              # recursing...
              self.reParentTree(item, newItem)

           # continue iteration to the next child
           item, cookie = self.tree.GetNextChild(parent, cookie)

    def reParentData(self, parent, child):
        child.wrtReparentTo(parent)

    def reParent(self, oldParent, newParent, child):
        if newParent is None:
           newParent = self.root
        itemId = self.tree.GetItemPyData(oldParent)
        newItem = self.tree.AppendItem(newParent, child)
        self.tree.SetItemPyData(newItem, itemId)
        self.reParentTree(oldParent, newItem)

        obj = self.editor.objectMgr.findObjectById(itemId)
        itemId = self.tree.GetItemPyData(newParent)
        if itemId != "render":
          newParentObj = self.editor.objectMgr.findObjectById(itemId)
          self.reParentData(newParentObj[OG.OBJ_NP], obj[OG.OBJ_NP])
        else:
          self.reParentData(render, obj[OG.OBJ_NP])

        self.tree.Delete(oldParent)
        if self.shouldShowPandaObjChildren:
           self.removePandaObjectChildren(oldParent)
           self.addPandaObjectChildren(oldParent)
           self.removePandaObjectChildren(newParent)
           self.addPandaObjectChildren(newpParent)

    def isChildOrGrandChild(self, parent, child):
        childId = self.tree.GetItemPyData(child)
        return self.traverse(parent, childId)

    def changeHierarchy(self, data, x, y):
        itemText = data.split('_')
        itemId = itemText[-1] # uid is the last token
        item = self.traverse(self.tree.GetRootItem(), itemId)
        if item is None:
           return

        dragToItem, flags = self.tree.HitTest(wx.Point(x, y))
        if dragToItem.IsOk():
           # prevent draging into itself
           if dragToItem == item:
              return
           if self.isChildOrGrandChild(item, dragToItem):
              return

           # undo function setup...
           action = ActionChangeHierarchy(self.editor, self.tree.GetItemPyData(self.tree.GetItemParent(item)), self.tree.GetItemPyData(item), self.tree.GetItemPyData(dragToItem), data)
           self.editor.actionMgr.push(action)
           action()

    def parent(self, oldParentId, newParentId, childName):
        oldParent = self.traverse(self.tree.GetRootItem(), oldParentId)
        newParent = self.traverse(self.tree.GetRootItem(), newParentId)
        self.reParent(oldParent, newParent, childName)

    def showPandaObjectChildren(self):
        itemList = list()
        self.shouldShowPandaObjChildren = not self.shouldShowPandaObjChildren

        item, cookie = self.tree.GetFirstChild(self.root)
        while item:
             itemList.append(item)
             item, cookie = self.tree.GetNextChild(self.root, cookie)

        #import pdb;set_trace()
        for item in itemList:
             if self.shouldShowPandaObjChildren:
                self.addPandaObjectChildren(item)
             else:
                self.removePandaObjectChildren(item)
             # continue iteration to the next child

    def delete(self, itemId):
        item = self.traverse(self.root, itemId)
        if item:
           self.tree.Delete(item)

    def select(self, itemId):
        item = self.traverse(self.root, itemId)
        if item:
           if not self.tree.IsSelected(item):
              self.tree.SelectItem(item)

    def deSelect(self, itemId):
        item =  self.traverse(self.root, itemId)
        if item is not None:
           self.tree.UnselectItem(item)

    def onSelected(self, event):
        item = event.GetItem();
        if item:
           itemId = self.tree.GetItemPyData(item)
           if itemId:
              obj = self.editor.objectMgr.findObjectById(itemId);
              if obj:
                 selections = self.tree.GetSelections()
                 if len(selections) > 1:
                    base.direct.select(obj[OG.OBJ_NP], fMultiSelect = 1, fLEPane = 0)
                 else:
                    base.direct.select(obj[OG.OBJ_NP], fMultiSelect = 0, fLEPane = 0)

    def onBeginDrag(self, event):
        item = event.GetItem()

        if item != self.tree.GetRootItem(): # prevent dragging root item
            text = self.tree.GetItemText(item)
            print "Starting SceneGraphUI drag'n'drop with %s..." % repr(text)

            tdo = wx.TextDataObject(text)
            tds = wx.DropSource(self.tree)
            tds.SetData(tdo)
            tds.DoDragDrop(True)
