// Filename: config_dxml.cxx
// Created by: drose (08Aug09)
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

#include "config_dxml.h"
#include "dconfig.h"

BEGIN_PUBLISH
#include "tinyxml.h"
END_PUBLISH

Configure(config_dxml);
NotifyCategoryDef(dxml, "");

ConfigureFn(config_dxml) {
  init_libdxml();
}

////////////////////////////////////////////////////////////////////
//     Function: init_libdxml
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libdxml() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;
}

BEGIN_PUBLISH
////////////////////////////////////////////////////////////////////
//     Function: read_xml_stream
//  Description: Reads an XML document from the indicated stream.
//               Returns the document, or NULL on error.
////////////////////////////////////////////////////////////////////
TiXmlDocument *
read_xml_stream(istream &in) {
  TiXmlDocument *doc = new TiXmlDocument;
  in >> *doc;
  if (in.fail() && !in.eof()) {
    delete doc;
    return NULL;
  }

  return doc;
}
END_PUBLISH

BEGIN_PUBLISH
////////////////////////////////////////////////////////////////////
//     Function: write_xml_stream
//  Description: Writes an XML document to the indicated stream.
////////////////////////////////////////////////////////////////////
void
write_xml_stream(ostream &out, TiXmlDocument *doc) {
  out << *doc;
}
END_PUBLISH
