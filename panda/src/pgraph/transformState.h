// Filename: transformState.h
// Created by:  drose (25Feb02)
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

#ifndef TRANSFORMSTATE_H
#define TRANSFORMSTATE_H

#include "pandabase.h"

#include "nodeCachedReferenceCount.h"
#include "pointerTo.h"
#include "luse.h"
#include "pset.h"
#include "event.h"
#include "updateSeq.h"
#include "pStatCollector.h"
#include "geomEnums.h"
#include "lightReMutex.h"
#include "lightReMutexHolder.h"
#include "lightMutex.h"
#include "lightMutexHolder.h"
#include "config_pgraph.h"
#include "deletedChain.h"
#include "simpleHashMap.h"
#include "cacheStats.h"

class GraphicsStateGuardianBase;
class FactoryParams;

////////////////////////////////////////////////////////////////////
//       Class : TransformState
// Description : Indicates a coordinate-system transform on vertices.
//               TransformStates are the primary means for storing
//               transformations on the scene graph.
//
//               Transforms may be specified in one of two ways:
//               componentwise, with a pos-hpr-scale, or with an
//               arbitrary transform matrix.  If you specify a
//               transform componentwise, it will remember its
//               original components.
//
//               TransformState objects are managed very much like
//               RenderState objects.  They are immutable and
//               reference-counted automatically.
//
//               You should not attempt to create or modify a
//               TransformState object directly.  Instead, call one of
//               the make() functions to create one for you.  And
//               instead of modifying a TransformState object, create a
//               new one.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_PGRAPH TransformState : public NodeCachedReferenceCount {
protected:
  TransformState();

private:
  TransformState(const TransformState &copy);
  void operator = (const TransformState &copy);

public:
  virtual ~TransformState();
  ALLOC_DELETED_CHAIN(TransformState);

PUBLISHED:
  INLINE bool operator < (const TransformState &other) const;
  bool sorts_less(const TransformState &other, bool uniquify_matrix) const;
  INLINE size_t get_hash() const;

  static CPT(TransformState) make_identity();
  static CPT(TransformState) make_invalid();
  INLINE static CPT(TransformState) make_pos(const LVecBase3f &pos);
  INLINE static CPT(TransformState) make_hpr(const LVecBase3f &hpr);
  INLINE static CPT(TransformState) make_quat(const LQuaternionf &quat);
  INLINE static CPT(TransformState) make_pos_hpr(const LVecBase3f &pos,
                                                 const LVecBase3f &hpr);
  INLINE static CPT(TransformState) make_scale(float scale);
  INLINE static CPT(TransformState) make_scale(const LVecBase3f &scale);
  INLINE static CPT(TransformState) make_shear(const LVecBase3f &shear);
  INLINE static CPT(TransformState) make_pos_hpr_scale(const LVecBase3f &pos,
                                                       const LVecBase3f &hpr, 
                                                       const LVecBase3f &scale);
  INLINE static CPT(TransformState) make_pos_quat_scale(const LVecBase3f &pos,
                                                        const LQuaternionf &quat, 
                                                        const LVecBase3f &scale);
  static CPT(TransformState) make_pos_hpr_scale_shear(const LVecBase3f &pos,
                                                      const LVecBase3f &hpr, 
                                                      const LVecBase3f &scale,
                                                      const LVecBase3f &shear);
  static CPT(TransformState) make_pos_quat_scale_shear(const LVecBase3f &pos,
                                                       const LQuaternionf &quat, 
                                                       const LVecBase3f &scale,
                                                       const LVecBase3f &shear);
  static CPT(TransformState) make_mat(const LMatrix4f &mat);


  INLINE static CPT(TransformState) make_pos2d(const LVecBase2f &pos);
  INLINE static CPT(TransformState) make_rotate2d(float rotate);
  INLINE static CPT(TransformState) make_pos_rotate2d(const LVecBase2f &pos,
                                                      float rotate);
  INLINE static CPT(TransformState) make_scale2d(float scale);
  INLINE static CPT(TransformState) make_scale2d(const LVecBase2f &scale);
  INLINE static CPT(TransformState) make_shear2d(float shear);
  INLINE static CPT(TransformState) make_pos_rotate_scale2d(const LVecBase2f &pos,
                                                            float rotate, 
                                                            const LVecBase2f &scale);
  static CPT(TransformState) make_pos_rotate_scale_shear2d(const LVecBase2f &pos,
                                                           float rotate,
                                                           const LVecBase2f &scale,
                                                           float shear);
  static CPT(TransformState) make_mat3(const LMatrix3f &mat);


  INLINE bool is_identity() const;
  INLINE bool is_invalid() const;
  INLINE bool is_singular() const;
  INLINE bool is_2d() const;

  INLINE bool has_components() const;
  INLINE bool components_given() const;
  INLINE bool hpr_given() const;
  INLINE bool quat_given() const;
  INLINE bool has_pos() const;
  INLINE bool has_hpr() const;
  INLINE bool has_quat() const;
  INLINE bool has_scale() const;
  INLINE bool has_identity_scale() const;
  INLINE bool has_uniform_scale() const;
  INLINE bool has_shear() const;
  INLINE bool has_nonzero_shear() const;
  INLINE bool has_mat() const;

  INLINE const LPoint3f &get_pos() const;
  INLINE const LVecBase3f &get_hpr() const;
  INLINE const LQuaternionf &get_quat() const;
  INLINE const LQuaternionf &get_norm_quat() const;
  INLINE const LVecBase3f &get_scale() const;
  INLINE float get_uniform_scale() const;
  INLINE const LVecBase3f &get_shear() const;
  INLINE const LMatrix4f &get_mat() const;

  INLINE LVecBase2f get_pos2d() const;
  INLINE float get_rotate2d() const;
  INLINE LVecBase2f get_scale2d() const;
  INLINE float get_shear2d() const;
  INLINE LMatrix3f get_mat3() const;

  CPT(TransformState) set_pos(const LVecBase3f &pos) const;
  CPT(TransformState) set_hpr(const LVecBase3f &hpr) const;
  CPT(TransformState) set_quat(const LQuaternionf &quat) const;
  CPT(TransformState) set_scale(const LVecBase3f &scale) const;
  CPT(TransformState) set_shear(const LVecBase3f &shear) const;

  CPT(TransformState) set_pos2d(const LVecBase2f &pos) const;
  CPT(TransformState) set_rotate2d(float rotate) const;
  CPT(TransformState) set_scale2d(const LVecBase2f &scale) const;
  CPT(TransformState) set_shear2d(float shear) const;

  CPT(TransformState) compose(const TransformState *other) const;
  CPT(TransformState) invert_compose(const TransformState *other) const;

  INLINE CPT(TransformState) get_inverse() const;
  INLINE CPT(TransformState) get_unique() const;

  INLINE int get_geom_rendering(int geom_rendering) const;

  virtual bool unref() const;

  INLINE void cache_ref() const;
  INLINE bool cache_unref() const;
  INLINE void node_ref() const;
  INLINE bool node_unref() const;

  INLINE int get_composition_cache_num_entries() const;
  INLINE int get_invert_composition_cache_num_entries() const;

  INLINE int get_composition_cache_size() const;
  INLINE const TransformState *get_composition_cache_source(int n) const;
  INLINE const TransformState *get_composition_cache_result(int n) const;
  INLINE int get_invert_composition_cache_size() const;
  INLINE const TransformState *get_invert_composition_cache_source(int n) const;
  INLINE const TransformState *get_invert_composition_cache_result(int n) const;
