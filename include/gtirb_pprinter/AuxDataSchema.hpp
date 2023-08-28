//===- AuxDataSchema.hpp ----------------------------------------*- C++ -*-===//
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
#ifndef GTIRB_PPRINTER_AUXDATASCHEMA_HPP
#define GTIRB_PPRINTER_AUXDATASCHEMA_HPP

#include <gtirb/gtirb.hpp>
#include <map>
#include <string>
#include <tuple>
#include <vector>

/// \file AuxDataSchema.hpp
/// \ingroup AUXDATA_GROUP
/// \brief AuxData types used by gtirb_pprinter that are not sanctioned.
/// \see AUXDATA_GROUP

namespace gtirb {
namespace schema {

/// \brief Auxiliary data covering data object encoding specifiers.
struct Encodings {
  static constexpr const char* Name = "encodings";
  typedef std::map<gtirb::UUID, std::string> Type;
};

/// \brief Auxiliary data covering section properties.
struct SectionProperties {
  static constexpr const char* Name = "sectionProperties";
  typedef std::map<gtirb::UUID, std::tuple<uint64_t, uint64_t>> Type;
};

/// \brief Auxiliary data covering cfi directives.
struct CfiDirectives {
  static constexpr const char* Name = "cfiDirectives";
  typedef std::map<
      gtirb::Offset,
      std::vector<std::tuple<std::string, std::vector<int64_t>, gtirb::UUID>>>
      Type;
};

/// \brief Auxiliary data that includes names of necessary libraries.
struct Libraries {
  static constexpr const char* Name = "libraries";
  typedef std::vector<std::string> Type;
};

/// \brief Auxiliary data that includes names of necessary library paths.
struct LibraryPaths {
  static constexpr const char* Name = "libraryPaths";
  typedef std::vector<std::string> Type;
};

/// \brief Auxiliary data for Windows data directories.
struct DataDirectories {
  static constexpr const char* Name = "dataDirectories";
  typedef std::vector<std::tuple<std::string, uint64_t, uint64_t>> Type;
};

/// \brief Auxiliary data for the UUIDs of imported symbols in a PE file.
struct PeImportedSymbols {
  static constexpr const char* Name = "peImportedSymbols";
  typedef std::vector<gtirb::UUID> Type;
};

/// \brief Auxiliary data for the UUIDs of exported symbols in a PE file.
struct PeExportedSymbols {
  static constexpr const char* Name = "peExportedSymbols";
  typedef std::vector<gtirb::UUID> Type;
};

/// \brief Auxiliary data for the UUIDs of PE exception handlers.
struct PeSafeExceptionHandlers {
  static constexpr const char* Name = "peSafeExceptionHandlers";
  typedef std::set<gtirb::UUID> Type;
};

/// \brief Auxiliary data for extra symbol info.
struct ElfSymbolInfo {
  static constexpr const char* Name = "elfSymbolInfo";
  typedef std::map<gtirb::UUID, std::tuple<uint64_t, std::string, std::string,
                                           std::string, uint64_t>>
      Type;
};

/// \brief Auxiliary data that stores the size of symbolic expressions.
struct SymbolicExpressionSizes {
  static constexpr const char* Name = "symbolicExpressionSizes";
  typedef std::map<gtirb::Offset, uint64_t> Type;
};

/// \brief Auxiliary data describing a binary's type.
struct BinaryType {
  static constexpr const char* Name = "binaryType";
  typedef std::vector<std::string> Type;
};

/// \brief Auxiliary data representing the export table of a PE file.
struct ExportEntries {
  static constexpr const char* Name = "peExportEntries";
  // Tuples of the form {Address, Ordinal, Name}.
  typedef std::vector<std::tuple<uint64_t, int64_t, std::string>> Type;
};

/// \brief Auxiliary data representing the import table of a PE file.
struct ImportEntries {
  static constexpr const char* Name = "peImportEntries";
  // Tuples of the form {Iat_address, Ordinal, Function, Library}.
  typedef std::vector<std::tuple<uint64_t, int64_t, std::string, std::string>>
      Type;
};

// \brief List on PE Resources in the form <header, data_offset, data_length
struct PEResources {
  static constexpr const char* Name = "peResources";
  typedef std::vector<std::tuple<std::vector<uint8_t>, gtirb::Offset, uint64_t>>
      Type;
};

/// ElfSymbolTabIdxInfo is a vector of tuples of the form {Name, Index}.
using ElfSymbolTabIdxInfoEntry = std::vector<std::tuple<std::string, uint64_t>>;

/// \brief Auxiliary data for extra symbol info.
struct ElfSymbolTabIdxInfo {
  static constexpr const char* Name = "elfSymbolTabIdxInfo";
  typedef std::map<gtirb::UUID, ElfSymbolTabIdxInfoEntry> Type;
};

} // namespace schema

namespace provisional_schema {

// Type descriptors used by gtirb-types
typedef std::variant<
    uint64_t,                                          // Unknown{Width}
    std::tuple<uint8_t>,                               // Bool
    std::tuple<int8_t, uint64_t>,                      // Int{Signed, Width}
    uint64_t,                                          // Char{Width}
    uint64_t,                                          // Float{Width}
    std::tuple<gtirb::UUID, std::vector<gtirb::UUID>>, // Function{ReturnType,
                                                       // ArgumentTypes}
    gtirb::UUID,                                       // Pointer{Type}
    std::tuple<gtirb::UUID, uint64_t>,                 // Array {Type, Size}
    std::tuple<uint64_t,
               std::vector<std::tuple<uint64_t, gtirb::UUID>>>, // Struct {Size,
                                                                // Fields}
    std::tuple<uint8_t>,                                        // Void
    gtirb::UUID>                                                // Alias {Type}
    GtirbType;

struct TypeTable {
  // Map assigning each type used a UUID
  static constexpr const char* Name = "typeTable";
  typedef std::map<gtirb::UUID, GtirbType> Type;
};

struct PrototypeTable {
  // Map from UUIDs of functions to UUIDs for their types in typeTable
  static constexpr const char* Name = "prototypeTable";
  typedef std::map<gtirb::UUID, gtirb::UUID> Type;
};

} // namespace provisional_schema

} // namespace gtirb

#endif // GTIRB_PPRINTER_AUXDATASCHEMA_HPP
