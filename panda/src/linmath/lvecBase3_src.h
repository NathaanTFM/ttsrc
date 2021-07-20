// Filename: lvecBase3_src.h
// Created by:  drose (08Mar00)
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

////////////////////////////////////////////////////////////////////
//       Class : LVecBase3
// Description : This is the base class for all three-component
//               vectors and points.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA_LINMATH FLOATNAME(LVecBase3) {
PUBLISHED:
  typedef const FLOATTYPE *iterator;
  typedef const FLOATTYPE *const_iterator;

  INLINE_LINMATH FLOATNAME(LVecBase3)();
  INLINE_LINMATH FLOATNAME(LVecBase3)(const FLOATNAME(LVecBase3) &copy);
  INLINE_LINMATH FLOATNAME(LVecBase3) &operator = (const FLOATNAME(LVecBase3) &copy);
  INLINE_LINMATH FLOATNAME(LVecBase3) &operator = (FLOATTYPE fill_value);
  INLINE_LINMATH FLOATNAME(LVecBase3)(FLOATTYPE fill_value);
  INLINE_LINMATH FLOATNAME(LVecBase3)(FLOATTYPE x, FLOATTYPE y, FLOATTYPE z);
  ALLOC_DELETED_CHAIN(FLOATNAME(LVecBase3));

  INLINE_LINMATH static const FLOATNAME(LVecBase3) &zero();
  INLINE_LINMATH static const FLOATNAME(LVecBase3) &unit_x();
  INLINE_LINMATH static const FLOATNAME(LVecBase3) &unit_y();
  INLINE_LINMATH static const FLOATNAME(LVecBase3) &unit_z();

  INLINE_LINMATH ~FLOATNAME(LVecBase3)();

#ifdef HAVE_PYTHON
  PyObject *__reduce__(PyObject *self) const;
#endif

  INLINE_LINMATH FLOATTYPE operator [](int i) const;
  INLINE_LINMATH FLOATTYPE &operator [](int i);
#ifdef HAVE_PYTHON
  INLINE_LINMATH void __setitem__(int i, FLOATTYPE v);
#endif
  INLINE_LINMATH static int size();

  INLINE_LINMATH bool is_nan() const;

  INLINE_LINMATH FLOATTYPE get_cell(int i) const;
  INLINE_LINMATH FLOATTYPE get_x() const;
  INLINE_LINMATH FLOATTYPE get_y() const;
  INLINE_LINMATH FLOATTYPE get_z() const;

  INLINE_LINMATH void set_cell(int i, FLOATTYPE value);
  INLINE_LINMATH void set_x(FLOATTYPE value);
  INLINE_LINMATH void set_y(FLOATTYPE value);
  INLINE_LINMATH void set_z(FLOATTYPE value);

  INLINE_LINMATH FLOATNAME(LVecBase2) get_xy() const;
  INLINE_LINMATH FLOATNAME(LVecBase2) get_xz() const;
  INLINE_LINMATH FLOATNAME(LVecBase2) get_yz() const;

  // These next functions add to an existing value.
  // i.e. foo.set_x(foo.get_x() + value)
  // These are useful to reduce overhead in scripting
  // languages:
  INLINE_LINMATH void add_to_cell(int i, FLOATTYPE value);
  INLINE_LINMATH void add_x(FLOATTYPE value);
  INLINE_LINMATH void add_y(FLOATTYPE value);
  INLINE_LINMATH void add_z(FLOATTYPE value);

  INLINE_LINMATH const FLOATTYPE *get_data() const;
  INLINE_LINMATH int get_num_components() const;

public:
  INLINE_LINMATH iterator begin();
  INLINE_LINMATH iterator end();

  INLINE_LINMATH const_iterator begin() const;
  INLINE_LINMATH const_iterator end() const;

