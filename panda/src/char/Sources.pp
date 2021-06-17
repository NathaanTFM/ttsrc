#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                   dtoolutil:c dtoolbase:c dtool:m prc:c
                   
#begin lib_target
  #define TARGET char
  #define LOCAL_LIBS \
    chan linmath putil event mathutil gsgbase \
    pstatclient    
    
  #define COMBINED_SOURCES $[TARGET]_composite1.cxx $[TARGET]_composite2.cxx    

  #define SOURCES \
    character.I character.h \
    characterJoint.I characterJoint.h \
    characterJointBundle.I characterJointBundle.h \
    characterJointEffect.h characterJointEffect.I \
    characterSlider.h \
    characterVertexSlider.I characterVertexSlider.h \
    config_char.h \
    jointVertexTransform.I jointVertexTransform.h
    
  #define INCLUDED_SOURCES \
    character.cxx \
    characterJoint.cxx characterJointBundle.cxx  \
    characterJointEffect.cxx \
    characterSlider.cxx \
    characterVertexSlider.cxx \
    config_char.cxx  \
    jointVertexTransform.cxx

  #define INSTALL_HEADERS \
    character.I character.h \
    characterJoint.I characterJoint.h \
    characterJointBundle.I characterJointBundle.h \
    characterJointEffect.h characterJointEffect.I \
    characterSlider.h \
    characterVertexSlider.I characterVertexSlider.h \
    config_char.h \
    jointVertexTransform.I jointVertexTransform.h
    
  #define IGATESCAN all

#end lib_target

