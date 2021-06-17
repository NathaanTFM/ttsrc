// Filename: nurbsBasisVector.h
// Created by:  drose (03Dec02)
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

#ifndef NURBSBASISVECTOR_H
#define NURBSBASISVECTOR_H

#include "pandabase.h"
#include "luse.h"
#include "pvector.h"
#include "pmap.h"

class NurbsVertex;

////////////////////////////////////////////////////////////////////
//       Class : NurbsBasisVector
// Description : This encapsulates a series of matrices that are used
//               to represent the sequential segments of a
//               NurbsCurveEvaluator.
//
//               This is not related to NurbsCurve, CubicCurveseg or
//               any of the ParametricCurve-derived objects in this
//               module.  It is a completely parallel implementation
//               of NURBS curves, and will probably eventually replace
//               the whole ParametricCurve class hierarchy.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_PARAMETRICS NurbsBasisVector {
public:
  INLINE NurbsBasisVector();
  INLINE ~NurbsBasisVector();

  INLINE int get_order() const;

  INLINE int get_num_segments() const;
  INLINE float get_start_t() const;
  INLINE float get_end_t() const;

  INLINE int get_vertex_index(int segment) const;
  INLINE float get_from(int segment) const;
  INLINE float get_to(int segment) const;
  INLINE const LMatrix4f &get_basis(int segment) const;
  INLINE float scale_t(int segment, float t) const;

  void clear(int order);
  void append_segment(int vertex_index, const float knots[]);

  void transpose();

private:
  static LVecBase4f nurbs_blending_function(int order, int i, int j, 
                                            const float knots[]);

private:
  int _order;

  class Segment {
  public:
    int _vertex_index;
    float _from;
    float _to;
    LMatrix4f _basis;
  };

  typedef pvector<Segment> Segments;
  Segments _segments;
};

#include "nurbsBasisVector.I"

#endif

