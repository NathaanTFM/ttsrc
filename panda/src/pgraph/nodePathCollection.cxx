// Filename: nodePathCollection.cxx
// Created by:  drose (06Mar02)
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

#include "nodePathCollection.h"
#include "findApproxPath.h"
#include "findApproxLevelEntry.h"
#include "textureAttrib.h"
#include "colorScaleAttrib.h"
#include "colorAttrib.h"
#include "indent.h"

#ifdef HAVE_PYTHON
#include "py_panda.h"
#endif

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
NodePathCollection::
NodePathCollection() {
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::Copy Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
NodePathCollection::
NodePathCollection(const NodePathCollection &copy) :
  _node_paths(copy._node_paths)
{
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::Copy Assignment Operator
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
void NodePathCollection::
operator = (const NodePathCollection &copy) {
  _node_paths = copy._node_paths;
}

#ifdef HAVE_PYTHON
////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::Constructor
//       Access: Published
//  Description: This special constructor accepts a Python list of
//               NodePaths.  Since this constructor accepts a generic
//               PyObject *, it should be the last constructor listed
//               in the class record.
////////////////////////////////////////////////////////////////////
NodePathCollection::
NodePathCollection(PyObject *self, PyObject *sequence) {
  // We have to pre-initialize self's "this" pointer when we receive
  // self in the constructor--the caller can't initialize this for us.
  ((Dtool_PyInstDef *)self)->_ptr_to_object = this;

  if (!PySequence_Check(sequence)) {
    // If passed with a non-sequence, this isn't the right constructor.
    PyErr_SetString(PyExc_TypeError, "NodePathCollection constructor requires a sequence");
    return;
  }

  int size = PySequence_Size(sequence);
  for (int i = 0; i < size; ++i) {
    PyObject *item = PySequence_GetItem(sequence, i);
    if (item == NULL) {
      return;
    }
    PyObject *result = PyObject_CallMethod(self, (char *)"addPath", (char *)"O", item);
    Py_DECREF(item);
    if (result == NULL) {
      // Unable to add item--probably it wasn't of the appropriate type.
      ostringstream stream;
      stream << "Element " << i << " in sequence passed to NodePathCollection constructor could not be added";
      string str = stream.str();
      PyErr_SetString(PyExc_TypeError, str.c_str());
      return;
    }
    Py_DECREF(result);
  }
}
#endif  // HAVE_PYTHON

#ifdef HAVE_PYTHON
////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::__reduce__
//       Access: Published
//  Description: This special Python method is implement to provide
//               support for the pickle module.
////////////////////////////////////////////////////////////////////
PyObject *NodePathCollection::
__reduce__(PyObject *self) const {
  // Here we will return a 4-tuple: (Class, (args), None, iterator),
  // where iterator is an iterator that will yield successive
  // NodePaths.

  // We should return at least a 2-tuple, (Class, (args)): the
  // necessary class object whose constructor we should call
  // (e.g. this), and the arguments necessary to reconstruct this
  // object.

  PyObject *this_class = PyObject_Type(self);
  if (this_class == NULL) {
    return NULL;
  }

  // Since a NodePathCollection is itself an iterator, we can simply
  // pass it as the fourth tuple component.
  PyObject *result = Py_BuildValue("(O()OO)", this_class, Py_None, self);
  Py_DECREF(this_class);
  return result;
}
#endif  // HAVE_PYTHON

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::add_path
//       Access: Published
//  Description: Adds a new NodePath to the collection.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
add_path(const NodePath &node_path) {
  // If the pointer to our internal array is shared by any other
  // NodePathCollections, we have to copy the array now so we won't
  // inadvertently modify any of our brethren NodePathCollection
  // objects.

  if (_node_paths.get_ref_count() > 1) {
    NodePaths old_node_paths = _node_paths;
    _node_paths = NodePaths::empty_array(0);
    _node_paths.v() = old_node_paths.v();
  }

  _node_paths.push_back(node_path);
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::remove_path
//       Access: Published
//  Description: Removes the indicated NodePath from the collection.
//               Returns true if the path was removed, false if it was
//               not a member of the collection.
////////////////////////////////////////////////////////////////////
bool NodePathCollection::
remove_path(const NodePath &node_path) {
  int path_index = -1;
  for (int i = 0; path_index == -1 && i < (int)_node_paths.size(); i++) {
    if (_node_paths[i] == node_path) {
      path_index = i;
    }
  }

  if (path_index == -1) {
    // The indicated path was not a member of the collection.
    return false;
  }

  // If the pointer to our internal array is shared by any other
  // NodePathCollections, we have to copy the array now so we won't
  // inadvertently modify any of our brethren NodePathCollection
  // objects.

  if (_node_paths.get_ref_count() > 1) {
    NodePaths old_node_paths = _node_paths;
    _node_paths = NodePaths::empty_array(0);
    _node_paths.v() = old_node_paths.v();
  }

  _node_paths.erase(_node_paths.begin() + path_index);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::add_paths_from
//       Access: Published
//  Description: Adds all the NodePaths indicated in the other
//               collection to this path.  The other paths are simply
//               appended to the end of the paths in this list;
//               duplicates are not automatically removed.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
add_paths_from(const NodePathCollection &other) {
  int other_num_paths = other.get_num_paths();
  for (int i = 0; i < other_num_paths; i++) {
    add_path(other.get_path(i));
  }
}


////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::remove_paths_from
//       Access: Published
//  Description: Removes from this collection all of the NodePaths
//               listed in the other collection.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
remove_paths_from(const NodePathCollection &other) {
  NodePaths new_paths;
  int num_paths = get_num_paths();
  for (int i = 0; i < num_paths; i++) {
    NodePath path = get_path(i);
    if (!other.has_path(path)) {
      new_paths.push_back(path);
    }
  }
  _node_paths = new_paths;
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::remove_duplicate_paths
//       Access: Published
//  Description: Removes any duplicate entries of the same NodePaths
//               on this collection.  If a NodePath appears multiple
//               times, the first appearance is retained; subsequent
//               appearances are removed.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
remove_duplicate_paths() {
  NodePaths new_paths;

  int num_paths = get_num_paths();
  for (int i = 0; i < num_paths; i++) {
    NodePath path = get_path(i);
    bool duplicated = false;

    for (int j = 0; j < i && !duplicated; j++) {
      duplicated = (path == get_path(j));
    }

    if (!duplicated) {
      new_paths.push_back(path);
    }
  }

  _node_paths = new_paths;
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::has_path
//       Access: Published
//  Description: Returns true if the indicated NodePath appears in
//               this collection, false otherwise.
////////////////////////////////////////////////////////////////////
bool NodePathCollection::
has_path(const NodePath &path) const {
  for (int i = 0; i < get_num_paths(); i++) {
    if (path == get_path(i)) {
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::clear
//       Access: Published
//  Description: Removes all NodePaths from the collection.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
clear() {
  _node_paths.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::is_empty
//       Access: Published
//  Description: Returns true if there are no NodePaths in the
//               collection, false otherwise.
////////////////////////////////////////////////////////////////////
bool NodePathCollection::
is_empty() const {
  return _node_paths.empty();
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::get_num_paths
//       Access: Published
//  Description: Returns the number of NodePaths in the collection.
////////////////////////////////////////////////////////////////////
int NodePathCollection::
get_num_paths() const {
  return _node_paths.size();
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::get_path
//       Access: Published
//  Description: Returns the nth NodePath in the collection.
////////////////////////////////////////////////////////////////////
NodePath NodePathCollection::
get_path(int index) const {
  nassertr(index >= 0 && index < (int)_node_paths.size(), NodePath());

  return _node_paths[index];
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::operator []
//       Access: Published
//  Description: Returns the nth NodePath in the collection.  This is
//               the same as get_path(), but it may be a more
//               convenient way to access it.
////////////////////////////////////////////////////////////////////
NodePath NodePathCollection::
operator [] (int index) const {
  nassertr(index >= 0 && index < (int)_node_paths.size(), NodePath());

  return _node_paths[index];
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::size
//       Access: Published
//  Description: Returns the number of paths in the collection.  This
//               is the same thing as get_num_paths().
////////////////////////////////////////////////////////////////////
int NodePathCollection::
size() const {
  return _node_paths.size();
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::ls
//       Access: Published
//  Description: Lists all the nodes at and below each node in the
//               collection hierarchically.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
ls(ostream &out, int indent_level) const {
  for (int i = 0; i < get_num_paths(); i++) {
    NodePath path = get_path(i);
    indent(out, indent_level) << path << "\n";
    path.ls(out, indent_level + 2);
    out << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::find_all_matches
//       Access: Published
//  Description: Returns the complete set of all NodePaths that begin
//               with any NodePath in this collection and can be
//               extended by path.  The shortest paths will be listed
//               first.
////////////////////////////////////////////////////////////////////
NodePathCollection NodePathCollection::
find_all_matches(const string &path) const {
  NodePathCollection result;

  FindApproxPath approx_path;
  if (approx_path.add_string(path)) {
    if (!is_empty()) {
      FindApproxLevelEntry *level = NULL;
      for (int i = 0; i < get_num_paths(); i++) {
        FindApproxLevelEntry *start = 
          new FindApproxLevelEntry(get_path(i), approx_path);
        start->_next = level;
        level = start;
      }
      get_path(0).find_matches(result, level, -1);
    }
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::reparent_to
//       Access: Published
//  Description: Reparents all the NodePaths in the collection to the
//               indicated node.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
reparent_to(const NodePath &other) {
  for (int i = 0; i < get_num_paths(); i++) {
    get_path(i).reparent_to(other);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::wrt_reparent_to
//       Access: Published
//  Description: Reparents all the NodePaths in the collection to the
//               indicated node, adjusting each transform so as not to
//               move in world coordinates.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
wrt_reparent_to(const NodePath &other) {
  for (int i = 0; i < get_num_paths(); i++) {
    get_path(i).wrt_reparent_to(other);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::show
//       Access: Published
//  Description: Shows all NodePaths in the collection.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
show() {
  for (int i = 0; i < get_num_paths(); i++) {
    get_path(i).show();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::show
//       Access: Published
//  Description: Hides all NodePaths in the collection.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
hide() {
  for (int i = 0; i < get_num_paths(); i++) {
    get_path(i).hide();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::stash
//       Access: Published
//  Description: Stashes all NodePaths in the collection.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
stash() {
  for (int i = 0; i < get_num_paths(); i++) {
    get_path(i).stash();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::unstash
//       Access: Published
//  Description: Unstashes all NodePaths in the collection.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
unstash() {
  for (int i = 0; i < get_num_paths(); i++) {
    get_path(i).unstash();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::detach
//       Access: Published
//  Description: Detaches all NodePaths in the collection.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
detach() {
  for (int i = 0; i < get_num_paths(); i++) {
    get_path(i).detach_node();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::get_collide_mask
//       Access: Published
//  Description: Returns the union of all of the into_collide_masks
//               for nodes at this level and below.  This is the same
//               thing as node()->get_net_collide_mask().
//
//               If you want to return what the into_collide_mask of
//               this node itself is, without regard to its children,
//               use node()->get_into_collide_mask().
////////////////////////////////////////////////////////////////////
CollideMask NodePathCollection::
get_collide_mask() const {
  CollideMask collide_mask;
  for (int i = 0; i < get_num_paths(); i++) {
    collide_mask |= get_path(i).get_collide_mask();
  }
  return collide_mask;
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::set_collide_mask
//       Access: Published
//  Description: Recursively applies the indicated CollideMask to the
//               into_collide_masks for all nodes at this level and
//               below.  Only nodes 
//
//               The default is to change all bits, but if
//               bits_to_change is not all bits on, then only the bits
//               that are set in bits_to_change are modified, allowing
//               this call to change only a subset of the bits in the
//               subgraph.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
set_collide_mask(CollideMask new_mask, CollideMask bits_to_change,
                 TypeHandle node_type) {
  for (int i = 0; i < get_num_paths(); i++) {
    get_path(i).set_collide_mask(new_mask, bits_to_change, node_type);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::set_texture
//       Access: Published
//  Description: Adds the indicated texture to the list of textures
//               that will be rendered on the default texture stage.
//
//               This is the deprecated single-texture variant of this
//               method; it is now superceded by set_texture() that
//               accepts a stage and texture.  However, this method
//               may be used in the presence of multitexture if you
//               just want to adjust the default stage.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
set_texture(Texture *tex, int priority) {
  PT(TextureStage) stage = TextureStage::get_default();
  set_texture(stage, tex, priority);
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::set_texture
//       Access: Published
//  Description: Adds the indicated texture to the list of textures
//               that will be rendered on the indicated multitexture
//               stage.  If there are multiple texture stages
//               specified (possibly on multiple different nodes at
//               different levels), they will all be applied to
//               geometry together, according to the stage
//               specification set up in the TextureStage object.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
set_texture(TextureStage *stage, Texture *tex, int priority) {
  StateMap state_map;

  NodePaths::iterator npi;
  for (npi = _node_paths.begin(); npi != _node_paths.end(); ++npi) {
    NodePath &np = (*npi);
    CPT(RenderState) orig_state = np.get_state();
    StateMap::iterator smi = state_map.find(orig_state);
    if (smi != state_map.end()) {
      // This RenderState has already been encountered; reuse it.
      np.set_state((*smi).second);
    } else {
      // This RenderState has not yet been encountered; apply the
      // attrib to it.
      np.set_texture(stage, tex, priority);
      state_map[orig_state] = np.get_state();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::set_texture_off
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using no texture, on any stage.  This is different
//               from not specifying a texture; rather, this
//               specifically contradicts set_texture() at a higher
//               node level (or, with a priority, overrides a
//               set_texture() at a lower level).
////////////////////////////////////////////////////////////////////
void NodePathCollection::
set_texture_off(int priority) {
  nassertv_always(!is_empty());
  set_attrib(TextureAttrib::make_all_off(), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::set_texture_off
//       Access: Published
//  Description: Sets the geometry at this level and below to render
//               using no texture, on the indicated stage.  This is
//               different from not specifying a texture; rather, this
//               specifically contradicts set_texture() at a higher
//               node level (or, with a priority, overrides a
//               set_texture() at a lower level).
////////////////////////////////////////////////////////////////////
void NodePathCollection::
set_texture_off(TextureStage *stage, int priority) {
  StateMap state_map;

  NodePaths::iterator npi;
  for (npi = _node_paths.begin(); npi != _node_paths.end(); ++npi) {
    NodePath &np = (*npi);
    CPT(RenderState) orig_state = np.get_state();
    StateMap::iterator smi = state_map.find(orig_state);
    if (smi != state_map.end()) {
      // This RenderState has already been encountered; reuse it.
      np.set_state((*smi).second);
    } else {
      // This RenderState has not yet been encountered; apply the
      // attrib to it.
      np.set_texture_off(stage, priority);
      state_map[orig_state] = np.get_state();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::set_color
//       Access: Published
//  Description: Colors all NodePaths in the collection
////////////////////////////////////////////////////////////////////
void NodePathCollection::
set_color(const Colorf &color, int priority) {
  set_attrib(ColorAttrib::make_flat(color), priority);
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::set_color_scale
//       Access: Published
//  Description: Applies color scales to all NodePaths in the
//               collection.  The existing color scale is replaced.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
set_color_scale(const LVecBase4f &scale, int priority) {
  StateMap state_map;

  NodePaths::iterator npi;
  for (npi = _node_paths.begin(); npi != _node_paths.end(); ++npi) {
    NodePath &np = (*npi);
    CPT(RenderState) orig_state = np.get_state();
    StateMap::iterator smi = state_map.find(orig_state);
    if (smi != state_map.end()) {
      // This RenderState has already been encountered; reuse it.
      np.set_state((*smi).second);
    } else {
      // This RenderState has not yet been encountered; apply the
      // attrib to it.
      np.set_color_scale(scale, priority);
      state_map[orig_state] = np.get_state();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::compose_color_scale
//       Access: Published
//  Description: Applies color scales to all NodePaths in the
//               collection.  The existing color scale, if any, is
//               multiplied by the specified color scale.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
compose_color_scale(const LVecBase4f &scale, int priority) {
  StateMap state_map;

  NodePaths::iterator npi;
  for (npi = _node_paths.begin(); npi != _node_paths.end(); ++npi) {
    NodePath &np = (*npi);
    CPT(RenderState) orig_state = np.get_state();
    StateMap::iterator smi = state_map.find(orig_state);
    if (smi != state_map.end()) {
      // This RenderState has already been encountered; reuse it.
      np.set_state((*smi).second);
    } else {
      // This RenderState has not yet been encountered; apply the
      // attrib to it.
      np.compose_color_scale(scale, priority);
      state_map[orig_state] = np.get_state();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::set_attrib
//       Access: Published
//  Description: Applies the indicated RenderAttrib to all NodePaths
//               in the collection.  An effort is made to apply the
//               attrib to many NodePaths as quickly as possible;
//               redundant RenderState compositions are not
//               duplicated.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
set_attrib(const RenderAttrib *attrib, int priority) {
  StateMap state_map;

  NodePaths::iterator npi;
  for (npi = _node_paths.begin(); npi != _node_paths.end(); ++npi) {
    NodePath &np = (*npi);
    CPT(RenderState) orig_state = np.get_state();
    StateMap::iterator smi = state_map.find(orig_state);
    if (smi != state_map.end()) {
      // This RenderState has already been encountered; reuse it.
      np.set_state((*smi).second);
    } else {
      // This RenderState has not yet been encountered; apply the
      // attrib to it.
      np.set_attrib(attrib, priority);
      state_map[orig_state] = np.get_state();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::output
//       Access: Published
//  Description: Writes a brief one-line description of the
//               NodePathCollection to the indicated output stream.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
output(ostream &out) const {
  if (get_num_paths() == 1) {
    out << "1 NodePath";
  } else {
    out << get_num_paths() << " NodePaths";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: NodePathCollection::write
//       Access: Published
//  Description: Writes a complete multi-line description of the
//               NodePathCollection to the indicated output stream.
////////////////////////////////////////////////////////////////////
void NodePathCollection::
write(ostream &out, int indent_level) const {
  for (int i = 0; i < get_num_paths(); i++) {
    indent(out, indent_level) << get_path(i) << "\n";
  }
}
