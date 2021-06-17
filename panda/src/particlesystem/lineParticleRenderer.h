// Filename: lineParticleRenderer.h
// Created by:  darren (06Oct00)
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

#ifndef LINEPARTICLERENDERER_H
#define LINEPARTICLERENDERER_H

#include "baseParticle.h"
#include "baseParticleRenderer.h"
#include "pointerTo.h"
#include "pointerToArray.h"
#include "geom.h"
#include "geomVertexData.h"
#include "geomLines.h"
#include "pStatCollector.h"

////////////////////////////////////////////////////////////////////
//       Class : LineParticleRenderer
// Description : renders a line from last position to current
//               position -- good for rain, sparks, etc.
////////////////////////////////////////////////////////////////////

class EXPCL_PANDAPHYSICS LineParticleRenderer : public BaseParticleRenderer {
PUBLISHED:
  LineParticleRenderer();
  LineParticleRenderer(const LineParticleRenderer& copy);
  LineParticleRenderer(const Colorf& head,
                       const Colorf& tail,
                       ParticleRendererAlphaMode alpha_mode);

  virtual ~LineParticleRenderer();

public:
  virtual BaseParticleRenderer *make_copy();

PUBLISHED:
  INLINE void set_head_color(const Colorf& c);
  INLINE void set_tail_color(const Colorf& c);

  INLINE const Colorf& get_head_color() const;
  INLINE const Colorf& get_tail_color() const;
  
  INLINE void set_line_scale_factor(float sf);
  INLINE float get_line_scale_factor() const;

  virtual void output(ostream &out) const;
  virtual void write(ostream &out, int indent_level = 0) const;

private:
  Colorf _head_color;
  Colorf _tail_color;

  PT(Geom) _line_primitive;
  PT(GeomLines) _lines;
  PT(GeomVertexData) _vdata;

  int _max_pool_size;

  LPoint3f _aabb_min;
  LPoint3f _aabb_max;

  float _line_scale_factor;

  virtual void birth_particle(int index);
  virtual void kill_particle(int index);
  virtual void init_geoms();
  virtual void render(pvector< PT(PhysicsObject) >& po_vector,
                      int ttl_particles);
  virtual void resize_pool(int new_size);

  static PStatCollector _render_collector;
};

#include "lineParticleRenderer.I"

#endif // LINEPARTICLERENDERER_H
