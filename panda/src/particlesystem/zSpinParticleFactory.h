// Filename: zSpinParticleFactory.h
// Created by:  charles (16Aug00)
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

#ifndef ZSPINPARTICLEFACTORY_H
#define ZSPINPARTICLEFACTORY_H

#include "baseParticleFactory.h"

////////////////////////////////////////////////////////////////////
//       Class : ZSpinParticleFactory
// Description :
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAPHYSICS ZSpinParticleFactory : public BaseParticleFactory {
PUBLISHED:
  ZSpinParticleFactory();
  ZSpinParticleFactory(const ZSpinParticleFactory &copy);
  virtual ~ZSpinParticleFactory();

  INLINE void set_initial_angle(float angle);
  INLINE void set_final_angle(float angle);
  INLINE void set_initial_angle_spread(float spread);
  INLINE void set_final_angle_spread(float spread);

  INLINE float get_initial_angle() const;
  INLINE float get_final_angle() const;
  INLINE float get_initial_angle_spread() const;
  INLINE float get_final_angle_spread() const;

  INLINE void  set_angular_velocity(float v);
  INLINE float get_angular_velocity() const;

  INLINE void  set_angular_velocity_spread(float spread);
  INLINE float get_angular_velocity_spread() const;

  INLINE void enable_angular_velocity(bool bEnabled);
  INLINE bool get_angular_velocity_enabled() const;

  virtual void output(ostream &out) const;
  virtual void write(ostream &out, int indent=0) const;

private:
  float _initial_angle;
  float _initial_angle_spread;
  float _final_angle;
  float _final_angle_spread;
  float _angular_velocity;
  float _angular_velocity_spread;
  bool  _bUseAngularVelocity;

  virtual void populate_child_particle(BaseParticle *bp) const;
  virtual BaseParticle *alloc_particle() const;
};

#include "zSpinParticleFactory.I"

#endif // ZSPINPARTICLEFACTORY_H
