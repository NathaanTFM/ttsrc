// Filename: encrypt_string.cxx
// Created by:  drose (30Jan07)
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

#include "encrypt_string.h"

#ifdef HAVE_OPENSSL
#include "encryptStream.h"
#include "virtualFileSystem.h"
#include "config_express.h"

////////////////////////////////////////////////////////////////////
//     Function: encrypt_string
//       Access: Published
//  Description: Encrypts the indicated source string using the given
//               password, and the algorithm specified by
//               encryption-algorithm.  Returns the encrypted string.
////////////////////////////////////////////////////////////////////
string
encrypt_string(const string &source, const string &password,
               const string &algorithm, int key_length, int iteration_count) {
  ostringstream dest;

  {
    OEncryptStream encrypt;
    if (!algorithm.empty()) {
      encrypt.set_algorithm(algorithm);
    }
    if (key_length > 0) {
      encrypt.set_key_length(key_length);
    }
    if (iteration_count >= 0) {
      encrypt.set_iteration_count(iteration_count);
    }
    encrypt.open(&dest, false, password);
    encrypt.write(source.data(), source.length());

    if (encrypt.fail()) {
      return string();
    }
  }

  return dest.str();
}

////////////////////////////////////////////////////////////////////
//     Function: decrypt_string
//       Access: Published
//  Description: Decrypts the previously-encrypted string using the
//               given password (which must be the same password
//               passed to encrypt()).  The return value is the
//               decrypted string.
//
//               Note that a decryption error, including an incorrect
//               password, cannot easily be detected, and the return
//               value may simply be a garbage string.
////////////////////////////////////////////////////////////////////
string
decrypt_string(const string &source, const string &password) {
  istringstream source_stream(source);
  ostringstream dest_stream;

  if (!decrypt_stream(source_stream, dest_stream, password)) {
    return string();
  }

  return dest_stream.str();
}

////////////////////////////////////////////////////////////////////
//     Function: encrypt_file
//       Access: Published
//  Description: Encrypts the data from the source file using the
//               given password.  The source file is read in its
//               entirety, and the encrypted results are written to
//               the dest file, overwriting its contents.  The return
//               value is bool on success, or false on failure.
////////////////////////////////////////////////////////////////////
EXPCL_PANDAEXPRESS bool 
encrypt_file(const Filename &source, const Filename &dest, const string &password,
             const string &algorithm, int key_length, int iteration_count) {
  Filename source_filename = source;
  if (!source_filename.is_binary() && !source_filename.is_text()) {
    // The default is binary, if not specified otherwise.
    source_filename.set_binary();
  }
  
  Filename dest_filename = Filename::binary_filename(dest);
  pofstream dest_stream;
  if (!dest_filename.open_write(dest_stream)) {
    express_cat.info() << "Couldn't open file " << dest_filename << "\n";
    return false;
  }

  // Try to open the file from disk first, instead of using the vfs,
  // so we can get the newline conversion with a text file.  This is a
  // little weird if you have a vfs file shadowing a disk file, but
  // whatever.
  if (source_filename.is_text()) {
    pifstream source_stream;
    if (source_filename.open_read(source_stream)) {
      bool result = encrypt_stream(source_stream, dest_stream, password,
                                   algorithm, key_length, iteration_count);
      return result;
    }
  }

  // OK, couldn't read the disk file, or it wasn't set in text mode.
  // Read the file from the vfs, and sorry--no text conversion for
  // you.
  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
  istream *source_stream = vfs->open_read_file(source_filename, true);
  if (source_stream == NULL) {
    express_cat.info() << "Couldn't open file " << source_filename << "\n";
    return false;
  }
    
  bool result = encrypt_stream(*source_stream, dest_stream, password,
                               algorithm, key_length, iteration_count);
  vfs->close_read_file(source_stream);
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: decrypt_file
//       Access: Published
//  Description: Decrypts the data from the source file using the
//               given password (which must match the same password
//               passed to encrypt()).  The source file is read in its
//               entirety, and the decrypted results are written to
//               the dest file, overwriting its contents.  The return
//               value is bool on success, or false on failure.
//
//               Note that a decryption error, including an incorrect
//               password, cannot easily be detected, and the output
//               may simply be a garbage string.
////////////////////////////////////////////////////////////////////
EXPCL_PANDAEXPRESS bool 
decrypt_file(const Filename &source, const Filename &dest, const string &password) {
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
    
  bool result = decrypt_stream(*source_stream, dest_stream, password);
  vfs->close_read_file(source_stream);
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: encrypt_stream
//       Access: Published
//  Description: Encrypts the data from the source stream using the
//               given password.  The source stream is read from its
//               current position to the end-of-file, and the
//               encrypted results are written to the dest stream.
//               The return value is bool on success, or false on
//               failure.
////////////////////////////////////////////////////////////////////
bool
encrypt_stream(istream &source, ostream &dest, const string &password,
               const string &algorithm, int key_length, int iteration_count) {
  OEncryptStream encrypt;
  if (!algorithm.empty()) {
    encrypt.set_algorithm(algorithm);
  }
  if (key_length > 0) {
    encrypt.set_key_length(key_length);
  }
  if (iteration_count >= 0) {
    encrypt.set_iteration_count(iteration_count);
  }
  encrypt.open(&dest, false, password);
    
  static const size_t buffer_size = 4096;
  char buffer[buffer_size];

  source.read(buffer, buffer_size);
  size_t count = source.gcount();
  while (count != 0) {
    encrypt.write(buffer, count);
    source.read(buffer, buffer_size);
    count = source.gcount();
  }
  encrypt.close();

  return (!source.fail() || source.eof()) && (!encrypt.fail());
}

////////////////////////////////////////////////////////////////////
//     Function: decrypt_stream
//       Access: Published
//  Description: Decrypts the data from the previously-encrypted
//               source stream using the given password (which must be
//               the same password passed to encrypt()).  The source
//               stream is read from its current position to the
//               end-of-file, and the decrypted results are written to
//               the dest stream.  The return value is bool on
//               success, or false on failure.
//
//               Note that a decryption error, including an incorrect
//               password, cannot easily be detected, and the output
//               may simply be a garbage string.
////////////////////////////////////////////////////////////////////
bool
decrypt_stream(istream &source, ostream &dest, const string &password) {
  IDecryptStream decrypt(&source, false, password);

  static const size_t buffer_size = 4096;
  char buffer[buffer_size];

  decrypt.read(buffer, buffer_size);
  size_t count = decrypt.gcount();
  while (count != 0) {
    dest.write(buffer, count);
    decrypt.read(buffer, buffer_size);
    count = decrypt.gcount();
  }
  
  return (!decrypt.fail() || decrypt.eof()) && (!dest.fail());
}

#endif // HAVE_OPENSSL
