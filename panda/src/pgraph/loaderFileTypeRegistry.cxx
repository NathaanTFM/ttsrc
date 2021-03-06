// Filename: loaderFileTypeRegistry.cxx
// Created by:  drose (20Jun00)
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

#include "loaderFileTypeRegistry.h"
#include "loaderFileType.h"
#include "config_pgraph.h"

#include "load_dso.h"
#include "string_utils.h"
#include "indent.h"

#include <algorithm>

LoaderFileTypeRegistry *LoaderFileTypeRegistry::_global_ptr;

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
LoaderFileTypeRegistry::
LoaderFileTypeRegistry() {
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
LoaderFileTypeRegistry::
~LoaderFileTypeRegistry() {
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::register_type
//       Access: Public
//  Description: Defines a new LoaderFileType in the universe.
////////////////////////////////////////////////////////////////////
void LoaderFileTypeRegistry::
register_type(LoaderFileType *type) {
  // Make sure we haven't already registered this type.
  if (find(_types.begin(), _types.end(), type) != _types.end()) {
    if (loader_cat->is_debug()) {
      loader_cat->debug()
        << "Attempt to register LoaderFileType " << type->get_name()
        << " (" << type->get_type() << ") more than once.\n";
    }
    return;
  }

  _types.push_back(type);

  record_extension(type->get_extension(), type);

  vector_string words;
  extract_words(type->get_additional_extensions(), words);
  vector_string::const_iterator wi;
  for (wi = words.begin(); wi != words.end(); ++wi) {
    record_extension(*wi, type);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::register_deferred_type
//       Access: Public
//  Description: Records a type associated with a particular extension
//               to be loaded in the future.  The named library will
//               be dynamically loaded the first time files of this
//               extension are loaded; presumably this library will
//               call register_type() when it initializes, thus making
//               the extension loadable.
////////////////////////////////////////////////////////////////////
void LoaderFileTypeRegistry::
register_deferred_type(const string &extension, const string &library) {
  string dcextension = downcase(extension);

  Extensions::const_iterator ei;
  ei = _extensions.find(dcextension);
  if (ei != _extensions.end()) {
    // We already have a loader for this type; no need to register
    // another one.
    if (loader_cat->is_debug()) {
      loader_cat->debug()
        << "Attempt to register loader library " << library
        << " (" << dcextension << ") when extension is already known.\n";
    }
    return;
  }

  DeferredTypes::const_iterator di;
  di = _deferred_types.find(dcextension);
  if (di != _deferred_types.end()) {
    if ((*di).second == library) {
      if (loader_cat->is_debug()) {
        loader_cat->debug()
          << "Attempt to register loader library " << library
          << " (" << dcextension << ") more than once.\n";
      }
      return;
    } else {
      if (loader_cat->is_debug()) {
        loader_cat->debug()
          << "Multiple libraries registered that use the extension "
          << dcextension << "\n";
      }
    }
  }

  _deferred_types[dcextension] = library;
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::get_num_types
//       Access: Published
//  Description: Returns the total number of types registered.
////////////////////////////////////////////////////////////////////
int LoaderFileTypeRegistry::
get_num_types() const {
  return _types.size();
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::get_type
//       Access: Published
//  Description: Returns the nth type registered.
////////////////////////////////////////////////////////////////////
LoaderFileType *LoaderFileTypeRegistry::
get_type(int n) const {
  nassertr(n >= 0 && n < (int)_types.size(), NULL);
  return _types[n];
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::get_type_from_extension
//       Access: Published
//  Description: Determines the type of the file based on the indicated
//               extension (without a leading dot).  Returns NULL if
//               the extension matches no known file types.
////////////////////////////////////////////////////////////////////
LoaderFileType *LoaderFileTypeRegistry::
get_type_from_extension(const string &extension) {
  string dcextension = downcase(extension);

  Extensions::const_iterator ei;
  ei = _extensions.find(dcextension);
  if (ei == _extensions.end()) {
    // Nothing matches that extension.  Do we have a deferred type?

    DeferredTypes::iterator di;
    di = _deferred_types.find(dcextension);
    if (di != _deferred_types.end()) {
      // We do!  Try to load the deferred library on-the-fly.  Note
      // that this is a race condition if we support threaded loading;
      // this whole function needs to be protected from multiple
      // entry.
      string name = (*di).second;
      Filename dlname = Filename::dso_filename("lib" + name + ".so");
      _deferred_types.erase(di);

      loader_cat->info()
        << "loading file type module: " << name << endl;
      void *tmp = load_dso(get_plugin_path().get_value(), dlname);
      if (tmp == (void *)NULL) {
        loader_cat->warning()
          << "Unable to load " << dlname.to_os_specific() << ": " 
          << load_dso_error() << endl;
        return NULL;
      }

      // Now try again to find the LoaderFileType.
      ei = _extensions.find(dcextension);
    }
  }

  if (ei == _extensions.end()) {
    // Nothing matches that extension, even after we've checked for a
    // deferred type description.
    return NULL;
  }

  return (*ei).second;
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::write
//       Access: Published
//  Description: Writes a list of supported file types to the
//               indicated output stream, one per line.
////////////////////////////////////////////////////////////////////
void LoaderFileTypeRegistry::
write(ostream &out, int indent_level) const {
  if (_types.empty()) {
    indent(out, indent_level) << "(No file types are known).\n";
  } else {
    Types::const_iterator ti;
    for (ti = _types.begin(); ti != _types.end(); ++ti) {
      LoaderFileType *type = (*ti);
      string name = type->get_name();
      indent(out, indent_level) << name;
      indent(out, max(30 - (int)name.length(), 0))
        << "  ." << type->get_extension() << "\n";
    }
  }

  if (!_deferred_types.empty()) {
    indent(out, indent_level) << "Also available:";
    DeferredTypes::const_iterator di;
    for (di = _deferred_types.begin(); di != _deferred_types.end(); ++di) {
      const string &extension = (*di).first;
      out << " ." << extension;
    }
    out << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::get_global_ptr
//       Access: Published, Static
//  Description: Returns a pointer to the global LoaderFileTypeRegistry
//               object.
////////////////////////////////////////////////////////////////////
LoaderFileTypeRegistry *LoaderFileTypeRegistry::
get_global_ptr() {
  if (_global_ptr == (LoaderFileTypeRegistry *)NULL) {
    _global_ptr = new LoaderFileTypeRegistry;
  }
  return _global_ptr;
}

////////////////////////////////////////////////////////////////////
//     Function: LoaderFileTypeRegistry::record_extension
//       Access: Private
//  Description: Records a filename extension recognized by a loader
//               file type.
////////////////////////////////////////////////////////////////////
void LoaderFileTypeRegistry::
record_extension(const string &extension, LoaderFileType *type) {
  string dcextension = downcase(extension);
  Extensions::const_iterator ei;
  ei = _extensions.find(dcextension);
  if (ei != _extensions.end()) {
    if (loader_cat->is_debug()) {
      loader_cat->debug()
        << "Multiple LoaderFileTypes registered that use the extension "
        << dcextension << "\n";
    }
  } else {
    _extensions.insert(Extensions::value_type(dcextension, type));
  }

  _deferred_types.erase(dcextension);
}