#ifdef HAVE_PYTHON
  PyObject *get_composition_cache() const;
  PyObject *get_invert_composition_cache() const;
#endif  // HAVE_PYTHON

  void output(ostream &out) const;
  void write(ostream &out, int indent_level) const;
  void write_composition_cache(ostream &out, int indent_level) const;

  static int get_num_states();
  static int get_num_unused_states();
  static int clear_cache();
  static void list_cycles(ostream &out);
  static void list_states(ostream &out);
  static bool validate_states();
#ifdef HAVE_PYTHON
  static PyObject *get_states();
#endif  // HAVE_PYTHON


public:
  static void init_states();

  INLINE static void flush_level();

private:
  INLINE bool do_cache_unref() const;
  INLINE bool do_node_unref() const;

  class CompositionCycleDescEntry {
  public:
    INLINE CompositionCycleDescEntry(const TransformState *obj,
                                     const TransformState *result,
                                     bool inverted);

    const TransformState *_obj;
    const TransformState *_result;
    bool _inverted;
  };
  typedef pvector<CompositionCycleDescEntry> CompositionCycleDesc;

  static CPT(TransformState) return_new(TransformState *state);
  static CPT(TransformState) return_unique(TransformState *state);

  CPT(TransformState) do_compose(const TransformState *other) const;
  CPT(TransformState) do_invert_compose(const TransformState *other) const;
  static bool r_detect_cycles(const TransformState *start_state,
                              const TransformState *current_state,
                              int length, UpdateSeq this_seq,
                              CompositionCycleDesc *cycle_desc);
  static bool r_detect_reverse_cycles(const TransformState *start_state,
                                      const TransformState *current_state,
                                      int length, UpdateSeq this_seq,
                                      CompositionCycleDesc *cycle_desc);

  void release_new();
  void remove_cache_pointers();

