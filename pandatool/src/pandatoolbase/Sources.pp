#define OTHER_LIBS \
   pipeline:c panda:m \
   express:c putil:c pandabase:c pandaexpress:m \
   interrogatedb:c prc:c dconfig:c dtoolconfig:m \
   dtoolutil:c dtoolbase:c dtool:m

#begin ss_lib_target
  #define TARGET pandatoolbase
  
  #define SOURCES \
    animationConvert.cxx animationConvert.h \
    config_pandatoolbase.cxx config_pandatoolbase.h \
    distanceUnit.cxx distanceUnit.h \
    pandatoolbase.cxx pandatoolbase.h pandatoolsymbols.h \
    pathReplace.cxx pathReplace.I pathReplace.h \
    pathStore.cxx pathStore.h

  #define INSTALL_HEADERS \
    animationConvert.h \
    config_pandatoolbase.h \
    distanceUnit.h \
    pandatoolbase.h pandatoolsymbols.h \
    pathReplace.I pathReplace.h \
    pathStore.h

#end ss_lib_target
