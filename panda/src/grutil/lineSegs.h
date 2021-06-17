// Filename: lineSegs.h
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

#ifndef LINESEGS_H
#define LINESEGS_H

#include "pandabase.h"

#include "luse.h"
#include "geom.h"
#include "geomNode.h"
#include "geomVertexData.h"
#include "namable.h"

#include "pvector.h"

////////////////////////////////////////////////////////////////////
//       Class : LineSegs
// Description : Encapsulates creation of a series of connected or
//               disconnected line segments or points, for drawing
//               paths or rays.  This class doesn't attempt to be the
//               smartest it could possibly be; it's intended
//               primarily as a visualization and editing tool.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_GRUTIL LineSegs : public Namable {
PUBLISHED:
  LineSegs(const string &name = "lines");
  ~LineSegs();

  void reset();
  INLINE void set_color(float r, float g, float b, float a = 1.0f);
  INLINE void set_color(const Colorf &color);
  INLINE void set_thickness(float thick);

  INLINE void move_to(float x, float y, float z);
  void move_to(const LVecBase3f &v);

  INLINE void draw_to(float x, float y, float z);
  void draw_to(const LVecBase3f &v);

  const Vertexf &get_current_position();
  bool is_empty();

  INLINE GeomNode *create(bool dynamic = false);
  GeomNode *create(GeomNode *previous, bool dynamic = false);

  // Functions to move the line vertices after they have been created.
  INLINE int get_num_vertices() const;
  Vertexf get_vertex(int n) const;
  MAKE_SEQ(get_vertices, get_num_vertices, get_vertex);
  void set_vertex(int n, const Vertexf &vert);
  INLINE void set_vertex(int vertex, float x, float y, float z);

  Colorf get_vertex_color(int vertex) const;
  MAKE_SEQ(get_vertex_colors, get_num_vertices, get_vertex_color);
  void set_vertex_color(int vertex, const Colorf &c);
  INLINE void set_vertex_color(int vertex, float r, float g, float b, float a = 1.0f);

private:
  class Point {
  public:
    INLINE Point();
    INLINE Point(const LVecBase3f &point, const Colorf &color);
    INLINE Point(const Point &copy);
    INLINE void operator = (const Point &copy);

    Vertexf _point;
    Colorf _color;
  };

  typedef pvector<Point> SegmentList;
  typedef pvector<SegmentList> LineList;

  LineList _list;
  Colorf _color;
  float _thick;

  PT(GeomVertexData) _created_data;
};

#include "lineSegs.I"

#endif
