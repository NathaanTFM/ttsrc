// Filename: compress_string.cxx
// Created by:  drose (09Aug09)
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

#include "compress_string.h"

#ifdef HAVE_ZLIB
#include "zStream.h"
#include "virtualFileSystem.h"
#include "config_express.h"

////////////////////////////////////////////////////////////////////
//     Function: compress_string
//       Access: Published
//  Description: Compress the indicated source string at the given
//               compression level (1 through 9).  Returns the
//               compressed string.
////////////////////////////////////////////////////////////////////
string
compress_string(const string &source, int compression_level) {
  ostringstream dest;

  {
    OCompressStream compress;
    compress.open(&dest, false, compression_level);
    compress.write(source.data(), source.length());

    if (compress.fail()) {
      return string();
    }
  }

  return dest.str();
}

////////////////////////////////////////////////////////////////////
//     Function: decompress_string
//       Access: Published
//  Description: Decompresss the previously-compressed string()).  The
//               return value is the decompressed string.
//
//               Note that a decompression error cannot easily be
//               detected, and the return value may simply be a
//               garbage or truncated string.
////////////////////////////////////////////////////////////////////
string
decompress_string(const string &source) {
  istringstream source_stream(source);
  ostringstream dest_stream;

  if (!decompress_stream(source_stream, dest_stream)) {
    return string();
  }

  return dest_stream.str();
}

////////////////////////////////////////////////////////////////////
//     Function: compress_file
//       Access: Published
//  Description: Compresss the data from the source file at the given
//               compression level (1 through 9).  The source file is
//               read in its entirety, and the compressed results are
//               written to the dest file, overwriting its contents.
//               The return value is bool on success, or false on
//               failure.
////////////////////////////////////////////////////////////////////
EXPCL_PANDAEXPRESS bool 
compress_file(const Filename &source, const Filename &dest, int compression_level) {
  Filename source_filename = Filename::binary_filename(source);
  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
  istream *source_stream = vfs->open_read_file(source_filename, true);
  if (source_stream == NULL) {
    express_cat.info() << "Couldn't open file " << source_filename << "\n";
    return false;
  }
  
  Filename dest_filename = Filename::binary_filename(dest);
  pofstream dest_stream;
  if (!dest_filename.open_write(dest_stream)) {
    express_cat.info() << "Couldn't open file " << dest_filename << "\n";
    vfs->close_read_file(source_stream);
    return false;
  }
    
  bool result = compress_stream(*source_stream, dest_stream, compression_level);
  vfs->close_read_file(source_stream);
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: decompress_file
//       Access: Published
//  Description: Decompresss the data from the source file.  The
//               source file is read in its entirety, and the
//               decompressed results are written to the dest file,
//               overwriting its contents.  The return value is bool
//               on success, or false on failure.
//
//               Note that a decompression error cannot easily be
//               detected, and the output may simply be a garbage
//               or truncated string.
////////////////////////////////////////////////////////////////////
EXPCL_PANDAEXPRESS bool 
decompress_file(const Filename &source, const Filename &dest) {
  Filename source_filename = Filename::binary_filename(source);
  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
  istream *source_stream = vfs->open_read_file(source_filename, false);
  if (source_stream == NULL) {
    express_cat.info() << "Couldn't open file " << source_filename << "\n";
    return false;
  }
  
  Filename dest_filename = Filename::binary_filename(dest);
  pofstream dest_stream;
  if (!dest_filename.open_write(dest_stream)) {
    express_cat.info() << "Couldn't open file " << dest_filename << "\n";
    vfs->close_read_file(source_stream);
    return false;
  }
    
  bool result = decompress_stream(*source_stream, dest_stream);
  vfs->close_read_file(source_stream);
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: compress_stream
//       Access: Published
//  Description: Compresss the data from the source stream at the
//               given compression level (1 through 9).  The source
//               stream is read from its current position to the
//               end-of-file, and the compressed results are written
//               to the dest stream.  The return value is bool on
//               success, or false on failure.
////////////////////////////////////////////////////////////////////
bool
compress_stream(istream &source, ostream &dest, int compression_level) {
  OCompressStream compress;
  compress.open(&dest, false, compression_level);
    
  static const size_t buffer_size = 4096;
  char buffer[buffer_size];

  source.read(buffer, buffer_size);
  size_t count = source.gcount();
  while (count != 0) {
    compress.write(buffer, count);
    source.read(buffer, buffer_size);
    count = source.gcount();
  }
  compress.close();

  return (!source.fail() || source.eof()) && (!compress.fail());
}

////////////////////////////////////////////////////////////////////
//     Function: decompress_stream
//       Access: Published
//  Description: Decompresss the data from the previously-compressed
//               source stream.  The source stream is read from its
//               current position to the end-of-file, and the
//               decompressed results are written to the dest stream.
//               The return value is bool on success, or false on
//               failure.
//
//               Note that a decompression error cannot easily be
//               detected, and the output may simply be a garbage
//               or truncated string.
////////////////////////////////////////////////////////////////////
bool
decompress_stream(istream &source, ostream &dest) {
  IDecompressStream decompress(&source, false);

  static const size_t buffer_size = 4096;
  char buffer[buffer_size];

  decompress.read(buffer, buffer_size);
  size_t count = decompress.gcount();
  while (count != 0) {
    dest.write(buffer, count);
    decompress.read(buffer, buffer_size);
    count = decompress.gcount();
  }
  
  return (!decompress.fail() || decompress.eof()) && (!dest.fail());
}

#endif // HAVE_ZLIB
