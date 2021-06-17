#begin ss_lib_target
  #define TARGET eggcharbase
  #define LOCAL_LIBS \
    eggbase progbase
  #define OTHER_LIBS \
    egg:c pandaegg:m \
    event:c linmath:c mathutil:c pnmimage:c putil:c \
    pipeline:c pstatclient:c downloader:c net:c nativenet:c \
    panda:m \
    pandabase:c express:c pandaexpress:m \
    interrogatedb:c prc:c dconfig:c dtooolconfig:m \
    dtoolutil:c dtoolbase:c dtool:m
  #define USE_PACKAGES zlib
    
  #define COMBINED_SOURCES $[TARGET]_composite1.cxx 

  #define SOURCES \ 
     config_eggcharbase.h eggBackPointer.h \
     eggCharacterCollection.h eggCharacterCollection.I \
     eggCharacterData.h eggCharacterData.I \
     eggCharacterDb.I eggCharacterDb.h \
     eggCharacterFilter.h \
     eggComponentData.h eggComponentData.I \
     eggJointData.h \
     eggJointData.I eggJointPointer.h eggJointPointer.I \
     eggJointNodePointer.h \
     eggMatrixTablePointer.h eggScalarTablePointer.h \
     eggSliderData.h eggSliderData.I \
     eggVertexPointer.h
    
  #define INCLUDED_SOURCES \
     config_eggcharbase.cxx eggBackPointer.cxx \
     eggCharacterCollection.cxx eggCharacterData.cxx \
     eggCharacterDb.cxx \
     eggCharacterFilter.cxx eggComponentData.cxx eggJointData.cxx \
     eggJointPointer.cxx eggJointNodePointer.cxx \
     eggMatrixTablePointer.cxx eggScalarTablePointer.cxx \
     eggSliderData.cxx \
     eggSliderPointer.cxx \
     eggVertexPointer.cxx 

  #define INSTALL_HEADERS \
    eggBackPointer.h \
    eggCharacterCollection.I eggCharacterCollection.h \
    eggCharacterData.I eggCharacterData.h \
    eggCharacterDb.I eggCharacterDb.h \
    eggCharacterFilter.h \
    eggComponentData.I eggComponentData.h \
    eggJointData.h eggJointData.I \
    eggJointPointer.h eggJointPointer.I \
    eggJointNodePointer.h \
    eggMatrixTablePointer.h \
    eggScalarTablePointer.h \
    eggSliderData.I eggSliderData.h \
    eggVertexPointer.h

#end ss_lib_target