PUBLISHED:
  INLINE_LINMATH void fill(FLOATTYPE fill_value);
  INLINE_LINMATH void set(FLOATTYPE x, FLOATTYPE y, FLOATTYPE z);

  INLINE_LINMATH FLOATTYPE length() const;
  INLINE_LINMATH FLOATTYPE length_squared() const;
  INLINE_LINMATH bool normalize();

  INLINE_LINMATH FLOATTYPE dot(const FLOATNAME(LVecBase3) &other) const;
  INLINE_LINMATH FLOATNAME(LVecBase3) cross(const FLOATNAME(LVecBase3) &other) const;
  INLINE_LINMATH FLOATNAME(LVecBase3) project(const FLOATNAME(LVecBase3) &onto) const;

  INLINE_LINMATH bool operator < (const FLOATNAME(LVecBase3) &other) const;
  INLINE_LINMATH bool operator == (const FLOATNAME(LVecBase3) &other) const;
  INLINE_LINMATH bool operator != (const FLOATNAME(LVecBase3) &other) const;

  INLINE_LINMATH FLOATNAME(LVecBase3) get_standardized_hpr() const;

  INLINE_LINMATH int compare_to(const FLOATNAME(LVecBase3) &other) const;
  INLINE_LINMATH int compare_to(const FLOATNAME(LVecBase3) &other,
                                FLOATTYPE threshold) const;
  INLINE_LINMATH size_t get_hash() const;
  INLINE_LINMATH size_t get_hash(FLOATTYPE threshold) const;
  INLINE_LINMATH size_t add_hash(size_t hash) const;
  INLINE_LINMATH size_t add_hash(size_t hash, FLOATTYPE threshold) const;

  INLINE_LINMATH FLOATNAME(LVecBase3) operator - () const;

  INLINE_LINMATH FLOATNAME(LVecBase3)
  operator + (const FLOATNAME(LVecBase3) &other) const;
  INLINE_LINMATH FLOATNAME(LVecBase3)
  operator - (const FLOATNAME(LVecBase3) &other) const;

  INLINE_LINMATH FLOATNAME(LVecBase3) operator * (FLOATTYPE scalar) const;
  INLINE_LINMATH FLOATNAME(LVecBase3) operator / (FLOATTYPE scalar) const;

  INLINE_LINMATH void operator += (const FLOATNAME(LVecBase3) &other);
  INLINE_LINMATH void operator -= (const FLOATNAME(LVecBase3) &other);

  INLINE_LINMATH void operator *= (FLOATTYPE scalar);
  INLINE_LINMATH void operator /= (FLOATTYPE scalar);

  INLINE_LINMATH FLOATNAME(LVecBase3) fmax(const FLOATNAME(LVecBase3) &other);
  INLINE_LINMATH FLOATNAME(LVecBase3) fmin(const FLOATNAME(LVecBase3) &other);

  INLINE_LINMATH void cross_into(const FLOATNAME(LVecBase3) &other);

  INLINE_LINMATH bool almost_equal(const FLOATNAME(LVecBase3) &other,
                           FLOATTYPE threshold) const;
  INLINE_LINMATH bool almost_equal(const FLOATNAME(LVecBase3) &other) const;

  INLINE_LINMATH void output(ostream &out) const;
#ifdef HAVE_PYTHON
  INLINE_LINMATH void python_repr(ostream &out, const string &class_name) const;
#endif

public:
  INLINE_LINMATH void generate_hash(ChecksumHashGenerator &hashgen) const;
  INLINE_LINMATH void generate_hash(ChecksumHashGenerator &hashgen,
                                    FLOATTYPE threshold) const;

public:
  union {
        FLOATTYPE data[3];
        struct {FLOATTYPE _0, _1, _2;} v;
   } _v;

private:
  static const FLOATNAME(LVecBase3) _zero;
  static const FLOATNAME(LVecBase3) _unit_x;
  static const FLOATNAME(LVecBase3) _unit_y;
  static const FLOATNAME(LVecBase3) _unit_z;

public:
  INLINE_LINMATH void write_datagram(Datagram &destination) const;
  INLINE_LINMATH void read_datagram(DatagramIterator &source);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type();

private:
  static TypeHandle _type_handle;
};


INLINE_LINMATH ostream &operator << (ostream &out, const FLOATNAME(LVecBase3) &vec) {
  vec.output(out);
  return out;
};

#include "lvecBase3_src.I"
