// Filename: geomParticleRenderer.h
// Created by:  charles (05Jul00)
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

#ifndef GEOMPARTICLERENDERER_H
#define GEOMPARTICLERENDERER_H

#include "baseParticleRenderer.h"
#include "baseParticle.h"
#include "colorInterpolationManager.h"
#include "pandaNode.h"
#include "pointerTo.h"
#include "pointerToArray.h"
#include "pvector.h"
#include "pStatCollector.h"

class EXPCL_PANDAPHYSICS GeomParticleRenderer : public BaseParticleRenderer {
PUBLISHED:
  GeomParticleRenderer(ParticleRendererAlphaMode am = PR_ALPHA_NONE,
                       PandaNode *geom_node = (PandaNode *) NULL);
  GeomParticleRenderer(const GeomParticleRenderer& copy);
  virtual ~GeomParticleRenderer();

  INLINE void set_geom_node(PandaNode *node);
  INLINE PandaNode *get_geom_node();
  INLINE ColorInterpolationManager* get_color_interpolation_manager() const;  

  INLINE void set_x_scale_flag(bool animate_x_ratio);
  INLINE void set_y_scale_flag(bool animate_y_ratio);
  INLINE void set_z_scale_flag(bool animate_z_ratio);
  INLINE void set_initial_x_scale(float initial_x_scale);
  INLINE void set_final_x_scale(float final_x_scale);
  INLINE void set_initial_y_scale(float initial_y_scale);
  INLINE void set_final_y_scale(float final_y_scale);
  INLINE void set_initial_z_scale(float initial_z_scale);
  INLINE void set_final_z_scale(float final_z_scale);

  INLINE bool get_x_scale_flag() const;
  INLINE bool get_y_scale_flag() const;
  INLINE bool get_z_scale_flag() const;
  INLINE float get_initial_x_scale() const;
  INLINE float get_final_x_scale() const;
  INLINE float get_initial_y_scale() const;
  INLINE float get_final_y_scale() const;
  INLINE float get_initial_z_scale() const;
  INLINE float get_final_z_scale() const;

public:
  virtual BaseParticleRenderer *make_copy();

  virtual void output(ostream &out) const;
  virtual void write_linear_forces(ostream &out, int indent=0) const;
  virtual void write(ostream &out, int indent=0) const;

private:
  PT(PandaNode) _geom_node;
  PT(ColorInterpolationManager) _color_interpolation_manager;

  pvector< PT(PandaNode) > _node_vector;

  int _pool_size;
  float _initial_x_scale;
  float _final_x_scale;
  float _initial_y_scale;
  float _final_y_scale;
  float _initial_z_scale;
  float _final_z_scale;

  bool _animate_x_ratio;
  bool _animate_y_ratio;
  bool _animate_z_ratio;

  // geomparticlerenderer takes advantage of the birth/death functions

  virtual void birth_particle(int index);
  virtual void kill_particle(int index);

  virtual void init_geoms();
  virtual void render(pvector< PT(PhysicsObject) >& po_vector,
                      int ttl_particles);

  virtual void resize_pool(int new_size);
  void kill_nodes();

  static PStatCollector _render_collector;
};

#include "geomParticleRenderer.I"

#endif // GEOMPARTICLERENDERER_H
