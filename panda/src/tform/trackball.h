// Filename: trackball.h
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

#ifndef TRACKBALL_H
#define TRACKBALL_H

#include "pandabase.h"

#include "mouseInterfaceNode.h"
#include "nodePath.h"
#include "modifierButtons.h"
#include "luse.h"
#include "transformState.h"


////////////////////////////////////////////////////////////////////
//       Class : Trackball
// Description : Trackball acts like Performer in trackball mode.  It
//               can either spin around a piece of geometry directly,
//               or it can spin around a camera with the inverse
//               transform to make it appear that the whole world is
//               spinning.
//
//               The Trackball object actually just places a transform
//               in the data graph; parent a Transform2SG node under
//               it to actually transform objects (or cameras) in the
//               world.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_TFORM Trackball : public MouseInterfaceNode {
PUBLISHED:
  Trackball(const string &name);
  ~Trackball();

  void reset();

  float get_forward_scale() const;
  void set_forward_scale(float fwdscale);

  /// **** Translation ****

  const LPoint3f &get_pos() const;
  float get_x() const;
  float get_y() const;
  float get_z() const;
  void set_pos(const LVecBase3f &vec);
  void set_pos(float x, float y, float z);
  void set_x(float x);
  void set_y(float y);
  void set_z(float z);

  /// **** Rotation ****

  LVecBase3f get_hpr() const;
  float get_h() const;
  float get_p() const;
  float get_r() const;
  void set_hpr(const LVecBase3f &hpr);
  void set_hpr(float h, float p, float r);
  void set_h(float h);
  void set_p(float p);
  void set_r(float r);

  /// **** Origin of Rotation ****

  void reset_origin_here();
  void move_origin(float x, float y, float z);

  LPoint3f get_origin() const;
  void set_origin(const LVecBase3f &origin);

  /// **** Misc ****

  void set_invert(bool flag);
  bool get_invert() const;

  void set_rel_to(const NodePath &rel_to);
  const NodePath &get_rel_to() const;

  void set_coordinate_system(CoordinateSystem cs);
  CoordinateSystem get_coordinate_system() const;

  void set_mat(const LMatrix4f &mat);
  const LMatrix4f &get_mat() const;
  const LMatrix4f &get_trans_mat() const;


private:
  void apply(double x, double y, int button);

  void reextract();
  void recompute();

  int _last_button;
  float _lastx, _lasty;

  float _rotscale;
  float _fwdscale;

  LMatrix4f _rotation;
  LPoint3f _translation;
  LMatrix4f _mat, _orig;
  bool _invert;
  NodePath _rel_to;
  CoordinateSystem _cs;

protected:
  // Inherited from DataNode
  virtual void do_transmit_data(DataGraphTraverser *trav,
                                const DataNodeTransmit &input,
                                DataNodeTransmit &output);

private:
  // inputs
  int _pixel_xy_input;

  // outputs
  int _transform_output;

  CPT(TransformState) _transform;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    MouseInterfaceNode::init_type();
    register_type(_type_handle, "Trackball",
                  MouseInterfaceNode::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#endif
