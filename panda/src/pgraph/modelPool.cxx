// Filename: modelPool.cxx
// Created by:  drose (12Mar02)
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

#include "modelPool.h"
#include "loader.h"
#include "config_pgraph.h"
#include "lightMutexHolder.h"


ModelPool *ModelPool::_global_ptr = (ModelPool *)NULL;

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::write
//       Access: Published, Static
//  Description: Lists the contents of the model pool to the
//               indicated output stream.
//               Helps with debugging.
////////////////////////////////////////////////////////////////////
void ModelPool::
write(ostream &out) {
  get_ptr()->ns_list_contents(out);
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_has_model
//       Access: Private
//  Description: The nonstatic implementation of has_model().
////////////////////////////////////////////////////////////////////
bool ModelPool::
ns_has_model(const Filename &filename) {
  LightMutexHolder holder(_lock);
  Models::const_iterator ti;
  ti = _models.find(filename);
  if (ti != _models.end() && (*ti).second != (ModelRoot *)NULL) {
    // This model was previously loaded.
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_load_model
//       Access: Private
//  Description: The nonstatic implementation of load_model().
////////////////////////////////////////////////////////////////////
ModelRoot *ModelPool::
ns_load_model(const Filename &filename, const LoaderOptions &options) {
  {
    LightMutexHolder holder(_lock);
    Models::const_iterator ti;
    ti = _models.find(filename);
    if (ti != _models.end()) {
      // This model was previously loaded.
      return (*ti).second;
    }
  }

  LoaderOptions new_options(options);
  new_options.set_flags((new_options.get_flags() | LoaderOptions::LF_no_ram_cache) &
                        ~(LoaderOptions::LF_search | LoaderOptions::LF_report_errors));

  Loader *model_loader = Loader::get_global_ptr();
  PT(PandaNode) panda_node = model_loader->load_sync(filename, new_options);
  PT(ModelRoot) node;

  if (panda_node.is_null()) {
    // This model was not found.

  } else {
    if (panda_node->is_of_type(ModelRoot::get_class_type())) {
      node = DCAST(ModelRoot, panda_node);
      
    } else {
      // We have to construct a ModelRoot node to put it under.
      node = new ModelRoot(filename);
      node->add_child(panda_node);
    }
    node->set_fullpath(filename);
  }

  {
    LightMutexHolder holder(_lock);

    // Look again, in case someone has just loaded the model in
    // another thread.
    Models::const_iterator ti;
    ti = _models.find(filename);
    if (ti != _models.end()) {
      // This model was previously loaded.
      return (*ti).second;
    }

    _models[filename] = node;
  }

  return node;
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_add_model
//       Access: Private
//  Description: The nonstatic implementation of add_model().
////////////////////////////////////////////////////////////////////
void ModelPool::
ns_add_model(const Filename &filename, ModelRoot *model) {
  LightMutexHolder holder(_lock);
  // We blow away whatever model was there previously, if any.
  _models[filename] = model;
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_release_model
//       Access: Private
//  Description: The nonstatic implementation of release_model().
////////////////////////////////////////////////////////////////////
void ModelPool::
ns_release_model(const Filename &filename) {
  LightMutexHolder holder(_lock);
  Models::iterator ti;
  ti = _models.find(filename);
  if (ti != _models.end()) {
    _models.erase(ti);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_add_model
//       Access: Private
//  Description: The nonstatic implementation of add_model().
////////////////////////////////////////////////////////////////////
void ModelPool::
ns_add_model(ModelRoot *model) {
  LightMutexHolder holder(_lock);
  // We blow away whatever model was there previously, if any.
  _models[model->get_fullpath()] = model;
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_release_model
//       Access: Private
//  Description: The nonstatic implementation of release_model().
////////////////////////////////////////////////////////////////////
void ModelPool::
ns_release_model(ModelRoot *model) {
  LightMutexHolder holder(_lock);
  Models::iterator ti;
  ti = _models.find(model->get_fullpath());
  if (ti != _models.end()) {
    _models.erase(ti);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_release_all_models
//       Access: Private
//  Description: The nonstatic implementation of release_all_models().
////////////////////////////////////////////////////////////////////
void ModelPool::
ns_release_all_models() {
  LightMutexHolder holder(_lock);
  _models.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_garbage_collect
//       Access: Private
//  Description: The nonstatic implementation of garbage_collect().
////////////////////////////////////////////////////////////////////
int ModelPool::
ns_garbage_collect() {
  LightMutexHolder holder(_lock);

  int num_released = 0;
  Models new_set;

  Models::iterator ti;
  for (ti = _models.begin(); ti != _models.end(); ++ti) {
    ModelRoot *node = (*ti).second;
    if (node == (ModelRoot *)NULL ||
        node->get_model_ref_count() == 1) {
      if (loader_cat.is_debug()) {
        loader_cat.debug()
          << "Releasing " << (*ti).first << "\n";
      }
      ++num_released;
    } else {
      new_set.insert(new_set.end(), *ti);
    }
  }

  _models.swap(new_set);
  return num_released;
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::ns_list_contents
//       Access: Private
//  Description: The nonstatic implementation of list_contents().
////////////////////////////////////////////////////////////////////
void ModelPool::
ns_list_contents(ostream &out) const {
  LightMutexHolder holder(_lock);

  out << "model pool contents:\n";
  
  Models::const_iterator ti;
  int num_models = 0;
  for (ti = _models.begin(); ti != _models.end(); ++ti) {
    if ((*ti).second != NULL) {
      ++num_models;
      out << (*ti).first << "\n"
          << "  (count = " << (*ti).second->get_model_ref_count() 
          << ")\n";
    }
  }
  
  out << "total number of models: " << num_models << " (plus " 
      << _models.size() - num_models << " entries for nonexistent files)\n";
}

////////////////////////////////////////////////////////////////////
//     Function: ModelPool::get_ptr
//       Access: Private, Static
//  Description: Initializes and/or returns the global pointer to the
//               one ModelPool object in the system.
////////////////////////////////////////////////////////////////////
ModelPool *ModelPool::
get_ptr() {
  if (_global_ptr == (ModelPool *)NULL) {
    _global_ptr = new ModelPool;
  }
  return _global_ptr;
}
