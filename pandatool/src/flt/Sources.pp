#define USE_PACKAGES zlib

#begin ss_lib_target
  #define TARGET flt
  #define LOCAL_LIBS converter pandatoolbase
  #define OTHER_LIBS \
    egg:c pandaegg:m \
    pipeline:c pstatclient:c downloader:c net:c nativenet:c \
    mathutil:c linmath:c putil:c event:c express:c \
    panda:m \
    pandabase:c pandaexpress:m \
    interrogatedb:c prc:c dconfig:c dtoolconfig:m \
    dtoolutil:c dtoolbase:c dtool:m
  #define UNIX_SYS_LIBS m
  
  #define COMBINED_SOURCES $[TARGET]_composite1.cxx   

  #define SOURCES \
     config_flt.h fltBead.h fltBeadID.h fltCurve.I fltCurve.h  \
     fltError.h fltExternalReference.h fltEyepoint.h fltFace.I  \
     fltFace.h fltGeometry.I fltGeometry.h fltGroup.h fltHeader.h  \
     fltInstanceDefinition.h fltInstanceRef.h fltLOD.h  \
     fltLightSourceDefinition.h fltLocalVertexPool.I  \
     fltLocalVertexPool.h fltMaterial.h fltMesh.I fltMesh.h  \
     fltMeshPrimitive.I fltMeshPrimitive.h fltObject.h  \
     fltOpcode.h fltPackedColor.I fltPackedColor.h fltRecord.I  \
     fltRecord.h fltRecordReader.h fltRecordWriter.h fltTexture.h  \
     fltTrackplane.h fltTransformGeneralMatrix.h  \
     fltTransformPut.h fltTransformRecord.h  \
     fltTransformRotateAboutEdge.h fltTransformRotateAboutPoint.h  \
     fltTransformRotateScale.h fltTransformScale.h  \
     fltTransformTranslate.h fltUnsupportedRecord.h \
     fltVectorRecord.h \
     fltVertex.I fltVertex.h fltVertexList.h 
    
 #define INCLUDED_SOURCES \
     config_flt.cxx fltBead.cxx fltBeadID.cxx fltCurve.cxx  \
     fltError.cxx fltExternalReference.cxx fltEyepoint.cxx  \
     fltFace.cxx fltGeometry.cxx fltGroup.cxx fltHeader.cxx  \
     fltInstanceDefinition.cxx fltInstanceRef.cxx fltLOD.cxx  \
     fltLightSourceDefinition.cxx fltLocalVertexPool.cxx  \
     fltMaterial.cxx fltMesh.cxx fltMeshPrimitive.cxx  \
     fltObject.cxx fltOpcode.cxx fltPackedColor.cxx fltRecord.cxx  \
     fltRecordReader.cxx fltRecordWriter.cxx fltTexture.cxx  \
     fltTrackplane.cxx fltTransformGeneralMatrix.cxx  \
     fltTransformPut.cxx fltTransformRecord.cxx  \
     fltTransformRotateAboutEdge.cxx  \
     fltTransformRotateAboutPoint.cxx fltTransformRotateScale.cxx  \
     fltTransformScale.cxx fltTransformTranslate.cxx  \
     fltUnsupportedRecord.cxx fltVectorRecord.cxx \
     fltVertex.cxx fltVertexList.cxx 

  #define INSTALL_HEADERS \
    fltBead.h fltBeadID.h fltCurve.I fltCurve.h \
    fltError.h fltExternalReference.h \
    fltEyepoint.h fltFace.I fltFace.h fltGeometry.I fltGeometry.h \
    fltGroup.h fltHeader.h \
    fltInstanceDefinition.h fltInstanceRef.h fltLOD.h \
    fltLightSourceDefinition.h fltLocalVertexPool.I fltLocalVertexPool.h \
    fltMaterial.h fltMesh.I fltMesh.h \
    fltMeshPrimitive.I fltMeshPrimitive.h \
    fltObject.h fltOpcode.h \
    fltPackedColor.I fltPackedColor.h fltRecord.I fltRecord.h \
    fltRecordReader.h fltRecordWriter.h fltTexture.h fltTrackplane.h \
    fltTransformGeneralMatrix.h fltTransformPut.h fltTransformRecord.h \
    fltTransformRotateAboutEdge.h fltTransformRotateAboutPoint.h \
    fltTransformRotateScale.h fltTransformScale.h \
    fltTransformTranslate.h fltUnsupportedRecord.h fltVectorRecord.h \
    fltVertex.I fltVertex.h fltVertexList.h

#end ss_lib_target