private:
  // This mutex protects _states.  It also protects any modification
  // to the cache, which is encoded in _composition_cache and
  // _invert_composition_cache.
  static LightReMutex *_states_lock;
  typedef phash_set<const TransformState *, indirect_less_hash<const TransformState *> > States;
  static States *_states;
  static CPT(TransformState) _identity_state;
  static CPT(TransformState) _invalid_state;

  // This iterator records the entry corresponding to this TransformState
  // object in the above global set.  We keep the iterator around so
  // we can remove it when the TransformState destructs.
  States::iterator _saved_entry;

  // This data structure manages the job of caching the composition of
  // two TransformStates.  It's complicated because we have to be sure to
  // remove the entry if *either* of the input TransformStates destructs.
  // To implement this, we always record Composition entries in pairs,
  // one in each of the two involved TransformState objects.
    
  // The first element of the map is the object we compose with.  This
  // is not reference counted within this map; instead we store a
  // companion pointer in the other object, and remove the references
  // explicitly when either object destructs.
  class Composition {
  public:
    INLINE Composition();
    INLINE Composition(const Composition &copy);

    // _result is reference counted if and only if it is not the same
    // pointer as this.
    const TransformState *_result;
  };

  typedef SimpleHashMap<const TransformState *, Composition, pointer_hash> CompositionCache;
  CompositionCache _composition_cache;
  CompositionCache _invert_composition_cache;

  // This is used to mark nodes as we visit them to detect cycles.
  UpdateSeq _cycle_detect;
  static UpdateSeq _last_cycle_detect;

  static PStatCollector _cache_update_pcollector;
  static PStatCollector _transform_compose_pcollector;
  static PStatCollector _transform_invert_pcollector;
  static PStatCollector _transform_calc_pcollector;
  static PStatCollector _transform_break_cycles_pcollector;
  static PStatCollector _transform_new_pcollector;
  static PStatCollector _transform_validate_pcollector;
  static PStatCollector _transform_hash_pcollector;

  static PStatCollector _node_counter;
  static PStatCollector _cache_counter;

private:
  // This is the actual data within the TransformState.
  INLINE void check_hash() const;
  INLINE void check_singular() const;
  INLINE void check_components() const;
  INLINE void check_hpr() const;
  INLINE void check_quat() const;
  INLINE void check_norm_quat() const;
  INLINE void check_mat() const;
  INLINE void calc_hash();
  void do_calc_hash();
  void calc_singular();
  INLINE void calc_components();
  void do_calc_components();
  INLINE void calc_hpr();
  void do_calc_hpr();
  void calc_quat();
  void calc_norm_quat();
  INLINE void calc_mat();
  void do_calc_mat();

  INLINE void check_uniform_scale();
  INLINE void check_uniform_scale2d();

  INLINE void set_destructing();
  INLINE bool is_destructing() const;

  INLINE void consider_update_pstats(int old_referenced_bits) const;
  static void update_pstats(int old_referenced_bits, int new_referenced_bits);

  enum Flags {
    F_is_identity        = 0x00000001,
    F_is_singular        = 0x00000002,
    F_singular_known     = 0x00000004,  // set if we know F_is_singular
    F_components_given   = 0x00000008,
    F_components_known   = 0x00000010,  // set if we know F_has_components
    F_has_components     = 0x00000020,
    F_mat_known          = 0x00000040,  // set if _mat is defined
    F_is_invalid         = 0x00000080,
    F_quat_given         = 0x00000100,
    F_quat_known         = 0x00000200,  // set if _quat is defined
    F_hpr_given          = 0x00000400,
    F_hpr_known          = 0x00000800,  // set if _hpr is defined
    F_uniform_scale      = 0x00001000,
    F_identity_scale     = 0x00002000,
    F_has_nonzero_shear  = 0x00004000,
    F_is_destructing     = 0x00008000,
    F_is_2d              = 0x00010000,
    F_hash_known         = 0x00020000,
    F_norm_quat_known    = 0x00040000,
  };
  LPoint3f _pos;
  LVecBase3f _hpr, _scale, _shear;
  LQuaternionf _quat, _norm_quat;
  LMatrix4f _mat;
  LMatrix4f *_inv_mat;
  size_t _hash;
  
  unsigned int _flags;

  // This mutex protects _flags, and all of the above computed values.
  LightMutex _lock;

  static CacheStats _cache_stats;

public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter *manager, Datagram &dg);
  static PT(TypedWritableReferenceCount) change_this(TypedWritableReferenceCount *old_ptr, BamReader *manager);

protected:
  static TypedWritable *make_from_bam(const FactoryParams &params);
  void fillin(DatagramIterator &scan, BamReader *manager);
  
public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    NodeCachedReferenceCount::init_type();
    register_type(_type_handle, "TransformState",
                  NodeCachedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

INLINE ostream &operator << (ostream &out, const TransformState &state) {
  state.output(out);
  return out;
}

#include "transformState.I"

#endif

