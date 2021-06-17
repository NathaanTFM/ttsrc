#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                  dtoolutil:c dtoolbase:c dtool:m prc:c
#define LOCAL_LIBS pipeline express pandabase
#define USE_PACKAGES zlib

#begin lib_target
  #define TARGET putil
 
  #define COMBINED_SOURCES $[TARGET]_composite1.cxx $[TARGET]_composite2.cxx   
  
  #define SOURCES \
    animInterface.h animInterface.I \
    bam.h \
    bamCache.h bamCache.I \
    bamCacheIndex.h bamCacheIndex.I \
    bamCacheRecord.h bamCacheRecord.I \
    bamEnums.h \
    bamReader.I bamReader.N bamReader.h bamReaderParam.I \
    bamReaderParam.h \
    bamWriter.I bamWriter.h \
    bitArray.I bitArray.h \
    bitMask.I bitMask.h \
    buttonHandle.I \
    buttonHandle.h buttonRegistry.I buttonRegistry.h \
    cachedTypedWritableReferenceCount.h cachedTypedWritableReferenceCount.I \
    callbackData.h callbackData.I \
    callbackObject.h callbackObject.I \
    clockObject.h clockObject.I \
    collideMask.h \
    copyOnWriteObject.h copyOnWriteObject.I \
    copyOnWritePointer.h copyOnWritePointer.I \
    compareTo.I compareTo.h \
    config_util.N config_util.h configurable.h \
    cPointerCallbackObject.h cPointerCallbackObject.I \
    datagramInputFile.I datagramInputFile.h \
    datagramOutputFile.I datagramOutputFile.h \
    doubleBitMask.I doubleBitMask.h \
    drawMask.h \
    factory.I factory.h \
    factoryBase.I factoryBase.h \
    factoryParam.I factoryParam.h factoryParams.I \
    factoryParams.h \
    firstOfPairCompare.I firstOfPairCompare.h \
    firstOfPairLess.I firstOfPairLess.h \
    globalPointerRegistry.I globalPointerRegistry.h \
    indirectCompareNames.I indirectCompareNames.h \
    indirectCompareSort.I indirectCompareSort.h \
    indirectCompareTo.I indirectCompareTo.h \
    ioPtaDatagramFloat.h ioPtaDatagramInt.h \
    ioPtaDatagramShort.h keyboardButton.h lineStream.I \
    lineStream.h lineStreamBuf.I lineStreamBuf.h \
    linkedListNode.I linkedListNode.h \
    load_prc_file.h \
    loaderOptions.I loaderOptions.h \
    modifierButtons.I modifierButtons.h mouseButton.h \
    mouseData.I mouseData.h nameUniquifier.I nameUniquifier.h \
    nodeCachedReferenceCount.h nodeCachedReferenceCount.I \
    pbitops.I pbitops.h \
    portalMask.h \
    pta_double.h \
    pta_ushort.h \
    pythonCallbackObject.h pythonCallbackObject.I \
    simpleHashMap.I simpleHashMap.h \
    sparseArray.I sparseArray.h \
    string_utils.I string_utils.N string_utils.h \
    timedCycle.I timedCycle.h typedWritable.I \
    typedWritable.h typedWritableReferenceCount.I \
    typedWritableReferenceCount.h updateSeq.I updateSeq.h \
    uniqueIdAllocator.h \
    vector_double.h vector_typedWritable.h \
    vector_ushort.h vector_writable.h \
    writableConfigurable.h \
    writableParam.I writableParam.h 
    
 #define INCLUDED_SOURCES \
    animInterface.cxx \
    bamCache.cxx \
    bamCacheIndex.cxx \
    bamCacheRecord.cxx \
    bamEnums.cxx \
    bamReader.cxx bamReaderParam.cxx \
    bamWriter.cxx \
    bitArray.cxx \
    bitMask.cxx \
    buttonHandle.cxx buttonRegistry.cxx \
    cachedTypedWritableReferenceCount.cxx \
    callbackData.cxx \
    callbackObject.cxx \
    clockObject.cxx \
    copyOnWriteObject.cxx \
    copyOnWritePointer.cxx \
    config_util.cxx configurable.cxx \
    cPointerCallbackObject.cxx \
    datagramInputFile.cxx datagramOutputFile.cxx \
    doubleBitMask.cxx \
    factoryBase.cxx \
    factoryParam.cxx factoryParams.cxx \
    globalPointerRegistry.cxx \
    ioPtaDatagramFloat.cxx \
    ioPtaDatagramInt.cxx ioPtaDatagramShort.cxx \
    keyboardButton.cxx lineStream.cxx lineStreamBuf.cxx \
    linkedListNode.cxx \
    load_prc_file.cxx \
    loaderOptions.cxx \
    modifierButtons.cxx mouseButton.cxx mouseData.cxx \
    nameUniquifier.cxx \
    nodeCachedReferenceCount.cxx \
    pbitops.cxx \
    pta_double.cxx \
    pta_ushort.cxx \
    pythonCallbackObject.cxx \
    simpleHashMap.cxx \
    sparseArray.cxx \
    string_utils.cxx \
    timedCycle.cxx typedWritable.cxx \
    typedWritableReferenceCount.cxx updateSeq.cxx \
    uniqueIdAllocator.cxx \
    vector_double.cxx \
    vector_typedWritable.cxx \
    vector_ushort.cxx vector_writable.cxx \
    writableConfigurable.cxx writableParam.cxx 

  #define INSTALL_HEADERS \
    animInterface.h animInterface.I \
    bam.h \
    bamCache.h bamCache.I \
    bamCacheIndex.h bamCacheIndex.I \
    bamCacheRecord.h bamCacheRecord.I \
    bamEnums.h \
    bamReader.I bamReader.h bamReaderParam.I bamReaderParam.h \
    bamWriter.I bamWriter.h \
    bitArray.I bitArray.h \
    bitMask.I bitMask.h \
    buttonHandle.I buttonHandle.h buttonRegistry.I \
    buttonRegistry.h \
    cachedTypedWritableReferenceCount.h cachedTypedWritableReferenceCount.I \
    callbackData.h callbackData.I \
    callbackObject.h callbackObject.I \
    clockObject.h clockObject.I \
    collideMask.h \
    copyOnWriteObject.h copyOnWriteObject.I \
    copyOnWritePointer.h copyOnWritePointer.I \
    compareTo.I compareTo.h \
    config_util.h configurable.h \
    cPointerCallbackObject.h cPointerCallbackObject.I \
    datagramInputFile.I datagramInputFile.h \
    datagramOutputFile.I datagramOutputFile.h \
    doubleBitMask.I doubleBitMask.h \
    drawMask.h \
    factory.I factory.h \
    factoryBase.I factoryBase.h factoryParam.I factoryParam.h \
    factoryParams.I factoryParams.h \
    firstOfPairCompare.I firstOfPairCompare.h \
    firstOfPairLess.I firstOfPairLess.h \
    globalPointerRegistry.I globalPointerRegistry.h \
    indirectCompareNames.I indirectCompareNames.h \
    indirectCompareSort.I indirectCompareSort.h \
    indirectCompareTo.I indirectCompareTo.h \
    ioPtaDatagramFloat.h ioPtaDatagramInt.h \
    ioPtaDatagramShort.h iterator_types.h keyboardButton.h lineStream.I \
    lineStream.h lineStreamBuf.I lineStreamBuf.h \
    linkedListNode.I linkedListNode.h \
    load_prc_file.h \
    loaderOptions.I loaderOptions.h \
    modifierButtons.I \
    modifierButtons.h mouseButton.h mouseData.I mouseData.h \
    nameUniquifier.I nameUniquifier.h \
    nodeCachedReferenceCount.h nodeCachedReferenceCount.I \
    portalMask.h \
    pbitops.I pbitops.h \
    pta_double.h \
    pta_ushort.h \
    pythonCallbackObject.h pythonCallbackObject.I \
    simpleHashMap.I simpleHashMap.h \
    sparseArray.I sparseArray.h \
    string_utils.I string_utils.h \
    timedCycle.I timedCycle.h typedWritable.I \
    typedWritable.h typedWritableReferenceCount.I \
    typedWritableReferenceCount.h updateSeq.I updateSeq.h \
    uniqueIdAllocator.h \
    vector_double.h vector_typedWritable.h \
    vector_ushort.h vector_writable.h \
    writableConfigurable.h writableParam.I \
    writableParam.h

  #define IGATESCAN all

