// Filename: pystub.cxx
// Created by:  drose (09Aug00)
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

#include "pystub.h"

extern "C" {
  EXPCL_DTOOLCONFIG int PyArg_Parse(...);
  EXPCL_DTOOLCONFIG int PyArg_ParseTuple(...);
  EXPCL_DTOOLCONFIG int PyArg_ParseTupleAndKeywords(...);
  EXPCL_DTOOLCONFIG int PyCFunction_New(...);
  EXPCL_DTOOLCONFIG int PyCFunction_NewEx(...);
  EXPCL_DTOOLCONFIG int PyCallable_Check(...);
  EXPCL_DTOOLCONFIG int PyDict_DelItemString(...);
  EXPCL_DTOOLCONFIG int PyDict_GetItem(...);
  EXPCL_DTOOLCONFIG int PyDict_GetItemString(...);
  EXPCL_DTOOLCONFIG int PyDict_New(...);
  EXPCL_DTOOLCONFIG int PyDict_SetItem(...);
  EXPCL_DTOOLCONFIG int PyDict_SetItemString(...);
  EXPCL_DTOOLCONFIG int PyDict_Size(...);
  EXPCL_DTOOLCONFIG int PyDict_Type(...);
  EXPCL_DTOOLCONFIG int PyErr_Clear(...);
  EXPCL_DTOOLCONFIG int PyErr_ExceptionMatches(...);
  EXPCL_DTOOLCONFIG int PyErr_Fetch(...);
  EXPCL_DTOOLCONFIG int PyErr_Format(...);
  EXPCL_DTOOLCONFIG int PyErr_Occurred(...);
  EXPCL_DTOOLCONFIG int PyErr_Print(...);
  EXPCL_DTOOLCONFIG int PyErr_Restore(...);
  EXPCL_DTOOLCONFIG int PyErr_SetString(...);
  EXPCL_DTOOLCONFIG int PyErr_Warn(...);
  EXPCL_DTOOLCONFIG int PyErr_WarnEx(...);
  EXPCL_DTOOLCONFIG int PyEval_CallFunction(...);
  EXPCL_DTOOLCONFIG int PyEval_CallObjectWithKeywords(...);
  EXPCL_DTOOLCONFIG int PyEval_InitThreads(...);
  EXPCL_DTOOLCONFIG int PyEval_RestoreThread(...);
  EXPCL_DTOOLCONFIG int PyEval_SaveThread(...);
  EXPCL_DTOOLCONFIG int PyFloat_AsDouble(...);
  EXPCL_DTOOLCONFIG int PyFloat_FromDouble(...);
  EXPCL_DTOOLCONFIG int PyFloat_Type(...);
  EXPCL_DTOOLCONFIG int PyGen_Check(...);
  EXPCL_DTOOLCONFIG int PyGen_Type(...);
  EXPCL_DTOOLCONFIG int PyGILState_Ensure(...);
  EXPCL_DTOOLCONFIG int PyGILState_Release(...);
  EXPCL_DTOOLCONFIG int PyImport_GetModuleDict(...);
  EXPCL_DTOOLCONFIG int PyInt_AsLong(...);
  EXPCL_DTOOLCONFIG int PyInt_AsSsize_t(...);
  EXPCL_DTOOLCONFIG int PyInt_FromLong(...);
  EXPCL_DTOOLCONFIG int PyInt_Type(...);
  EXPCL_DTOOLCONFIG int PyList_Append(...);
  EXPCL_DTOOLCONFIG int PyList_AsTuple(...);
  EXPCL_DTOOLCONFIG int PyList_GetItem(...);
  EXPCL_DTOOLCONFIG int PyList_New(...);
  EXPCL_DTOOLCONFIG int PyList_SetItem(...);
  EXPCL_DTOOLCONFIG int PyList_Type(...);
  EXPCL_DTOOLCONFIG int PyLong_AsLong(...);
  EXPCL_DTOOLCONFIG int PyLong_AsLongLong(...);
  EXPCL_DTOOLCONFIG int PyLong_AsUnsignedLong(...);
  EXPCL_DTOOLCONFIG int PyLong_AsUnsignedLongLong(...);
  EXPCL_DTOOLCONFIG int PyLong_FromLong(...);
  EXPCL_DTOOLCONFIG int PyLong_FromLongLong(...);
  EXPCL_DTOOLCONFIG int PyLong_FromUnsignedLong(...);
  EXPCL_DTOOLCONFIG int PyLong_FromUnsignedLongLong(...);
  EXPCL_DTOOLCONFIG int PyLong_Type(...);
  EXPCL_DTOOLCONFIG int PyMapping_GetItemString(...);
  EXPCL_DTOOLCONFIG int PyModule_AddIntConstant(...);
  EXPCL_DTOOLCONFIG int PyModule_AddObject(...);
  EXPCL_DTOOLCONFIG int PyModule_AddStringConstant(...);
  EXPCL_DTOOLCONFIG int PyNumber_Float(...);
  EXPCL_DTOOLCONFIG int PyNumber_Long(...);
  EXPCL_DTOOLCONFIG int PyObject_Call(...);
  EXPCL_DTOOLCONFIG int PyObject_CallFunction(...);
  EXPCL_DTOOLCONFIG int PyObject_CallMethod(...);
  EXPCL_DTOOLCONFIG int PyObject_CallMethodObjArgs(...);
  EXPCL_DTOOLCONFIG int PyObject_CallObject(...);
  EXPCL_DTOOLCONFIG int PyObject_Cmp(...);
  EXPCL_DTOOLCONFIG int PyObject_Compare(...);
  EXPCL_DTOOLCONFIG int PyObject_Free(...);
  EXPCL_DTOOLCONFIG int PyObject_GenericGetAttr(...);
  EXPCL_DTOOLCONFIG int PyObject_GenericSetAttr(...);
  EXPCL_DTOOLCONFIG int PyObject_GetAttrString(...);
  EXPCL_DTOOLCONFIG int PyObject_HasAttrString(...);
  EXPCL_DTOOLCONFIG int PyObject_IsInstance(...);
  EXPCL_DTOOLCONFIG int PyObject_IsTrue(...);
  EXPCL_DTOOLCONFIG int PyObject_Repr(...);
  EXPCL_DTOOLCONFIG int PyObject_SetAttrString(...);
  EXPCL_DTOOLCONFIG int PyObject_Str(...);
  EXPCL_DTOOLCONFIG int PyObject_Type(...);
  EXPCL_DTOOLCONFIG int PySequence_Check(...);
  EXPCL_DTOOLCONFIG int PySequence_Fast(...);
  EXPCL_DTOOLCONFIG int PySequence_GetItem(...);
  EXPCL_DTOOLCONFIG int PySequence_Size(...);
  EXPCL_DTOOLCONFIG int PySequence_Tuple(...);
  EXPCL_DTOOLCONFIG int PyString_AsString(...);
  EXPCL_DTOOLCONFIG int PyString_AsStringAndSize(...);
  EXPCL_DTOOLCONFIG int PyString_FromString(...);
  EXPCL_DTOOLCONFIG int PyString_FromStringAndSize(...);
  EXPCL_DTOOLCONFIG int PyString_Size(...);
  EXPCL_DTOOLCONFIG int PyString_Type(...);
  EXPCL_DTOOLCONFIG int PySys_GetObject(...);
  EXPCL_DTOOLCONFIG int PyThreadState_Clear(...);
  EXPCL_DTOOLCONFIG int PyThreadState_Delete(...);
  EXPCL_DTOOLCONFIG int PyThreadState_Get(...);
  EXPCL_DTOOLCONFIG int PyThreadState_New(...);
  EXPCL_DTOOLCONFIG int PyThreadState_Swap(...);
  EXPCL_DTOOLCONFIG int PyTuple_GetItem(...);
  EXPCL_DTOOLCONFIG int PyTuple_New(...);
  EXPCL_DTOOLCONFIG int PyTuple_Pack(...);
  EXPCL_DTOOLCONFIG int PyTuple_Size(...);
  EXPCL_DTOOLCONFIG int PyTuple_Type(...);
  EXPCL_DTOOLCONFIG int PyType_GenericAlloc(...);
  EXPCL_DTOOLCONFIG int PyType_IsSubtype(...);
  EXPCL_DTOOLCONFIG int PyType_Ready(...);
  EXPCL_DTOOLCONFIG int PyUnicodeUCS2_FromWideChar(...);
  EXPCL_DTOOLCONFIG int PyUnicodeUCS2_AsWideChar(...);
  EXPCL_DTOOLCONFIG int PyUnicodeUCS2_GetSize(...);
  EXPCL_DTOOLCONFIG int PyUnicodeUCS4_FromWideChar(...);
  EXPCL_DTOOLCONFIG int PyUnicodeUCS4_AsWideChar(...);
  EXPCL_DTOOLCONFIG int PyUnicodeUCS4_GetSize(...);
  EXPCL_DTOOLCONFIG int PyUnicode_Type(...);
  EXPCL_DTOOLCONFIG int Py_BuildValue(...);
  EXPCL_DTOOLCONFIG int Py_InitModule4(...);
  EXPCL_DTOOLCONFIG int Py_InitModule4_64(...);
  EXPCL_DTOOLCONFIG int Py_InitModule4TraceRefs(...);
  EXPCL_DTOOLCONFIG int _PyObject_DebugFree(...);
  EXPCL_DTOOLCONFIG int _PyObject_Del(...);
  EXPCL_DTOOLCONFIG int _Py_Dealloc(...);
  EXPCL_DTOOLCONFIG int _Py_NegativeRefcount(...);
  EXPCL_DTOOLCONFIG int _Py_RefTotal(...);

  EXPCL_DTOOLCONFIG int Py_IsInitialized();

  EXPCL_DTOOLCONFIG extern void *PyExc_AssertionError;
  EXPCL_DTOOLCONFIG extern void *PyExc_AttributeError;
  EXPCL_DTOOLCONFIG extern void *PyExc_FutureWarning;
  EXPCL_DTOOLCONFIG extern void *PyExc_IndexError;
  EXPCL_DTOOLCONFIG extern void *PyExc_RuntimeError;
  EXPCL_DTOOLCONFIG extern void *PyExc_StandardError;
  EXPCL_DTOOLCONFIG extern void *PyExc_StopIteration;
  EXPCL_DTOOLCONFIG extern void *PyExc_TypeError;
  EXPCL_DTOOLCONFIG extern void *PyExc_ValueError;
  EXPCL_DTOOLCONFIG extern void *_Py_NoneStruct;
  EXPCL_DTOOLCONFIG extern void *_Py_NotImplementedStruct;
};


