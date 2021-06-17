// Filename: sparkleParticleRenderer.h
// Created by:  charles (27Jun00)
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

#ifndef SPARKLEPARTICLERENDERER_H
#define SPARKLEPARTICLERENDERER_H

#include "baseParticle.h"
#include "baseParticleRenderer.h"
#include "pointerTo.h"
#include "pointerToArray.h"
#include "geom.h"
#include "geomVertexData.h"
#include "geomLines.h"
#include "pStatCollector.h"

enum SparkleParticleLifeScale {
  SP_NO_SCALE,
  SP_SCALE
};

////////////////////////////////////////////////////////////////////
//       Class : SparkleParticleRenderer
// Description : pretty sparkly things.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAPHYSICS SparkleParticleRenderer : public BaseParticleRenderer {
PUBLISHED:
  enum SparkleParticleLifeScale {
    SP_NO_SCALE,
    SP_SCALE
  };

  SparkleParticleRenderer();
  SparkleParticleRenderer(const SparkleParticleRenderer& copy);
  SparkleParticleRenderer(const Colorf& center,
                          const Colorf& edge,
                          float birth_radius,
                          float death_radius,
                          SparkleParticleLifeScale life_scale,
                          ParticleRendererAlphaMode alpha_mode);

  virtual ~SparkleParticleRenderer();

public:
  virtual BaseParticleRenderer *make_copy();

PUBLISHED:
  INLINE void set_center_color(const Colorf& c);
  INLINE void set_edge_color(const Colorf& c);
  INLINE void set_birth_radius(float radius);
  INLINE void set_death_radius(float radius);
  INLINE void set_life_scale(SparkleParticleLifeScale);

  INLINE const Colorf& get_center_color() const;
  INLINE const Colorf& get_edge_color() const;
  INLINE float get_birth_radius() const;
  INLINE float get_death_radius() const;
  INLINE SparkleParticleLifeScale get_life_scale() const;

  virtual void output(ostream &out) const;
  virtual void write(ostream &out, int indent_level = 0) const;

private:
  Colorf _center_color;
  Colorf _edge_color;

  float _birth_radius;
  float _death_radius;

  PT(Geom) _line_primitive;
  PT(GeomLines) _lines;
  PT(GeomVertexData) _vdata;

  int _max_pool_size;

  SparkleParticleLifeScale _life_scale;
  LPoint3f _aabb_min;
  LPoint3f _aabb_max;

  INLINE float get_radius(BaseParticle *bp);

  virtual void birth_particle(int index);
  virtual void kill_particle(int index);
  virtual void init_geoms();
  virtual void render(pvector< PT(PhysicsObject) >& po_vector,
                      int ttl_particles);
  virtual void resize_pool(int new_size);

  static PStatCollector _render_collector;
};

#include "sparkleParticleRenderer.I"

#endif // SPARKLEPARTICLERENDERER_H
