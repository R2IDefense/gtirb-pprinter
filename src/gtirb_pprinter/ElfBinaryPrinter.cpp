//===- ElfBinaryPrinter.cpp -------------------------------------*- C++ -*-===//
//
//  Copyright (C) 2018 GrammaTech, Inc.
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
#include "ElfBinaryPrinter.hpp"

#include "AuxDataSchema.hpp"
#include "file_utils.hpp"
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace gtirb_bprint {

std::optional<std::string>
ElfBinaryPrinter::getInfixLibraryName(const std::string& library) const {
  std::regex libsoRegex("^lib(.*)\\.so.*");
  std::smatch m;
  if (std::regex_match(library, m, libsoRegex)) {
    return m.str(1);
  }
  return std::nullopt;
}

std::optional<std::string>
ElfBinaryPrinter::findLibrary(const std::string& library,
                              const std::vector<std::string>& paths) const {
  for (const auto& path : paths) {
    if (auto fp = resolve_regular_file_path(path, library))
      return fp;
  }
  return std::nullopt;
}

std::vector<std::string> ElfBinaryPrinter::buildCompilerArgs(
    std::string outputFilename, const std::vector<std::string>& asmPaths,
    const std::vector<std::string>& extraCompilerArgs,
    const std::vector<std::string>& userLibraryPaths, gtirb::IR& ir) const {
  std::vector<std::string> args;
  // Start constructing the compile arguments, of the form
  // -o <output_filename> fileAXADA.s
  args.emplace_back("-o");
  args.emplace_back(outputFilename);
  args.insert(args.end(), asmPaths.begin(), asmPaths.end());
  args.insert(args.end(), extraCompilerArgs.begin(), extraCompilerArgs.end());

  // collect all the library paths
  std::vector<std::string> allBinaryPaths = userLibraryPaths;

  for (gtirb::Module& module : ir.modules()) {

    if (const auto* binaryLibraryPaths =
            module.getAuxData<gtirb::schema::LibraryPaths>())
      allBinaryPaths.insert(allBinaryPaths.end(), binaryLibraryPaths->begin(),
                            binaryLibraryPaths->end());
  }
  // add needed libraries
  for (gtirb::Module& module : ir.modules()) {
    if (const auto* libraries = module.getAuxData<gtirb::schema::Libraries>()) {
      for (const auto& library : *libraries) {
        // if they match the lib*.so pattern we let the compiler look for them
        std::optional<std::string> infixLibraryName =
            getInfixLibraryName(library);
        if (infixLibraryName) {
          args.push_back("-l" + *infixLibraryName);
        } else {
          // otherwise we try to find them here
          if (std::optional<std::string> libraryLocation =
                  findLibrary(library, allBinaryPaths)) {
            args.push_back(*libraryLocation);
          } else {
            std::cerr << "ERROR: Could not find library " << library
                      << std::endl;
          }
        }
      }
    }
  }
  // add user library paths
  for (const auto& libraryPath : userLibraryPaths) {
    args.push_back("-L" + libraryPath);
  }
  // add binary library paths (add them to rpath as well)
  for (gtirb::Module& module : ir.modules()) {
    if (const auto* binaryLibraryPaths =
            module.getAuxData<gtirb::schema::LibraryPaths>()) {
      for (const auto& libraryPath : *binaryLibraryPaths) {
        args.push_back("-L" + libraryPath);
        args.push_back("-Wl,-rpath," + libraryPath);
      }
    }
  }
  // add pie or no pie depending on the binary type
  for (gtirb::Module& M : ir.modules()) {
    if (const auto* BinType = M.getAuxData<gtirb::schema::BinaryType>()) {
      // if DYN, pie. if EXEC, no-pie. if both, pie overrides no-pie. If none,
      // do not specify either argument.

      bool pie = false;
      bool noPie = false;

      for (const auto& BinTypeStr : *BinType) {
        if (BinTypeStr == "DYN") {
          pie = true;
          noPie = false;
        } else if (BinTypeStr == "EXEC") {
          if (!pie) {
            noPie = true;
            pie = false;
          }
        } else {
          assert(!"Unknown binary type!");
        }
      }

      if (pie) {
        args.push_back("-pie");
      }
      if (noPie) {
        args.push_back("-no-pie");
      }

      break;
    }
  }

  if (debug) {
    std::cout << "Compiler arguments: ";
    for (auto i : args)
      std::cout << i << ' ';
    std::cout << std::endl;
  }
  return args;
}

int ElfBinaryPrinter::link(const std::string& outputFilename,
                           const std::vector<std::string>& extraCompilerArgs,
                           const std::vector<std::string>& userLibraryPaths,
                           const gtirb_pprint::PrettyPrinter& pp,
                           gtirb::Context& ctx, gtirb::IR& ir) const {
  if (debug)
    std::cout << "Generating binary file" << std::endl;
  std::vector<TempFile> tempFiles;
  std::vector<std::string> tempFileNames;
  if (!prepareSources(ctx, ir, pp, tempFiles, tempFileNames)) {
    std::cerr << "ERROR: Could not write assembly into a temporary file.\n";
    return -1;
  }

  if (!execute(compiler,
               buildCompilerArgs(outputFilename, tempFileNames,
                                 extraCompilerArgs, userLibraryPaths, ir)))
    return -1;
  return 0;
}

} // namespace gtirb_bprint