int PyArg_Parse(...) { return 0; };
int PyArg_ParseTuple(...) { return 0; }
int PyArg_ParseTupleAndKeywords(...) { return 0; }
int PyCFunction_New(...) { return 0; };
int PyCFunction_NewEx(...) { return 0; };
int PyCallable_Check(...) { return 0; }
int PyDict_DelItemString(...) { return 0; }
int PyDict_GetItem(...) { return 0; }
int PyDict_GetItemString(...) { return 0; }
int PyDict_New(...) { return 0; };
int PyDict_SetItem(...) { return 0; };
int PyDict_SetItemString(...) { return 0; };
int PyDict_Size(...){ return 0; }
int PyDict_Type(...) { return 0; };
int PyErr_Clear(...) { return 0; };
int PyErr_ExceptionMatches(...) { return 0; };
int PyErr_Fetch(...) { return 0; }
int PyErr_Format(...) { return 0; };
int PyErr_Occurred(...) { return 0; }
int PyErr_Print(...) { return 0; }
int PyErr_Restore(...) { return 0; }
int PyErr_SetString(...) { return 0; }
int PyErr_Warn(...) { return 0; }
int PyErr_WarnEx(...) { return 0; }
int PyEval_CallFunction(...) { return 0; }
int PyEval_CallObjectWithKeywords(...) { return 0; }
int PyEval_InitThreads(...) { return 0; }
int PyEval_RestoreThread(...) { return 0; }
int PyEval_SaveThread(...) { return 0; }
int PyFloat_AsDouble(...) { return 0; }
int PyFloat_FromDouble(...) { return 0; }
int PyFloat_Type(...) { return 0; }
int PyGen_Check(...) { return 0; }
int PyGen_Type(...) { return 0; }
int PyGILState_Ensure(...) { return 0; }
int PyGILState_Release(...) { return 0; }
int PyImport_GetModuleDict(...) { return 0; }
int PyInt_AsLong(...) { return 0; }
int PyInt_AsSsize_t(...) { return 0; }
int PyInt_FromLong(...) { return 0; }
int PyInt_Type(...) { return 0; }
int PyList_Append(...) { return 0; }
int PyList_AsTuple(...) { return 0; }
int PyList_GetItem(...) { return 0; }
int PyList_New(...) { return 0; }
int PyList_SetItem(...) { return 0; }
int PyList_Type(...) { return 0; }
int PyLong_AsLong(...) { return 0; }
int PyLong_AsLongLong(...) { return 0; }
int PyLong_AsUnsignedLong(...) { return 0; }
int PyLong_AsUnsignedLongLong(...) { return 0; }
int PyLong_FromLong(...) { return 0; }
int PyLong_FromLongLong(...) { return 0; }
int PyLong_FromUnsignedLong(...) { return 0; }
int PyLong_FromUnsignedLongLong(...) { return 0; }
int PyLong_Type(...) { return 0; }
int PyMapping_GetItemString(...) { return 0; }
int PyModule_AddIntConstant(...) { return 0; };
int PyModule_AddObject(...) { return 0; };
int PyModule_AddStringConstant(...) { return 0; };
int PyNumber_Float(...) { return 0; }
int PyNumber_Long(...) { return 0; }
int PyObject_Call(...) { return 0; }
int PyObject_CallFunction(...) { return 0; }
int PyObject_CallMethod(...) { return 0; }
int PyObject_CallMethodObjArgs(...) { return 0; }
int PyObject_CallObject(...) { return 0; }
int PyObject_Cmp(...) { return 0; }
int PyObject_Compare(...) { return 0; }
int PyObject_Free(...) { return 0; }
int PyObject_GenericGetAttr(...) { return 0; };
int PyObject_GenericSetAttr(...) { return 0; };
int PyObject_GetAttrString(...) { return 0; }
int PyObject_HasAttrString(...) { return 0; }
int PyObject_IsInstance(...) { return 0; }
int PyObject_IsTrue(...) { return 0; }
int PyObject_Repr(...) { return 0; }
int PyObject_SetAttrString(...) { return 0; }
int PyObject_Str(...) { return 0; }
int PyObject_Type(...) { return 0; }
int PySequence_Check(...) { return 0; }
int PySequence_Fast(...) { return 0; }
int PySequence_GetItem(...) { return 0; }
int PySequence_Size(...) { return 0; }
int PySequence_Tuple(...) { return 0; }
int PyString_AsString(...) { return 0; }
int PyString_AsStringAndSize(...) { return 0; }
int PyString_FromString(...) { return 0; }
int PyString_FromStringAndSize(...) { return 0; }
int PyString_Size(...) { return 0; }
int PyString_Type(...) { return 0; }
int PySys_GetObject(...) { return 0; }
int PyThreadState_Clear(...) { return 0; }
int PyThreadState_Delete(...) { return 0; }
int PyThreadState_Get(...) { return 0; }
int PyThreadState_New(...) { return 0; }
int PyThreadState_Swap(...) { return 0; }
int PyTuple_GetItem(...) { return 0; }
int PyTuple_New(...) { return 0; }
int PyTuple_Pack(...) { return 0; }
int PyTuple_Size(...) { return 0; };
int PyTuple_Type(...) { return 0; };
int PyType_GenericAlloc(...) { return 0; };
int PyType_IsSubtype(...) { return 0; }
int PyType_Ready(...) { return 0; };
int PyUnicodeUCS2_FromWideChar(...) { return 0; }
int PyUnicodeUCS2_AsWideChar(...) { return 0; }
int PyUnicodeUCS2_GetSize(...) { return 0; }
int PyUnicodeUCS4_FromWideChar(...) { return 0; }
int PyUnicodeUCS4_AsWideChar(...) { return 0; }
int PyUnicodeUCS4_GetSize(...) { return 0; }
int PyUnicode_Type(...) { return 0; }
int Py_BuildValue(...) { return 0; }
int Py_InitModule4(...) { return 0; }
int Py_InitModule4_64(...) { return 0; }
int Py_InitModule4TraceRefs(...) { return 0; };
int _PyObject_DebugFree(...) { return 0; };
int _PyObject_Del(...) { return 0; };
int _Py_Dealloc(...) { return 0; };
int _Py_NegativeRefcount(...) { return 0; };
int _Py_RefTotal(...) { return 0; };

// We actually might call this one.
int Py_IsInitialized() {
  return 0;
}


void *PyExc_AssertionError = (void *)NULL;
void *PyExc_AttributeError = (void *)NULL;
void *PyExc_FutureWarning = (void *)NULL;
void *PyExc_IndexError = (void *)NULL;
void *PyExc_RuntimeError = (void *)NULL;
void *PyExc_StandardError = (void *)NULL;
void *PyExc_StopIteration = (void *)NULL;
void *PyExc_TypeError = (void *)NULL;
void *PyExc_ValueError = (void *)NULL;
void *_Py_NoneStruct = (void *)NULL;
void *_Py_NotImplementedStruct = (void *)NULL;


void
pystub() {
}

