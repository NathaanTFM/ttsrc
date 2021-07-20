// Filename: maxNodeTree.cxx
// Created by: crevilla
// from mayaNodeTree.cxx created by:  drose (06Jun03)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "maxEgg.h"

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
MaxNodeTree::
MaxNodeTree() {
  _root = new MaxNodeDesc;
  _fps = 0.0;
  _has_collision = false;
  _export_mesh = false;
  _cs_type = EggGroup::CST_none;
  _cf_type = EggGroup::CF_none;
  _egg_data = (EggData *)NULL;
  _egg_root = (EggGroupNode *)NULL;
  _skeleton_node = (EggGroupNode *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::build_node
//       Access: Public
//  Description: Returns a pointer to the node corresponding to the
//               indicated INode object, creating it first if
//               necessary.
////////////////////////////////////////////////////////////////////
MaxNodeDesc *MaxNodeTree::
build_node(INode *max_node) {
  MaxNodeDesc *node_desc = r_build_node(max_node);
  node_desc->from_INode(max_node);
  if (node_desc->is_node_joint())
    node_desc->_joint_entry = build_joint(max_node, node_desc);
  return node_desc;
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::build_node
//       Access: Public
//  Description: Returns a pointer to the node corresponding to the
//               indicated INode object, creating it first if
//               necessary.
////////////////////////////////////////////////////////////////////
MaxNodeDesc *MaxNodeTree::
build_joint(INode *max_node, MaxNodeDesc *node_joint) {
  MaxNodeDesc *node_desc = r_build_joint(node_joint, max_node);
  node_desc->from_INode(max_node);
  node_desc->set_joint(true);  
  return node_desc;
}

bool MaxNodeTree::node_in_list(ULONG handle, ULONG *list, int len) {
  if (!list) return true;
  for (int i = 0; i < len; i++)
    if (list[i] == handle) return true;
  return false;
}

bool MaxNodeTree::is_joint(INode *node) {
  Control *c = node->GetTMController();
  return (node->GetBoneNodeOnOff() ||                    //joints
         (c &&                                           //bipeds
         ((c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) ||
         (c->ClassID() == BIPBODY_CONTROL_CLASS_ID) ||
         (c->ClassID() == FOOTPRINT_CLASS_ID))));
}

bool MaxNodeTree::
r_build_hierarchy(INode *root, ULONG *selection_list, int len) {
  if (node_in_list(root->GetHandle(), selection_list, len))
    build_node(root);
  // Export children
  for ( int i = 0; i < root->NumberOfChildren(); i++ ) {
    // *** Should probably be checking the return value of the following line
    r_build_hierarchy(root->GetChildNode(i), selection_list, len);
  }
  return true;
}
////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::build_complete_hierarchy
//       Access: Public
//  Description: Walks through the complete Max hierarchy and builds
//               up the corresponding tree.
////////////////////////////////////////////////////////////////////
bool MaxNodeTree::
build_complete_hierarchy(INode *root, ULONG *selection_list, int len) {

  // Get the entire Max scene.
  if (root == NULL) {
    // *** Log an error
    return false;
  }
    
  bool all_ok = true;
  r_build_hierarchy(root, selection_list, len);

  if (all_ok) {
    _root->check_pseudo_joints(false);
  }

  return all_ok;
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::get_num_nodes
//       Access: Public
//  Description: Returns the total number of nodes in the hierarchy,
//               not counting the root node.
////////////////////////////////////////////////////////////////////
int MaxNodeTree::
get_num_nodes() const {
  return _nodes.size();
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::get_node
//       Access: Public
//  Description: Returns the nth node in the hierarchy, in an
//               arbitrary ordering.
////////////////////////////////////////////////////////////////////
MaxNodeDesc *MaxNodeTree::
get_node(int n) const {
  nassertr(n >= 0 && n < (int)_nodes.size(), NULL);
  return _nodes[n];
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::clear_egg
//       Access: Public
//  Description: Removes all of the references to generated egg
//               structures from the tree, and prepares the tree for
//               generating new egg structures.
////////////////////////////////////////////////////////////////////
void MaxNodeTree::
clear_egg(EggData *egg_data, EggGroupNode *egg_root, 
          EggGroupNode *skeleton_node) {
  _root->clear_egg();
  _egg_data = egg_data;
  _egg_root = egg_root;
  _skeleton_node = skeleton_node;
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::get_egg_group
//       Access: Public
//  Description: Returns the EggGroupNode corresponding to the group
//               or joint for the indicated node.  Creates the group
//               node if it has not already been created.
////////////////////////////////////////////////////////////////////
EggGroup *MaxNodeTree::
get_egg_group(MaxNodeDesc *node_desc) {
  nassertr(_egg_root != (EggGroupNode *)NULL, NULL);

  if (node_desc->_egg_group == (EggGroup *)NULL) {
    // We need to make a new group node.
    EggGroup *egg_group;

    nassertr(node_desc->_parent != (MaxNodeDesc *)NULL, NULL);
    egg_group = new EggGroup(node_desc->get_name());
    if (node_desc->is_joint()) {
      egg_group->set_group_type(EggGroup::GT_joint);
    }

    if (node_desc->_parent == _root) {
      // The parent is the root.
      // Set collision properties for the root if it has them:
      if(_has_collision && !_export_mesh)
      {
        egg_group->set_collision_name(node_desc->get_name());
        egg_group->set_cs_type(_cs_type);
        egg_group->set_collide_flags(_cf_type);
      }
      _egg_root->add_child(egg_group);

    } else {
      // The parent is another node.
      // if export mesh, the tag should be added at the second level
      if(_has_collision && _export_mesh)
      {
        if(node_desc->_parent->_parent == _root)
        {
          egg_group->set_collision_name(node_desc->get_name());
          egg_group->set_cs_type(_cs_type);
          egg_group->set_collide_flags(_cf_type);
        }
      }
      EggGroup *parent_egg_group = get_egg_group(node_desc->_parent);
      parent_egg_group->add_child(egg_group);
    }

    // *** This is probably something that a Max plugin would need to be 
    //     written for.  May want to ask Disney about it
    /*
    if (node_desc->has_dag_path()) {
      // Check for an object type setting, from Oliver's plug-in.
      MObject dag_object = node_desc->get_dag_path().node();
      string object_type;
      if (get_enum_attribute(dag_object, "eggObjectTypes1", object_type)) {
        egg_group->add_object_type(object_type);
      }
      if (get_enum_attribute(dag_object, "eggObjectTypes2", object_type)) {
        egg_group->add_object_type(object_type);
      }
      if (get_enum_attribute(dag_object, "eggObjectTypes3", object_type)) {
        egg_group->add_object_type(object_type);
      }

      // We treat the object type "billboard" as a special case: we
      // apply this one right away and also flag the group as an
      // instance.
      if (egg_group->has_object_type("billboard")) {    
        egg_group->remove_object_type("billboard");
        egg_group->set_group_type(EggGroup::GT_instance);
        egg_group->set_billboard_type(EggGroup::BT_axis);
        
      } else if (egg_group->has_object_type("billboard-point")) {    
        egg_group->remove_object_type("billboard-point");
        egg_group->set_group_type(EggGroup::GT_instance);
        egg_group->set_billboard_type(EggGroup::BT_point_camera_relative);
      }
      
      // We also treat the object type "dcs" and "model" as a special
      // case, so we can test for these flags later.
      if (egg_group->has_object_type("dcs")) {
        egg_group->remove_object_type("dcs");
        egg_group->set_dcs_type(EggGroup::DC_default);
      }
      if (egg_group->has_object_type("model")) {
        egg_group->remove_object_type("model");
        egg_group->set_model_flag(true);
      }
      
      // And "vertex-color" has meaning only to this converter.
      if (egg_group->has_object_type("vertex-color")) {
        egg_group->remove_object_type("vertex-color");
        MaxEggGroupUserData *user_data = new MaxEggGroupUserData;
        user_data->_vertex_color = true;
        egg_group->set_user_data(user_data);
      }
    }
    */
    node_desc->_egg_group = egg_group;
  }

  return node_desc->_egg_group;
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::get_egg_table
//       Access: Public
//  Description: Returns the EggTable corresponding to the joint
//               for the indicated node.  Creates the table node if it
//               has not already been created.
////////////////////////////////////////////////////////////////////
EggTable *MaxNodeTree::
get_egg_table(MaxNodeDesc *node_desc) {
  nassertr(_skeleton_node != (EggGroupNode *)NULL, NULL);
  nassertr(node_desc->is_joint(), NULL);

  if (node_desc->_egg_table == (EggTable *)NULL) {
    // We need to make a new table node.
    nassertr(node_desc->_parent != (MaxNodeDesc *)NULL, NULL);

    EggTable *egg_table = new EggTable(node_desc->get_name());
    node_desc->_anim = new EggXfmSAnim("xform", 
                                       _egg_data->get_coordinate_system());
    node_desc->_anim->set_fps(_fps);
    egg_table->add_child(node_desc->_anim);

    if (!node_desc->_parent->is_joint()) {
      // The parent is not a joint; put it at the top.
      _skeleton_node->add_child(egg_table);

    } else {
      // The parent is another joint.
      EggTable *parent_egg_table = get_egg_table(node_desc->_parent);
      parent_egg_table->add_child(egg_table);
    }

    node_desc->_egg_table = egg_table;
  }

  return node_desc->_egg_table;
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::get_egg_anim
//       Access: Public
//  Description: Returns the anim table corresponding to the joint
//               for the indicated node.  Creates the table node if it
//               has not already been created.
////////////////////////////////////////////////////////////////////
EggXfmSAnim *MaxNodeTree::
get_egg_anim(MaxNodeDesc *node_desc) 
{
  get_egg_table(node_desc);
  return node_desc->_anim;
}


////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::r_build_node
//       Access: Private
//  Description: The recursive implementation of build_node().
////////////////////////////////////////////////////////////////////
MaxNodeDesc *MaxNodeTree::
r_build_node(INode* max_node) 
{
  // If we have already encountered this pathname, return the
  // corresponding MaxNodeDesc immediately.
  
  ULONG node_handle = 0;
  
  if (max_node) {
    node_handle = max_node->GetHandle();
  }    

  NodesByPath::const_iterator ni = _nodes_by_path.find(node_handle);
  if (ni != _nodes_by_path.end()) {
    return (*ni).second;
  }

  // Otherwise, we have to create it.  Do this recursively, so we
  // create each node along the path.
  MaxNodeDesc *node_desc;

  if (!max_node) {
    // This is the top.
    node_desc = _root;

  } else {
    INode *parent_node; 
    string local_name = max_node->GetName();
    if (max_node->IsRootNode()) {
      parent_node = NULL;
    } else {
      parent_node = max_node->GetParentNode();
    }

    MaxNodeDesc *parent_node_desc = r_build_node(parent_node);
    node_desc = new MaxNodeDesc(parent_node_desc, local_name);
    _nodes.push_back(node_desc);
  }

  _nodes_by_path.insert(NodesByPath::value_type(node_handle, node_desc));
  return node_desc;
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::r_build_joint
//       Access: Private
//  Description: The recursive implementation of build_joint().
////////////////////////////////////////////////////////////////////
MaxNodeDesc *MaxNodeTree::
r_build_joint(MaxNodeDesc *node_desc, INode *max_node) 
{
  MaxNodeDesc *node_joint;
  if (node_desc == _root) {
    node_joint =  new MaxNodeDesc(_root, max_node->GetName());
    _nodes.push_back(node_joint);
        return node_joint;
  }     else if (node_desc->is_node_joint() && node_desc->_joint_entry) {
    node_joint =  new MaxNodeDesc(node_desc->_joint_entry, max_node->GetName());
    _nodes.push_back(node_joint);
        return node_joint;
  } else {
        return r_build_joint(node_desc->_parent, max_node);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::find_node
//       Access: Private
//  Description: The recursive implementation of build_node().
////////////////////////////////////////////////////////////////////
MaxNodeDesc *MaxNodeTree::
find_node(INode* max_node) 
{
  // If we have already encountered this pathname, return the
  // corresponding MaxNodeDesc immediately.
  
  ULONG node_handle = 0;
  
  if (max_node) {
    node_handle = max_node->GetHandle();
  }    

  NodesByPath::const_iterator ni = _nodes_by_path.find(node_handle);
  if (ni != _nodes_by_path.end()) {
    return (*ni).second;
  } 

  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: MaxNodeTree::find_joint
//       Access: Private
//  Description: The recursive implementation of build_node().
////////////////////////////////////////////////////////////////////
MaxNodeDesc *MaxNodeTree::
find_joint(INode* max_node) 
{
  MaxNodeDesc *node = find_node(max_node);
  if (!node || (is_joint(max_node) && !node->is_node_joint()))
    node = build_node(max_node);
  return node->_joint_entry;
}
