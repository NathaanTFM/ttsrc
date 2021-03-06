// Filename: parse_file.cxx
// Created by:  drose (20Oct99)
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

#include "cppParser.h"
#include "cppManifest.h"
#include "cppStructType.h"
#include "cppFunctionGroup.h"
#include "cppTypedef.h"
#include "cppExpressionParser.h"
#include "cppExpression.h"
#include "cppType.h"
#include "cppGlobals.h"

#include <stdlib.h>

#ifndef HAVE_GETOPT_LONG_ONLY
  #include "gnu_getopt.h"
#else
  #ifdef PHAVE_GETOPT_H
    #include <getopt.h>
  #endif
#endif

CPPParser parser;

void
predefine_macro(CPPParser &parser, const string &option) {
  string macro_name, macro_def;

  size_t eq = option.find('=');
  if (eq != string::npos) {
    macro_name = option.substr(0, eq);
    macro_def = option.substr(eq + 1);
  } else {
    macro_name = option;
  }

  cerr << "Predefining " << macro_name << " as " << macro_def << "\n";

  CPPManifest *macro = new CPPManifest(macro_name + " " + macro_def);
  parser._manifests[macro->_name] = macro;
}

void
show_type_or_expression(const string &str) {
  CPPExpression *expr = parser.parse_expr(str);
  if (expr != NULL) {
    cout << "\nExpression: " << *expr << "\n";
    CPPType *type = expr->determine_type();
    if (type == NULL) {
      cout << "type is unknown\n";
    } else {
      cout << "type is " << *type << "\n";
    }
    cout << "value is " << expr->evaluate() << "\n\n";

  } else {
    CPPType *type = parser.parse_type(str);
    if (type != NULL) {
      cout << "\nType: " << *type << "\n"
           << "Defined in: " << type->_file << "\n"
           << "Subtype code is: " << (int)type->get_subtype() << "\n\n";

      CPPStructType *stype = type->as_struct_type();
      if (stype != (CPPStructType *)NULL) {
        stype->check_virtual();
      }

      type->output(cout, 0, &parser, true);
      cout << "\n\n"
           << "is_template = " << type->is_template() << "\n"
           << "is_fully_specified = " << type->is_fully_specified() << "\n"
           << "is_tbd = " << type->is_tbd() << "\n";
      if (type->has_typedef_name()) {
        cout << "get_typedef_name = " << type->get_typedef_name() << "\n";
      }
      cout << "get_simple_name = " << type->get_simple_name() << "\n"
           << "get_local_name = " << type->get_local_name() << "\n"
           << "get_fully_scoped_name = " << type->get_fully_scoped_name() << "\n"
           << "get_preferred_name = " << type->get_preferred_name() << "\n"
           << "is_incomplete = " << type->is_incomplete() << "\n";

      if (stype != (CPPStructType *)NULL) {
        cout << "scope = " << stype->get_scope()->get_fully_scoped_name() << "\n";
        bool is_abstract = stype->is_abstract();
        cout << "is_abstract = " << is_abstract << "\n";
        if (is_abstract) {
          cout << "pure virtual functions:\n";
          CPPStructType::VFunctions vf;
          stype->get_pure_virtual_funcs(vf);
          CPPStructType::VFunctions::const_iterator fi;
          for (fi = vf.begin(); fi != vf.end(); ++fi) {
            cout << "  " << *(*fi) << "\n";
          }
        }
      }

      cout << "\n";
    } else {
      cout << "Invalid expression or type.\n";
    }
  }
}

void
show_methods(const string &str) {
  CPPType *type = parser.parse_type(str);
  if (type == NULL) {
    cerr << "Invalid type: " << str << "\n";
    return;
  }

  CPPStructType *stype = type->as_struct_type();
  if (stype == NULL) {
    cerr << "Type is not a structure or class.\n";
    return;
  }

  CPPScope *scope = stype->get_scope();
  assert(scope != (CPPScope *)NULL);

  cerr << "Methods in " << *stype << ":\n";

  CPPScope::Functions::const_iterator fi;
  for (fi = scope->_functions.begin(); fi != scope->_functions.end(); ++fi) {
    CPPFunctionGroup *fgroup = (*fi).second;

    CPPFunctionGroup::Instances::const_iterator ii;
    for (ii = fgroup->_instances.begin();
         ii != fgroup->_instances.end();
         ++ii) {
      CPPInstance *inst = (*ii);
      cerr << "  " << *inst << "\n";
    }
  }
}

void
show_data_members(const string &str) {
  CPPType *type = parser.parse_type(str);
  if (type == NULL) {
    cerr << "Invalid type: " << str << "\n";
    return;
  }

  CPPStructType *stype = type->as_struct_type();
  if (stype == NULL) {
    cerr << "Type is not a structure or class.\n";
    return;
  }

  CPPScope *scope = stype->get_scope();
  assert(scope != (CPPScope *)NULL);

  cerr << "Data members in " << *stype << ":\n";

  CPPScope::Variables::const_iterator vi;
  for (vi = scope->_variables.begin(); vi != scope->_variables.end(); ++vi) {
    CPPInstance *inst = (*vi).second;
    cerr << "  " << *inst << "\n";
  }
}

