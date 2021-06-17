#begin ss_lib_target
  #define TARGET vrmlegg
  #define LOCAL_LIBS converter pvrml pandatoolbase
  #define OTHER_LIBS \
    egg:c pandaegg:m \
    mathutil:c linmath:c event:c putil:c express:c \
    pipeline:c \
    panda:m \
    pandabase:c pandaexpress:m \
    interrogatedb:c prc:c dconfig:c dtoolconfig:m \
    dtoolutil:c dtoolbase:c dtool:m

  #define SOURCES \
    indexedFaceSet.cxx indexedFaceSet.h \
    vrmlAppearance.cxx vrmlAppearance.h \
    vrmlToEggConverter.cxx vrmlToEggConverter.h

  #define INSTALL_HEADERS \
    indexedFaceSet.h \
    vrmlAppearance.h \
    vrmlToEggConverter.h

#end ss_lib_target
