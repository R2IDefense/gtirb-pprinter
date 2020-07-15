//===- file_utils.hpp ----------------------------------------------*- C++ ---//
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
#ifndef GTIRB_FILE_UTILS_H
#define GTIRB_FILE_UTILS_H

#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace gtirb_bprint {
/// Auxiliary class to make sure we delete the temporary assembly file at the
/// end
class TempFile {
  std::string name;
  std::ofstream fileStream;

public:
  TempFile();
  ~TempFile();

  bool isOpen() const { return static_cast<bool>(fileStream); }
  void close() { fileStream.close(); }

  operator const std::ofstream&() const { return fileStream; }
  operator std::ofstream&() { return fileStream; }
  const std::string& fileName() const { return name; }
};

// Helper functions to resolve symlinks and get a real path to a file.
std::optional<std::string> resolve_regular_file_path(const std::string& path);
std::optional<std::string>
resolve_regular_file_path(const std::string& path, const std::string& fileName);

// Helper function to execute a process with arguments; will search for the
// given tool on PATH automatically. If foundTool is non-null, it will be set
// to true if the tool can be found and false otherwise. This helps to
// distinguish between the tool failing and the tool not being found.
bool execute(const std::string& tool, const std::vector<std::string>& args,
             bool* foundTool = nullptr);

} // namespace gtirb_bprint
#endif /* GTIRB_FILE_UTILS_H */