void
show_typedefs(const string &str) {
  CPPType *type = parser.parse_type(str);
  if (type == NULL) {
    cerr << "Invalid type: " << str << "\n";
    return;
  }

  CPPStructType *stype = type->as_struct_type();
  if (stype == NULL) {
    cerr << "Type is not a structure or class.\n";
    return;
  }

  CPPScope *scope = stype->get_scope();
  assert(scope != (CPPScope *)NULL);

  cerr << "Typedefs in " << *stype << ":\n";

  CPPScope::Typedefs::const_iterator ti;
  for (ti = scope->_typedefs.begin(); ti != scope->_typedefs.end(); ++ti) {
    CPPTypedef *td = (*ti).second;
    cerr << "  " << *td << "\n";
  }
}


int
main(int argc, char *argv[]) {
  extern char *optarg;
  extern int optind;
  const char *optstr = "I:S:D:o:l:vp";

  parser.set_verbose(2);
  bool prompt = false;

  int flag = getopt(argc, argv, optstr);

  while (flag != EOF) {
    switch (flag) {
    case 'I':
      parser._quote_include_path.append_directory(optarg);
      parser._quote_include_kind.push_back(CPPFile::S_alternate);
      break;

    case 'S':
      parser._angle_include_path.append_directory(optarg);
      parser._quote_include_path.append_directory(optarg);
      parser._quote_include_kind.push_back(CPPFile::S_system);
      break;

    case 'D':
      predefine_macro(parser, optarg);
      break;

    case 'o':
      cerr << "Ignoring output file " << optarg << "\n";
      break;

    case 'l':
      cpp_longlong_keyword = optarg;
      break;

    case 'v':
      parser.set_verbose(parser.get_verbose() + 1);
      break;

    case 'p':
      prompt = true;
      break;

    default:
      exit(1);
    }
    flag = getopt(argc, argv, optstr);
  }

  argc -= (optind-1);
  argv += (optind-1);


  if (argc < 2) {
    cerr << "parse-file [opts] file1.h [file2.h ... ]\n"
         << "\nOptions:\n\n"
         << "  -I include_path\n"
         << "  -S system_include_path\n"
         << "  -D manifest_name\n"
         << "  -D manifest_name=manifest_definition\n"
         << "  -o output_file (ignored)\n"
         << "  -v             (increase verbosity)\n"
         << "  -p             (prompt for expression instead of dumping output)\n";

    exit(1);
  }

  for (int i = 1; i < argc; i++) {
    if (!parser.parse_file(argv[i])) {
      cerr << "Error in parsing.\n";
      exit(1);
    }
  }

  cerr << "Finished parsing.\n";

  if (prompt) {
    while (cin) {
      string str;
      cout << "Enter an expression or type name:\n";
      getline(cin, str);
      if (!str.empty()) {

        size_t space = str.find(' ');
        if (space != string::npos) {
          string first_word = str.substr(0, space);
          string remainder = str.substr(space + 1);

          if (first_word == "methods") {
            show_methods(remainder);
          } else if (first_word == "members") {
            show_data_members(remainder);
          } else if (first_word == "typedefs") {
            show_typedefs(remainder);
          } else {
            show_type_or_expression(str);
          }
        } else {
          show_type_or_expression(str);
        }
      }
    }
  } else {
    parser.write(cout, 0, &parser);
    cout << "\n";
  }

  /*
  cout << "Opened the following files:\n";
  CPPParser::ParsedFiles::const_iterator fi;
  for (fi = parser._parsed_files.begin();
       fi != parser._parsed_files.end();
       ++fi) {
    cout << "   ";

    switch ((*fi)._source) {
    case CPPFile::S_local:
      cout << "L";
      break;
    case CPPFile::S_alternate:
      cout << "A";
      break;
    case CPPFile::S_system:
      cout << "S";
      break;
    default:
      cout << " ";
    }

    cout << " " << (*fi) << "\n";
  }
  */

  /*
  CPPParser::Comments::const_iterator ci;
  for (ci = parser._comments.begin(); ci != parser._comments.end(); ++ci) {
    const CPPParser::CommentBlock &c = (*ci);
    cout << "Comment in file " << c._file << " at line " << c._line_number
         << ":\n" << c._comment << "\n\n";
  }
  */

  /*
  CPPParser::Manifests::const_iterator mi;
  for (mi = parser._manifests.begin(); mi != parser._manifests.end(); ++mi) {
    const CPPManifest *m = (*mi).second;
    cout << "Manifest " << m->_name << " defined in " << m->_file;
    CPPType *type = m->determine_type();
    if (type == (CPPType *)NULL) {
      cout << " (no type)\n";
    } else {
      cout << " has type " << *type << "\n";
    }
  }
  */

  return (0);
}



