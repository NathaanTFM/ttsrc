#define OTHER_LIBS \
    express:c pandaexpress:m \
    pstatclient:c pipeline:c panda:m \
    interrogatedb:c dconfig:c dtoolconfig:m \
    dtoolutil:c dtoolbase:c dtool:m prc:c pandabase:c \
    downloader:c $[if $[HAVE_NET],net:c] $[if $[WANT_NATIVE_NET],nativenet:c] \
    linmath:c putil:c

#define LOCAL_LIBS \
    directbase
#define YACC_PREFIX dcyy
#define C++FLAGS -DWITHIN_PANDA
#define UNIX_SYS_LIBS m
#define USE_PACKAGES python

#begin lib_target
  #define TARGET dcparser

  #define COMBINED_SOURCES $[TARGET]_composite1.cxx  $[TARGET]_composite2.cxx

  #define SOURCES \
     dcAtomicField.h dcAtomicField.I dcClass.h dcClass.I \
     dcDeclaration.h \
     dcField.h dcField.I \
     dcFile.h dcFile.I \
     dcKeyword.h dcKeywordList.h \
     dcLexer.lxx  \
     dcLexerDefs.h dcMolecularField.h dcParser.yxx dcParserDefs.h  \
     dcSubatomicType.h \
     dcPackData.h dcPackData.I \
     dcPacker.h dcPacker.I \
     dcPackerCatalog.h dcPackerCatalog.I \
     dcPackerInterface.h dcPackerInterface.I \
     dcParameter.h dcClassParameter.h dcArrayParameter.h \
     dcSimpleParameter.h dcSwitchParameter.h \
     dcNumericRange.h dcNumericRange.I \
     dcSwitch.h \
     dcTypedef.h \
     dcPython.h \
     dcbase.h dcindent.h hashGenerator.h  \
     primeNumberGenerator.h  

  #define INCLUDED_SOURCES \
     dcAtomicField.cxx dcClass.cxx \
     dcDeclaration.cxx \
     dcField.cxx dcFile.cxx \
     dcKeyword.cxx dcKeywordList.cxx \
     dcMolecularField.cxx dcSubatomicType.cxx \
     dcPackData.cxx \
     dcPacker.cxx \
     dcPackerCatalog.cxx \
     dcPackerInterface.cxx \
     dcParameter.cxx dcClassParameter.cxx dcArrayParameter.cxx \
     dcSimpleParameter.cxx dcSwitchParameter.cxx \
     dcSwitch.cxx \
     dcTypedef.cxx \
     dcindent.cxx  \
     hashGenerator.cxx primeNumberGenerator.cxx 

  //  #define SOURCES $[SOURCES] $[INCLUDED_SOURCES]
  //  #define COMBINED_SOURCES

  #define IGATESCAN all
#end lib_target
