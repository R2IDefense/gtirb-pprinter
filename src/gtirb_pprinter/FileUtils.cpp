//===- FileUtils.cpp -------------------------------------------*- C++ -*-===//
//
//  Copyright (C) 2020 GrammaTech, Inc.
//
//  This code is licensed under the MIT license. See the LICENSE file in the
//  project root for license terms.
//
//  This project is sponsored by the Office of Naval Research, One Liberty
//  Center, 875 N. Randolph Street, Arlington, VA 22203 under contract #
//  N68335-17-C-0700.  The content of the information does not necessarily
//  reflect the position or policy of the Government and no official
//  endorsement should be inferred.
//
//===----------------------------------------------------------------------===//
#include "FileUtils.hpp"
#include "driver/Logger.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wc++11-compat"
#pragma GCC diagnostic ignored "-Wpessimizing-move"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4456) // variable shadowing warning
#endif                          // __GNUC__
#include <boost/filesystem.hpp>
#include <boost/process/search_path.hpp>
#include <boost/process/system.hpp>
#include <iostream>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif // __GNUC__

namespace fs = boost::filesystem;
namespace bp = boost::process;

namespace gtirb_bprint {
TempFile::TempFile(const std::string extension) {
  // FIXME: this has TOCTOU issues.
#ifdef _WIN32
  std::string TmpFileName;
  std::FILE* F = nullptr;
  while (!F) {
    TmpFileName = std::tmpnam(nullptr);
    TmpFileName += extension;
    F = fopen(TmpFileName.c_str(), "wx");
  }
  fclose(F);
#else
  std::string TmpFileName = "/tmp/fileXXXXXX";
  TmpFileName += extension;
  ::close(mkstemps(TmpFileName.data(), extension.length())); // Create tmp file
#endif // _WIN32
  Name = TmpFileName;
  FileStream.open(Name);
}

TempFile::TempFile(TempFile&& Other)
    : Name(std::move(Other.Name)), FileStream(std::move(Other.FileStream)) {
  Other.Empty = true;
}

TempFile::~TempFile() {
  if (isOpen()) {
    LOG_WARNING << "Removing open temporary file: " << Name << "\n";
    close();
  }
  if (!Empty && !Name.empty()) {
    boost::system::error_code ErrorCode;
    fs::remove(Name, ErrorCode);
    if (ErrorCode.value()) {
      LOG_ERROR << "Failed to remove temporary file: " << Name << "\n";
      LOG_ERROR << ErrorCode.message();
    }
  }
}

TempDir::TempDir() : Name(), Errno(0) {
#ifdef _WIN32
  assert(0 && "Unimplemented!");
#else
  std::string TmpDirName = "/tmp/dirXXXXXX";
  if (mkdtemp(TmpDirName.data())) {
    Name = TmpDirName;
  } else {
    Errno = errno;
  }
#endif
}

TempDir::~TempDir() {
  if (created()) {
    fs::remove_all(Name);
  }
}

std::string replaceExtension(const std::string path,
                             const std::string new_ext) {
  return fs::path(path).stem().string() + new_ext;
}

std::optional<std::string> resolveRegularFilePath(const std::string& path) {
  // Check that if path is a symbolic link, it eventually leads to a regular
  // file.
  fs::path resolvedFilePath(path);
  while (fs::is_symlink(resolvedFilePath)) {
    resolvedFilePath = fs::read_symlink(resolvedFilePath);
  }
  if (fs::is_regular_file(resolvedFilePath)) {
    return resolvedFilePath.string();
  }
  return std::nullopt;
}

std::optional<std::string> resolveRegularFilePath(const std::string& path,
                                                  const std::string& fileName) {
  fs::path filePath(path);
  filePath.append(fileName);
  return resolveRegularFilePath(filePath.string());
}

std::optional<int> execute(const std::string& Tool,
                           const std::vector<std::string>& Args) {
  fs::path Path = fs::is_regular_file(Tool) ? Tool : bp::search_path(Tool);
  if (Path.empty()) {
    return std::nullopt;
  }
  return bp::system(Path, Args);
}
} // namespace gtirb_bprint