#end lib_target

#begin test_bin_target
  #define TARGET test_bamRead
  #define LOCAL_LIBS \
    putil pgraph

  #define SOURCES \
    test_bam.cxx test_bam.h test_bamRead.cxx

#end test_bin_target

#begin test_bin_target
  #define TARGET test_bamWrite
  #define LOCAL_LIBS \
    putil pgraph

  #define SOURCES \
    test_bam.cxx test_bam.h test_bamWrite.cxx

#end test_bin_target

#begin test_bin_target
  #define TARGET test_filename

  #define SOURCES \
    test_filename.cxx

#end test_bin_target

#begin test_bin_target
  #define TARGET test_uniqueIdAllocator

  #define SOURCES \
    uniqueIdAllocator.cxx test_uniqueIdAllocator.cxx

#end test_bin_target

#begin test_bin_target
  #define TARGET test_glob

  #define SOURCES \
    test_glob.cxx

  #define LOCAL_LIBS $[LOCAL_LIBS] putil
  #define OTHER_LIBS $[OTHER_LIBS] pystub

#end test_bin_target

#begin test_bin_target
  #define TARGET test_linestream

  #define SOURCES \
    test_linestream.cxx

  #define LOCAL_LIBS $[LOCAL_LIBS] putil
  #define OTHER_LIBS $[OTHER_LIBS] pystub

#end test_bin_target
