// Filename: eggAttributes.h
// Created by:  drose (16Jan99)
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

#ifndef EGGATTRIBUTES_H
#define EGGATTRIBUTES_H

#include "pandabase.h"

#include "eggMorphList.h"
#include "eggParameters.h"
#include "typedObject.h"
#include "luse.h"
#include "pnotify.h"

////////////////////////////////////////////////////////////////////
//       Class : EggAttributes
// Description : The set of attributes that may be applied to vertices
//               as well as polygons, such as surface normal and
//               color.
//
//               This class cannot inherit from EggObject, because it
//               causes problems at the EggPolygon level with multiple
//               appearances of the EggObject base class.  And making
//               EggObject a virtual base class is just no fun.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEGG EggAttributes {
PUBLISHED:
  EggAttributes();
  EggAttributes(const EggAttributes &copy);
  EggAttributes &operator = (const EggAttributes &copy);
  virtual ~EggAttributes();

  INLINE bool has_normal() const;
  INLINE const Normald &get_normal() const;
  INLINE void set_normal(const Normald &normal);
  INLINE void clear_normal();
  INLINE bool matches_normal(const EggAttributes &other) const;
  INLINE void copy_normal(const EggAttributes &other);

  INLINE bool has_color() const;
  INLINE Colorf get_color() const;
  INLINE void set_color(const Colorf &Color);
  INLINE void clear_color();
  INLINE bool matches_color(const EggAttributes &other) const;
  INLINE void copy_color(const EggAttributes &other);

  void write(ostream &out, int indent_level) const;
  INLINE bool sorts_less_than(const EggAttributes &other) const;
  int compare_to(const EggAttributes &other) const;

  void transform(const LMatrix4d &mat);

  EggMorphNormalList _dnormals;
  EggMorphColorList _drgbas;

private:
  enum Flags {
    F_has_normal = 0x001,
    F_has_color  = 0x002,
  };

  int _flags;
  Normald _normal;
  Colorf _color;


public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    register_type(_type_handle, "EggAttributes");
  }

private:
  static TypeHandle _type_handle;
};

#include "eggAttributes.I"

#endif

