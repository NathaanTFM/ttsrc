// Filename: physxHeightField.h
// Created by:  enn0x (15Oct09)
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

#ifndef PHYSXHEIGHTFIELD_H
#define PHYSXHEIGHTFIELD_H

#include "pandabase.h"
#include "lvector3.h"

#include "physxObject.h"
#include "physxContactReport.h"
#include "physxControllerReport.h"
#include "physxTriggerReport.h"
#include "physx_includes.h"

////////////////////////////////////////////////////////////////////
//       Class : PhysxHeightField
// Description : A height field object. 
//               Height fields work in a similar way as triangle
//               meshes specified to act as height fields, with
//               some important differences:
//
//               Triangle meshes can be made of nonuniform geometry,
//               while height fields are regular, rectangular grids.
//               This means that with PhysxHeightField, you
//               sacrifice flexibility in return for improved
//               performance and decreased memory consumption.
//
//               Height fields are referenced by shape instances of
//               type PhysxHeightFieldShape.
//
//               To create an instance of this class call
//               PhysxManager::create_height_field(), and
//               PhysxHeightField::release() to release it. This is
//               only possible once you have released all of its
//               PhysxHeightFiedShape instances before.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAPHYSX PhysxHeightField : public PhysxObject {

PUBLISHED:
  void release();

  unsigned int get_reference_count() const;
  float get_height(float x, float y) const;

  INLINE void ls() const;
  INLINE void ls(ostream &out, int indent_level=0) const;

public:
  INLINE PhysxHeightField();
  INLINE ~PhysxHeightField();

  INLINE NxHeightField *ptr() const { return _ptr; };

  void link(NxHeightField *ptr);
  void unlink();

private:
  NxHeightField *_ptr;

////////////////////////////////////////////////////////////////////
public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    PhysxObject::init_type();
    register_type(_type_handle, "PhysxHeightField", 
                  PhysxObject::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {
    init_type();
    return get_class_type();
  }

private:
  static TypeHandle _type_handle;
};

#include "physxHeightField.I"

#endif // PHYSXHEIGHTFIELD_H
