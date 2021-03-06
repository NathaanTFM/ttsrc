// Filename: typeManager.cxx
// Created by:  drose (14Aug00)
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

#include "typeManager.h"
#include "interrogate.h"

#include "cppFunctionType.h"
#include "cppFunctionGroup.h"
#include "cppParameterList.h"
#include "cppConstType.h"
#include "cppReferenceType.h"
#include "cppPointerType.h"
#include "cppSimpleType.h"
#include "cppStructType.h"
#include "cppTypeDeclaration.h"
#include "pnotify.h"
#include "cppTypedef.h"
#include "cppEnumType.h"


////////////////////////////////////////////////////////////////////
//     Function: TypeManager::resolve_type
//       Access: Public, Static
//  Description: A horrible hack around a CPPParser bug.  We don't
//               trust the CPPType pointer we were given; instead, we
//               ask CPPParser to parse a new type of the same name.
//               This has a better chance of fully resolving
//               templates.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
resolve_type(CPPType *type, CPPScope *scope) {
  if (scope == (CPPScope *)NULL) {
    scope = &parser;
  }

  type = type->resolve_type(scope, &parser);
  string name = type->get_local_name(&parser);
  if (name.empty()) {
    // Don't try to resolve unnamed types.
    return type;
  }

  CPPType *new_type = parser.parse_type(name);
  if (new_type == (CPPType *)NULL) {
    nout << "Type " << name << " is unknown to parser.\n";
  } else {
    type = new_type->resolve_type(&parser, &parser);
  }

  return type;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_assignable
//       Access: Public, Static
//  Description: Returns true if the indicated type is something we
//               can legitimately assign a value to, or false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_assignable(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
  case CPPDeclaration::ST_reference:
  case CPPDeclaration::ST_extension:
    return false;

  case CPPDeclaration::ST_struct:
    // In many cases, this is assignable, but there are some bizarre
    // cases where it is not.  Particularly in the event that the
    // programmer has defined a private copy assignment operator for
    // the class or struct.

    // We could try to figure out whether this has happened, but screw
    // it.  Concrete structure objects are not assignable, and so they
    // don't get setters synthesized for them.  If you want a setter,
    // write it yourself.

    // We'll make an exception for the string types, however, since
    // these are nearly an atomic type.
    if (is_basic_string_char(type) || is_basic_string_wchar(type)) {
      return true;
    }

    return false;

  default:
    return true;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_reference
//       Access: Public, Static
//  Description: Returns true if the indicated type is some kind of a
//               reference or const reference type to something
//               useful, false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_reference(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_reference(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return is_pointable(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_ref_to_anything
//       Access: Public, Static
//  Description: Returns true if the indicated type is some kind of a
//               reference or const reference type at all, false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_ref_to_anything(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_ref_to_anything(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return true;

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_ref_to_anything
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               reference to something, false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_ref_to_anything(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_const_ref_to_anything(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return is_const(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_pointer_to_anything
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               pointer to something, false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_pointer_to_anything(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_const_pointer_to_anything(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_pointer:
    return is_const(type->as_pointer_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_non_const_pointer_or_ref
//       Access: Public, Static
//  Description: Returns true if the indicated type is a non-const
//               pointer or reference to something, false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_non_const_pointer_or_ref(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_non_const_pointer_or_ref(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_pointer:
    return !is_const(type->as_pointer_type()->_pointing_at);

  case CPPDeclaration::ST_reference:
    return !is_const(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_pointer
//       Access: Public, Static
//  Description: Returns true if the indicated type is some kind of a
//               pointer or const pointer type, false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_pointer(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_pointer(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_pointer:
    return is_pointable(type->as_pointer_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const
//       Access: Public, Static
//  Description: Returns true if the indicated type is some kind of a
//               const type, false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return true;

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_struct
//       Access: Public, Static
//  Description: Returns true if the indicated type is a concrete
//               struct, class, or union type, or false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_struct(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_struct(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_struct:
  case CPPDeclaration::ST_extension:
    return true;

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_enum
//       Access: Public, Static
//  Description: Returns true if the indicated type is some kind of
//               enumerated type, const or otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_enum(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_enum:
    return true;

  case CPPDeclaration::ST_const:
    return is_enum(type->as_const_type()->_wrapped_around);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_enum
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               enumerated type.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_enum(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_enum(type->as_const_type()->_wrapped_around);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_ref_to_enum
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               reference to an enumerated type.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_ref_to_enum(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_reference:
    return is_const_enum(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_simple
//       Access: Public, Static
//  Description: Returns true if the indicated type is something that
//               a scripting language can handle directly as a
//               concrete, like an int or float, either const or
//               non-const.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_simple(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_simple:
  case CPPDeclaration::ST_enum:
    return true;

  case CPPDeclaration::ST_const:
    return is_simple(type->as_const_type()->_wrapped_around);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_simple
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const wrapper
//               around some simple type like int.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_simple(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_simple(type->as_const_type()->_wrapped_around);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_ref_to_simple
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               reference to something that a scripting language can
//               handle directly as a concrete.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_ref_to_simple(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_reference:
    return is_const_simple(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_ref_to_simple
//       Access: Public, Static
//  Description: Returns true if the indicated type is a non-const
//               reference to something that a scripting language can
//               handle directly as a concrete.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_ref_to_simple(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_reference:
    return is_simple(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_pointable
//       Access: Public, Static
//  Description: Returns true if the indicated type is something
//               ordinary that a scripting language can handle a
//               pointer to, e.g. a class or a structure, but not an
//               int or a function.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_pointable(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_pointable(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_extension:
    return (type->as_extension_type()->_type != CPPExtensionType::T_enum);

  case CPPDeclaration::ST_struct:
    return true;

  case CPPDeclaration::ST_simple:
    return is_char(type);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_char
//       Access: Public, Static
//  Description: Returns true if the indicated type is char or const
//               char, but not signed or unsigned char.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_char(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_char(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_simple:
    {
      CPPSimpleType *simple_type = type->as_simple_type();
      if (simple_type != (CPPSimpleType *)NULL) {
        return
          simple_type->_type == CPPSimpleType::T_char &&
          simple_type->_flags == 0;
      }
    }

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_char_pointer
//       Access: Public, Static
//  Description: Returns true if the indicated type is char * or const
//               char * or some such.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_char_pointer(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_char_pointer(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_pointer:
    return is_char(type->as_pointer_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_basic_string_char
//       Access: Public, Static
//  Description: Returns true if the type is basic_string<char>.  This
//               is the standard C++ string class.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_basic_string_char(CPPType *type) {
  CPPType *string_type = get_basic_string_char_type();
  if (string_type != (CPPType *)NULL &&
      string_type->get_local_name(&parser) == type->get_local_name(&parser)) {
    return true;
  }

  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_basic_string_char(type->as_const_type()->_wrapped_around);

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_basic_string_char
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const wrapper
//               around basic_string<char>.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_basic_string_char(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_basic_string_char(type->as_const_type()->_wrapped_around);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_ref_to_basic_string_char
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               reference to basic_string<char>.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_ref_to_basic_string_char(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_reference:
    return is_const_basic_string_char(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_ptr_to_basic_string_char
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               pointer to basic_string<char>.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_ptr_to_basic_string_char(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_pointer:
    return is_const_basic_string_char(type->as_pointer_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_string
//       Access: Public, Static
//  Description: Returns true if the type is basic_string<char>, or
//               a const reference to it.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_string(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_reference:
    return is_const_basic_string_char(type->as_reference_type()->_pointing_at);

  default:
    break;
  }

  return is_basic_string_wchar(type);
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_basic_string_wchar
//       Access: Public, Static
//  Description: Returns true if the type is basic_string<wchar_t>.  This
//               is the standard C++ wide string class.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_basic_string_wchar(CPPType *type) {
  CPPType *string_type = get_basic_string_wchar_type();
  if (string_type != (CPPType *)NULL &&
      string_type->get_local_name(&parser) == type->get_local_name(&parser)) {
    return true;
  }

  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_basic_string_wchar(type->as_const_type()->_wrapped_around);

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_basic_string_wchar
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const wrapper
//               around basic_string<wchar_t>.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_basic_string_wchar(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_basic_string_wchar(type->as_const_type()->_wrapped_around);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_ref_to_basic_string_wchar
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               reference to basic_string<wchar_t>.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_ref_to_basic_string_wchar(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_reference:
    return is_const_basic_string_wchar(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_ptr_to_basic_string_wchar
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               pointer to basic_string<wchar_t>.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_ptr_to_basic_string_wchar(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_pointer:
    return is_const_basic_string_wchar(type->as_pointer_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_wstring
//       Access: Public, Static
//  Description: Returns true if the type is basic_string<wchar_t>, or
//               a const reference to it.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_wstring(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_reference:
    return is_const_basic_string_wchar(type->as_reference_type()->_pointing_at);

  default:
    break;
  }

  return is_basic_string_wchar(type);
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_bool
//       Access: Public, Static
//  Description: Returns true if the indicated type is bool, or some
//               trivial variant.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_bool(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_bool(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_simple:
    {
      CPPSimpleType *simple_type = type->as_simple_type();
      if (simple_type != (CPPSimpleType *)NULL) {
        return
          simple_type->_type == CPPSimpleType::T_bool;
      }
    }
    break;

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_integer
//       Access: Public, Static
//  Description: Returns true if the indicated type is one of the
//               basic integer types: bool, char, short, int, or long,
//               signed or unsigned, as well as enumerated types.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_integer(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_integer(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_enum:
    return true;

  case CPPDeclaration::ST_simple:
    {
      CPPSimpleType *simple_type = type->as_simple_type();
      if (simple_type != (CPPSimpleType *)NULL) {
        return
          (simple_type->_type == CPPSimpleType::T_bool ||
           simple_type->_type == CPPSimpleType::T_char ||
           simple_type->_type == CPPSimpleType::T_wchar_t ||
           simple_type->_type == CPPSimpleType::T_int);
      }
    }
    break;

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_unsigned_integer
//       Access: Public, Static
//  Description: Returns true if the indicated type is one of the
//               basic integer types, but only the unsigned varieties.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_unsigned_integer(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_unsigned_integer(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_simple:
    {
      CPPSimpleType *simple_type = type->as_simple_type();
      if (simple_type != (CPPSimpleType *)NULL) {
        return
          ((simple_type->_type == CPPSimpleType::T_bool ||
            simple_type->_type == CPPSimpleType::T_char ||
            simple_type->_type == CPPSimpleType::T_wchar_t ||
            simple_type->_type == CPPSimpleType::T_int) &&
           (simple_type->_flags & CPPSimpleType::F_unsigned) != 0);
      }
    }
    break;

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_unsigned_longlong
//       Access: Public, Static
//  Description: Returns true if the indicated type is an unsigned
//               "long long" type or larger, or at least a 64-bit
//               unsigned integer.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_unsigned_longlong(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_unsigned_longlong(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_simple:
    {
      CPPSimpleType *simple_type = type->as_simple_type();
      if (simple_type != (CPPSimpleType *)NULL) {
        return (simple_type->_type == CPPSimpleType::T_int && 
                (simple_type->_flags & (CPPSimpleType::F_longlong | CPPSimpleType::F_unsigned)) == (CPPSimpleType::F_longlong | CPPSimpleType::F_unsigned));
      }
    }
    break;

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_longlong
//       Access: Public, Static
//  Description: Returns true if the indicated type is the "long long"
//               type or larger, or at least a 64-bit integer, whether
//               signed or unsigned.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_longlong(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_longlong(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_simple:
    {
      CPPSimpleType *simple_type = type->as_simple_type();
      if (simple_type != (CPPSimpleType *)NULL) {
        return (simple_type->_type == CPPSimpleType::T_int && 
                (simple_type->_flags & CPPSimpleType::F_longlong) != 0);
      }
    }
    break;

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_float
//       Access: Public, Static
//  Description: Returns true if the indicated type is one of the
//               basic floating-point types: float, double, or some
//               similar variant.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_float(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_float(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_simple:
    {
      CPPSimpleType *simple_type = type->as_simple_type();
      if (simple_type != (CPPSimpleType *)NULL) {
        return
          (simple_type->_type == CPPSimpleType::T_float ||
           simple_type->_type == CPPSimpleType::T_double);
      }
    }
    break;

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_void
//       Access: Public, Static
//  Description: Returns true if the indicated type is void.  (Not
//               void *, just void.)
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_void(CPPType *type) {
  CPPSimpleType *simple_type = type->as_simple_type();
  if (simple_type != (CPPSimpleType *)NULL) {
    return
      simple_type->_type == CPPSimpleType::T_void &&
      simple_type->_flags == 0;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_reference_count
//       Access: Public, Static
//  Description: Returns true if the indicated type is some class that
//               derives from ReferenceCount, or false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_reference_count(CPPType *type) {
  CPPType *refcount_type = get_reference_count_type();
  if (refcount_type != (CPPType *)NULL &&
      refcount_type->get_local_name(&parser) == type->get_local_name(&parser)) {
    return true;
  }

  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_reference_count(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_struct:
    {
      CPPStructType *stype = type->as_struct_type();
      CPPStructType::Derivation::const_iterator di;
      for (di = stype->_derivation.begin();
           di != stype->_derivation.end();
           ++di) {
        if (is_reference_count((*di)._base)) {
          return true;
        }
      }
    }
    break;

  default:
    break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_reference_count_pointer
//       Access: Public, Static
//  Description: Returns true if the indicated type is a pointer to a
//               class that derives from ReferenceCount.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_reference_count_pointer(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_reference_count_pointer(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_pointer:
    return is_reference_count(type->as_pointer_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_pointer_to_base
//       Access: Public, Static
//  Description: Returns true if the indicated type is some class that
//               derives from PointerToBase, or false otherwise.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_pointer_to_base(CPPType *type) {
  // We only check the simple name of the type against PointerToBase,
  // since we need to allow for the various template instantiations of
  // this thing.

  // We also check explicitly for "PointerTo" and "ConstPointerTo",
  // instead of actually checking for PointerToBase, because we don't
  // want to consider PointerToArray in this category.
  if (type->get_simple_name() == "PointerTo" ||
      type->get_simple_name() == "ConstPointerTo") {
    return true;
  }

  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_pointer_to_base(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_struct:
    {
      CPPStructType *stype = type->as_struct_type();
      CPPStructType::Derivation::const_iterator di;
      for (di = stype->_derivation.begin();
           di != stype->_derivation.end();
           ++di) {
        if (is_pointer_to_base((*di)._base)) {
          return true;
        }
      }
    }

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_pointer_to_base
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               PointerToBase or some derivative.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_pointer_to_base(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_pointer_to_base(type->as_const_type()->_wrapped_around);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_const_ref_to_pointer_to_base
//       Access: Public, Static
//  Description: Returns true if the indicated type is a const
//               reference to a class that derives from PointerToBase.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_const_ref_to_pointer_to_base(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_reference(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return is_const_pointer_to_base(type->as_reference_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_pointer_to_PyObject
//       Access: Public, Static
//  Description: Returns true if the indicated type is PyObject *.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_pointer_to_PyObject(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_pointer_to_PyObject(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_pointer:
    return is_PyObject(type->as_pointer_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_PyObject
//       Access: Public, Static
//  Description: Returns true if the indicated type is PyObject.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_PyObject(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_PyObject(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_extension:
    return (type->get_local_name(&parser) == "PyObject");

  default:
    return false;
  }
}


////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_ostream
//       Access: Public, Static
//  Description: Returns true if the indicated type is PyObject.
////////////////////////////////////////////////////////////////////
bool TypeManager::is_ostream(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_ostream(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_struct:
    return (type->get_local_name(&parser) == "ostream");

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::is_pointer_to_PyObject
//       Access: Public, Static
//  Description: Returns true if the indicated type is PyObject *.
////////////////////////////////////////////////////////////////////
bool TypeManager::
is_pointer_to_ostream(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return is_pointer_to_ostream(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return is_ostream(type->as_reference_type()->_pointing_at);

  case CPPDeclaration::ST_pointer:
    return is_ostream(type->as_pointer_type()->_pointing_at);

  default:
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::involves_unpublished
//       Access: Public, Static
//  Description: Returns true if the type is an unpublished type,
//               e.g. a protected or private nested class, or simply a
//               type not marked as 'published', or if the type is a
//               pointer or reference to such an unpublished type, or
//               even if the type is a function type that includes a
//               parameter of such an unpublished type.
////////////////////////////////////////////////////////////////////
bool TypeManager::
involves_unpublished(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return involves_unpublished(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return involves_unpublished(type->as_reference_type()->_pointing_at);

  case CPPDeclaration::ST_pointer:
    return involves_unpublished(type->as_pointer_type()->_pointing_at);

  case CPPDeclaration::ST_struct:
    // A struct type is unpublished only if all of its members are
    // unpublished.
    if (type->_declaration != (CPPTypeDeclaration *)NULL) {
      if (type->_declaration->_vis <= min_vis) {
        return false;
      }
    }
    {
      CPPScope *scope = type->as_struct_type()->_scope;

      bool any_exported = false;
      CPPScope::Declarations::const_iterator di;
      for (di = scope->_declarations.begin();
           di != scope->_declarations.end() && !any_exported;
           ++di) {
        if ((*di)->_vis <= min_vis) {
          any_exported = true;
        }
      }

      return !any_exported;
    }

  case CPPDeclaration::ST_function:
    if (type->_declaration != (CPPTypeDeclaration *)NULL) {
      if (type->_declaration->_vis <= min_vis) {
        return false;
      }
    }
    return true;
    /*
    {
      CPPFunctionType *ftype = type->as_function_type();
      if (involves_unpublished(ftype->_return_type)) {
        return true;
      }
      const CPPParameterList::Parameters &params =
        ftype->_parameters->_parameters;
      CPPParameterList::Parameters::const_iterator pi;
      for (pi = params.begin(); pi != params.end(); ++pi) {
        if (involves_unpublished((*pi)->_type)) {
          return true;
        }
      }
      return false;
    }
    */

  default:
    if (type->_declaration != (CPPTypeDeclaration *)NULL) {
      return (type->_declaration->_vis > min_vis);
    }
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::involves_protected
//       Access: Public, Static
//  Description: Returns true if the type is an protected type,
//               e.g. a protected or private nested class, or if the
//               type is a pointer or reference to such a protected
//               type, or even if the type is a function type that
//               includes a parameter of such a protected type.
////////////////////////////////////////////////////////////////////
bool TypeManager::
involves_protected(CPPType *type) {
  switch (type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return involves_protected(type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return involves_protected(type->as_reference_type()->_pointing_at);

  case CPPDeclaration::ST_pointer:
    return involves_protected(type->as_pointer_type()->_pointing_at);

  case CPPDeclaration::ST_function:
    {
      CPPFunctionType *ftype = type->as_function_type();
      if (involves_protected(ftype->_return_type)) {
        return true;
      }
      const CPPParameterList::Parameters &params =
        ftype->_parameters->_parameters;
      CPPParameterList::Parameters::const_iterator pi;
      for (pi = params.begin(); pi != params.end(); ++pi) {
        if (involves_protected((*pi)->_type)) {
          return true;
        }
      }
      return false;
    }

  default:
    if (type->_declaration != (CPPTypeDeclaration *)NULL) {
      return (type->_declaration->_vis > V_public);
    }
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::unwrap_pointer
//       Access: Public, Static
//  Description: Returns the type this pointer type points to.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
unwrap_pointer(CPPType *source_type) {
  switch (source_type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return unwrap_pointer(source_type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_pointer:
    return source_type->as_pointer_type()->_pointing_at;

  default:
    return source_type;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::unwrap_reference
//       Access: Public, Static
//  Description: Returns the type this reference type points to.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
unwrap_reference(CPPType *source_type) {
  switch (source_type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return unwrap_reference(source_type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return source_type->as_reference_type()->_pointing_at;

  default:
    return source_type;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::unwrap_const
//       Access: Public, Static
//  Description: Removes the const declaration from the outside of the
//               type.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
unwrap_const(CPPType *source_type) {
  switch (source_type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return unwrap_const(source_type->as_const_type()->_wrapped_around);

  default:
    return source_type;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::unwrap_const_reference
//       Access: Public, Static
//  Description: Removes a reference or a const reference from the
//               type.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
unwrap_const_reference(CPPType *source_type) {
  switch (source_type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return unwrap_const_reference(source_type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return unwrap_const(source_type->as_reference_type()->_pointing_at);

  default:
    return source_type;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::unwrap
//       Access: Public, Static
//  Description: Removes all const, pointer, and reference wrappers,
//               to get to the thing we're talking about.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
unwrap(CPPType *source_type) {
  switch (source_type->get_subtype()) {
  case CPPDeclaration::ST_const:
    return unwrap(source_type->as_const_type()->_wrapped_around);

  case CPPDeclaration::ST_reference:
    return unwrap(source_type->as_reference_type()->_pointing_at);

  case CPPDeclaration::ST_pointer:
    return unwrap(source_type->as_pointer_type()->_pointing_at);

  default:
    return source_type;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::get_pointer_type
//       Access: Public, Static
//  Description: Returns the type of pointer the given PointerTo class
//               emulates.  Essentially this just checks the return
//               type of the method called 'p()'.  Returns NULL if the
//               PointerTo class has no method p().
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
get_pointer_type(CPPStructType *pt_type) {
  CPPScope *scope = pt_type->_scope;

  CPPScope::Functions::const_iterator fi;
  fi = scope->_functions.find("p");
  if (fi != scope->_functions.end()) {
    CPPFunctionGroup *fgroup = (*fi).second;

    // These are all the functions named "p".  Now look for one that
    // takes no parameters.
    CPPFunctionGroup::Instances::iterator ii;
    for (ii = fgroup->_instances.begin();
         ii != fgroup->_instances.end();
         ++ii) {
      CPPInstance *function = (*ii);
      CPPFunctionType *ftype = function->_type->as_function_type();
      assert(ftype != (CPPFunctionType *)NULL);
      if (ftype->_parameters->_parameters.empty()) {
        // Here's the function p().  What's its return type?
        return resolve_type(ftype->_return_type);
      }
    }
  }

  return (CPPType *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::wrap_pointer
//       Access: Public, Static
//  Description: Returns the type corresponding to a pointer to the
//               given type.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
wrap_pointer(CPPType *source_type) {
  return CPPType::new_type(new CPPPointerType(source_type));
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::wrap_const_pointer
//       Access: Public, Static
//  Description: Returns the type corresponding to a const pointer
//               to the given type.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
wrap_const_pointer(CPPType *source_type) {
  if (source_type->as_const_type() != (CPPConstType *)NULL) {
    // It's already const.
    return
      CPPType::new_type(new CPPPointerType(source_type));
  } else {
    return
      CPPType::new_type(new CPPPointerType(new CPPConstType(source_type)));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::wrap_const_reference
//       Access: Public, Static
//  Description: Returns the type corresponding to a const reference
//               to the given type.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
wrap_const_reference(CPPType *source_type) {
  if (source_type->as_const_type() != (CPPConstType *)NULL) {
    // It's already const.
    return
      CPPType::new_type(new CPPReferenceType(source_type));
  } else {
    return
      CPPType::new_type(new CPPReferenceType(new CPPConstType(source_type)));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::get_basic_string_char_type
//       Access: Public, Static
//  Description: Returns a CPPType that represents basic_string<char>,
//               or NULL if the type is unknown.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
get_basic_string_char_type() {
  static bool got_type = false;
  static CPPType *type = (CPPType *)NULL;
  if (!got_type) {
    type = parser.parse_type("basic_string<char>");
    got_type = true;
  }
  return type;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::get_basic_string_wchar_type
//       Access: Public, Static
//  Description: Returns a CPPType that represents basic_string<wchar_t>,
//               or NULL if the type is unknown.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
get_basic_string_wchar_type() {
  static bool got_type = false;
  static CPPType *type = (CPPType *)NULL;
  if (!got_type) {
    type = parser.parse_type("basic_string<wchar_t>");
    got_type = true;
  }
  return type;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::get_reference_count_type
//       Access: Public, Static
//  Description: Returns a CPPType that represents ReferenceCount,
//               or NULL if the type is unknown.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
get_reference_count_type() {
  static bool got_type = false;
  static CPPType *type = (CPPType *)NULL;
  if (!got_type) {
    type = parser.parse_type("ReferenceCount");
    got_type = true;
  }
  return type;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::get_void_type
//       Access: Public, Static
//  Description: Returns a CPPType that represents void.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
get_void_type() {
  static bool got_type = false;
  static CPPType *type = (CPPType *)NULL;
  if (!got_type) {
    type = CPPType::new_type(new CPPSimpleType(CPPSimpleType::T_void));
    got_type = true;
  }
  return type;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::get_int_type
//       Access: Public, Static
//  Description: Returns a CPPType that represents int.
////////////////////////////////////////////////////////////////////
CPPType *TypeManager::
get_int_type() {
  static bool got_type = false;
  static CPPType *type = (CPPType *)NULL;
  if (!got_type) {
    type = CPPType::new_type(new CPPSimpleType(CPPSimpleType::T_int));
    got_type = true;
  }
  return type;
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::get_function_signature
//       Access: Public, Static
//  Description: Returns a string corresponding to the given function
//               signature.  This is a unique string per each
//               uniquely-callable C++ function or method.  Basically
//               it's the function prototype, sans the return type.
//
//               If num_default_parameters is nonzero, it is the
//               number of parameters to omit from the end of the
//               parameter list.  This in effect gets the function
//               signature for an equivalent function with n
//               parameters assuming default values.
////////////////////////////////////////////////////////////////////
string TypeManager::
get_function_signature(CPPInstance *function,
                       int num_default_parameters) {
  CPPFunctionType *ftype = function->_type->as_function_type();
  assert(ftype != (CPPFunctionType *)NULL);

  ostringstream out;

  // It's tempting to mark static methods with a different function
  // signature than non-static, because a static method doesn't have
  // an implicit 'this' parameter.  However, this breaks the lookup
  // when we come across a method definition outside of the class
  // body; since there's no clue at this point whether the method is
  // static or not, we can't successfully look it up.  Bummer.
  /*
    if ((function->_storage_class & CPPInstance::SC_static) != 0) {
    out << "static ";
    }
  */

  out << function->get_local_name(&parser) << "(";

  const CPPParameterList::Parameters &params =
    ftype->_parameters->_parameters;
  CPPParameterList::Parameters::const_iterator pi;

  int num_params = params.size() - num_default_parameters;
  pi = params.begin();
  for (int n = 0; n < num_params; n++) {
    assert(pi != params.end());
    CPPType *ptype = (*pi)->_type;

    // One exception: if the type is a const reference to something,
    // we build the signature with its corresponding concrete.  C++
    // can't differentiate these two anyway.
    if (is_const_ref_to_anything(ptype)) {
      ptype = unwrap_const_reference(ptype);
    }

    out << ptype->get_local_name(&parser);

    if (n + 1 < num_params) {
      out << ", ";
    }

    ++pi;
  }
  out << ")";

  if (ftype->_flags & CPPFunctionType::F_const_method) {
    out << " const";
  }

  return out.str();
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::get_function_name
//       Access: Public, Static
//  Description: Returns a string corresponding to the given function
//               name.  This is not necessarily unique to the
//               particular overloaded function instance, but is
//               common among all overloaded functions of the same
//               name.
////////////////////////////////////////////////////////////////////
string TypeManager::
get_function_name(CPPInstance *function) {
  return function->get_local_name(&parser);
}

////////////////////////////////////////////////////////////////////
//     Function: TypeManager::has_protected_destructor
//       Access: Public, Static
//  Description: Returns true if the destructor for the given class or
//               struct is protected or private, or false if the
//               destructor is public or absent.
////////////////////////////////////////////////////////////////////
bool TypeManager::
has_protected_destructor(CPPType *type) {
  CPPStructType *struct_type = type->as_struct_type();
  if (struct_type == (CPPStructType *)NULL) {
    // It's not even a struct type!
    return false;
  }

  CPPScope *scope = struct_type->get_scope();

  // Look for the destructor.
  CPPScope::Declarations::const_iterator di;
  for (di = scope->_declarations.begin();
       di != scope->_declarations.end();
       ++di) {
    if ((*di)->get_subtype() == CPPDeclaration::ST_instance) {
      CPPInstance *inst = (*di)->as_instance();
      if (inst->_type->get_subtype() == CPPDeclaration::ST_function) {
        // Here's a function declaration.
        CPPFunctionType *ftype = inst->_type->as_function_type();
        assert(ftype != (CPPFunctionType *)NULL);
        if ((ftype->_flags & CPPFunctionType::F_destructor) != 0) {
          // Here's the destructor!  Is it protected?
          return (inst->_vis > V_public);
        }
      }
    }
  }

  // No explicit destructor.
  return false;
}
////////////////////////////////////////////////////////////////////
//     Function: TypeManager::has_protected_destructor
//       Access: Public, Static
//  Description: Returns true if the destructor for the given class or
//               struct is protected or private, or false if the
//               destructor is public or absent.
////////////////////////////////////////////////////////////////////
bool TypeManager::IsExported(CPPType *in_type)
{

  string name = in_type->get_local_name(&parser);
  if (name.empty()) {
      return false;
  }

    //return true;

    // this question is about the base type 
    CPPType *base_type = resolve_type(unwrap(in_type));
    //CPPType *base_type = in_type;
    // Ok export Rules..
    //    Classes and Structs and Unions are exported only if they have a 
    //    function that is exported..      
    // function is the easiest case.  

    if (base_type->_vis <= min_vis)
        return true;

    if (in_type->_vis <= min_vis)
        return true;


    if (base_type->get_subtype() == CPPDeclaration::ST_struct) 
    {
            CPPStructType *sstruct_type = base_type->as_struct_type();
            CPPStructType *struct_type =sstruct_type->resolve_type(&parser, &parser)->as_struct_type();
            CPPScope *scope = struct_type->_scope;

            CPPScope::Declarations::const_iterator di;
            for (di = scope->_declarations.begin();di != scope->_declarations.end(); di++)
            {
                if ((*di)->_vis <= min_vis) 
                    return true;
            }

            

    }
    else if (base_type->get_subtype() == CPPDeclaration::ST_instance) 
    {
        CPPInstance *inst = base_type->as_instance();
        if (inst->_type->get_subtype() == CPPDeclaration::ST_function) 
        {
            CPPInstance *function = inst;
            CPPFunctionType *ftype = function->_type->resolve_type(&parser, &parser)->as_function_type();
            if (ftype->_vis <= min_vis)
                return true;
        }
        else 
        {
            if (inst->_vis <= min_vis)
                return true;
        }

    }
    else if (base_type->get_subtype() == CPPDeclaration::ST_typedef) 
    {
        CPPTypedef *tdef = base_type->as_typedef();
        if (tdef->_type->get_subtype() == CPPDeclaration::ST_struct) 
        {
            CPPStructType *struct_type =tdef->_type->resolve_type(&parser, &parser)->as_struct_type();
            return IsExported(struct_type);
        }

    }
    else if (base_type->get_subtype() == CPPDeclaration::ST_type_declaration) 
    {
        CPPType *type = base_type->as_type_declaration()->_type;
        if (type->get_subtype() == CPPDeclaration::ST_struct)
        {
            CPPStructType *struct_type =type->as_type()->resolve_type(&parser, &parser)->as_struct_type();
            //CPPScope *scope = struct_type->_scope;
            return IsExported(struct_type);

        }
        else if (type->get_subtype() == CPPDeclaration::ST_enum) 
        {
          //CPPEnumType *enum_type = type->as_type()->resolve_type(&parser, &parser)->as_enum_type();
            if (type->_vis <= min_vis)
                return true;
        }
    }

/*
    printf("---------------------> Visibility Failed  %s %d Vis=%d, Minvis=%d\n",
        base_type->get_fully_scoped_name().c_str(),
        base_type->get_subtype(),
        base_type->_vis,
        min_vis);
*/
    return false;       
};

bool TypeManager::IsLocal(CPPType *in_type)
{
    // A local means it was compiled in this scope of work..
    // IE a should actualy generate code for this objects....
   CPPType *base_type = resolve_type(unwrap(in_type));
   if (base_type->_forcetype) {
     return true;
   }

   if (base_type->_file._source == CPPFile::S_local && !base_type->is_incomplete()) {
     return true;
   }

   return false;

    /*

    if (base_type->get_subtype() == CPPDeclaration::ST_struct) 
    {
            CPPStructType *struct_type = base_type->as_struct_type();
            if (struct_type->_file._source == CPPFile::S_local)
                return  true;

    }
    else if (base_type->get_subtype() == CPPDeclaration::ST_instance) 
    {
        CPPInstance *inst = base_type->as_instance();
        if (inst->_type->get_subtype() == CPPDeclaration::ST_function) 
        {
            CPPInstance *function = inst;
            CPPFunctionType *ftype = function->_type->resolve_type(&parser, &parser)->as_function_type();
            if (ftype->_file._source == CPPFile::S_local)
                return true;
        }
        else 
        {
            if (inst->_file._source == CPPFile::S_local)
                return true;
        }
    }
    else if (base_type->get_subtype() == CPPDeclaration::ST_typedef) 
    {
        CPPTypedef *tdef = base_type->as_typedef();
        if (tdef->_type->get_subtype() == CPPDeclaration::ST_struct) 
        {
            CPPStructType *struct_type =tdef->_type->resolve_type(&parser, &parser)->as_struct_type();
            return IsLocal(struct_type);


        }
    }
    else if (base_type->get_subtype() == CPPDeclaration::ST_type_declaration) 
    {
        CPPType *type = base_type->as_type_declaration()->_type;
        if (type->get_subtype() == CPPDeclaration::ST_struct)
        {
            CPPStructType *struct_type =type->as_type()->resolve_type(&parser, &parser)->as_struct_type();
            if (struct_type->_file._source == CPPFile::S_local)
                return true;

        }
        else if (type->get_subtype() == CPPDeclaration::ST_enum) 
        {
            CPPEnumType *enum_type = type->as_type()->resolve_type(&parser, &parser)->as_enum_type();
            if (enum_type->_file._source != CPPFile::S_local)
                return true;
        }
    }

  if (base_type->_file._source == CPPFile::S_local)
      return true; 

 return false;
 */
};
