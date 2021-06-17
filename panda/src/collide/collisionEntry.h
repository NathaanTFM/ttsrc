// Filename: collisionEntry.h
// Created by:  drose (16Mar02)
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

#ifndef COLLISIONENTRY_H
#define COLLISIONENTRY_H

#include "pandabase.h"

#include "collisionTraverser.h"
#include "collisionSolid.h"
#include "collisionNode.h"
#include "collisionRecorder.h"

#include "transformState.h"
#include "typedWritableReferenceCount.h"
#include "luse.h"
#include "pointerTo.h"
#include "pandaNode.h"
#include "nodePath.h"
#include "clipPlaneAttrib.h"

////////////////////////////////////////////////////////////////////
//       Class : CollisionEntry
// Description : Defines a single collision event.  One of these is
//               created for each collision detected by a
//               CollisionTraverser, to be dealt with by the
//               CollisionHandler.
//
//               A CollisionEntry provides slots for a number of data
//               values (such as intersection point and normal) that
//               might or might not be known for each collision.  It
//               is up to the handler to determine what information is
//               known and to do the right thing with it.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_COLLIDE CollisionEntry : public TypedWritableReferenceCount {
public:
  INLINE CollisionEntry();
  CollisionEntry(const CollisionEntry &copy);
  void operator = (const CollisionEntry &copy);

PUBLISHED:
  INLINE const CollisionSolid *get_from() const;
  INLINE bool has_into() const;
  INLINE const CollisionSolid *get_into() const;

  INLINE CollisionNode *get_from_node() const;
  INLINE PandaNode *get_into_node() const;
  INLINE NodePath get_from_node_path() const;
  INLINE NodePath get_into_node_path() const;

  INLINE void set_t(float t);
  INLINE float get_t() const;
  INLINE bool collided() const;
  INLINE void reset_collided();

  INLINE bool get_respect_prev_transform() const;

  INLINE void set_surface_point(const LPoint3f &point);
  INLINE void set_surface_normal(const LVector3f &normal);
  INLINE void set_interior_point(const LPoint3f &point);

  INLINE bool has_surface_point() const;
  INLINE bool has_surface_normal() const;
  INLINE bool has_interior_point() const;

  INLINE void set_contact_pos(const LPoint3f &pos);
  INLINE void set_contact_normal(const LVector3f &normal);

  INLINE bool has_contact_pos() const;
  INLINE bool has_contact_normal() const;

  LPoint3f get_surface_point(const NodePath &space) const;
  LVector3f get_surface_normal(const NodePath &space) const;
  LPoint3f get_interior_point(const NodePath &space) const;
  bool get_all(const NodePath &space,
               LPoint3f &surface_point,
               LVector3f &surface_normal,
               LPoint3f &interior_point) const;

  LPoint3f get_contact_pos(const NodePath &space) const;
  LVector3f get_contact_normal(const NodePath &space) const;
  bool get_all_contact_info(const NodePath &space,
                            LPoint3f &contact_pos,
                            LVector3f &contact_normal) const;

  void output(ostream &out) const;
  void write(ostream &out, int indent_level = 0) const;

public:
  INLINE CPT(TransformState) get_wrt_space() const;
  INLINE CPT(TransformState) get_inv_wrt_space() const;
  INLINE CPT(TransformState) get_wrt_prev_space() const;

  INLINE const LMatrix4f &get_wrt_mat() const;
  INLINE const LMatrix4f &get_inv_wrt_mat() const;
  INLINE const LMatrix4f &get_wrt_prev_mat() const;

  INLINE const ClipPlaneAttrib *get_into_clip_planes() const;

private:
  INLINE void test_intersection(CollisionHandler *record, 
                                const CollisionTraverser *trav) const;
  void check_clip_planes();

  CPT(CollisionSolid) _from;
  CPT(CollisionSolid) _into;

  PT(CollisionNode) _from_node;
  PT(PandaNode) _into_node;
  NodePath _from_node_path;
  NodePath _into_node_path;
  CPT(ClipPlaneAttrib) _into_clip_planes;
  float _t;

  enum Flags {
    F_has_surface_point       = 0x0001,
    F_has_surface_normal      = 0x0002,
    F_has_interior_point      = 0x0004,
    F_respect_prev_transform  = 0x0008,
    F_checked_clip_planes     = 0x0010,
    F_has_contact_pos         = 0x0020,
    F_has_contact_normal      = 0x0040,
  };

  int _flags;

  LPoint3f _surface_point;
  LVector3f _surface_normal;
  LPoint3f _interior_point;

  LPoint3f _contact_pos;
  LVector3f _contact_normal;
  
public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedWritableReferenceCount::init_type();
    register_type(_type_handle, "CollisionEntry",
                  TypedWritableReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class CollisionTraverser;
  friend class CollisionHandlerFluidPusher;
};

INLINE ostream &operator << (ostream &out, const CollisionEntry &entry);

#include "collisionEntry.I"

#endif



