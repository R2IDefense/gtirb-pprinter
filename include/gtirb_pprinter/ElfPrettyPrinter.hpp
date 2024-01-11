//===- ElfPrettyPrinter.hpp -------------------------------------*- C++ -*-===//
//
//  Copyright (C) 2019 GrammaTech, Inc.
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
#ifndef GTIRB_PP_ELF_PRINTER_H
#define GTIRB_PP_ELF_PRINTER_H

#include "AuxDataUtils.hpp"
#include "PrettyPrinter.hpp"
#include <set>

namespace gtirb_pprint {

class DEBLOAT_PRETTYPRINTER_EXPORT_API ElfSyntax : public Syntax {
public:
  const std::string& comment() const override { return CommentStyle; }

  const std::string& string() const override { return StringDirective; }
  virtual const std::string& ascii() const { return AsciiDirective; }

  virtual const std::string& attributePrefix() const { return AttributePrefix; }

  const std::string& byteData() const override { return ByteDirective; }
  const std::string& longData() const override { return LongDirective; }
  const std::string& quadData() const override { return QuadDirective; }
  const std::string& wordData() const override { return WordDirective; }
  const std::string& rvaData() const {return RvaDirective;}

  const std::string& text() const override { return TextDirective; }
  const std::string& data() const override { return DataDirective; }
  const std::string& bss() const override { return BssDirective; }

  const std::string& section() const override { return SectionDirective; }
  const std::string& global() const override { return GlobalDirective; }
  const std::string& align() const override { return AlignDirective; }

  const std::string& programCounter() const override {
    return ProgramCounterName;
  }

  const std::string& type() const { return TypeDirective; }
  const std::string& weak() const { return WeakDirective; }
  const std::string& set() const { return SetDirective; }
  const std::string& hidden() const { return HiddenDirective; }
  const std::string& protected_() const { return ProtectedDirective; }
  const std::string& internal() const { return InternalDirective; }
  const std::string& uleb128() const { return ULEB128Directive; }
  const std::string& sleb128() const { return SLEB128Directive; }
  const std::string& symVer() const { return SymVerDirective; }
  const std::string& symSize() const { return SymSizeDirective; }

private:
  const std::string CommentStyle{"#"};

  const std::string StringDirective{".string"};
  const std::string AsciiDirective{".ascii"};

  const std::string AttributePrefix{"@"};

  const std::string TextDirective{".text"};
  const std::string DataDirective{".data"};
  const std::string BssDirective{".bss"};

  const std::string SectionDirective{".section"};
  const std::string GlobalDirective{".globl"};
  const std::string AlignDirective{".align"};

  const std::string ProgramCounterName{"."};

  const std::string TypeDirective{".type"};
  const std::string WeakDirective{".weak"};
  const std::string SetDirective{".set"};
  const std::string HiddenDirective{".hidden"};
  const std::string ProtectedDirective{".protected"};
  const std::string InternalDirective{".internal"};
  const std::string ULEB128Directive{".uleb128"};
  const std::string SLEB128Directive{".sleb128"};
  const std::string SymSizeDirective{".size"};
  const std::string SymVerDirective{".symver"};

protected:
  const std::string ByteDirective{".byte"};
  const std::string HWordDirective{".hword"};
  const std::string LongDirective{".long"};
  const std::string QuadDirective{".quad"};
  const std::string WordDirective{".word"};
  const std::string RvaDirective{".rva"};
};

class DEBLOAT_PRETTYPRINTER_EXPORT_API ElfPrettyPrinter
    : public PrettyPrinterBase {
public:
  ElfPrettyPrinter(gtirb::Context& context, const gtirb::Module& module,
                   const ElfSyntax& syntax, const PrintingPolicy& policy);

protected:
  const ElfSyntax& elfSyntax;

  void printInstruction(std::ostream& os, const gtirb::CodeBlock& block,
                        const cs_insn& inst,
                        const gtirb::Offset& offset) override;

  void printFooter(std::ostream& os) override;

  void printSectionHeaderDirective(std::ostream& os,
                                   const gtirb::Section& section) override;
  void printSectionProperties(std::ostream& os,
                              const gtirb::Section& section) override;
  void printSectionFooterDirective(std::ostream& os,
                                   const gtirb::Section& addr) override;

  /** Print the `.size FunctionSymbol, . - FunctionSymbol` label that defines
   * the size of the function symbol. */
  void printFunctionEnd(std::ostream& OS,
                        const gtirb::Symbol& FunctionSymbol) override;

  void printByte(std::ostream& os, std::byte byte) override;

  void printSymExprSuffix(std::ostream& OS, const gtirb::SymAttributeSet& Attrs,
                          bool IsNotBranch) override;

  void printSymbolDefinition(std::ostream& os,
                             const gtirb::Symbol& symbol) override;
  void printSymbolDefinitionRelativeToPC(std::ostream& os,
                                         const gtirb::Symbol& symbol,
                                         gtirb::Addr pc) override;
  void printIntegralSymbols(std::ostream& os) override;
  void printIntegralSymbol(std::ostream& os,
                           const gtirb::Symbol& symbol) override;
  void printUndefinedSymbol(std::ostream& os,
                            const gtirb::Symbol& symbol) override;

  void printSymbolicDataType(
      std::ostream& os,
      const gtirb::ByteInterval::ConstSymbolicExpressionElement& SEE,
      uint64_t Size, std::optional<std::string> Type) override;

  virtual void printHeader(std::ostream& /*os*/) override{};

  virtual void printSymbolHeader(std::ostream& os, const gtirb::Symbol& symbol);

  void printSymbolType(std::ostream& os, std::string& Name,
                       const aux_data::ElfSymbolInfo& SymbolInfo);

  /** Print .size directives for OBJECT and TLS symbols. */
  void printSymbolSize(std::ostream& os, std::string& Name,
                       const aux_data::ElfSymbolInfo& SymbolInfo);

  void printString(std::ostream& Stream, const gtirb::DataBlock& Block,
                   uint64_t Offset, bool NullTerminated = true) override;

  void skipVersionSymbols();

  std::optional<uint64_t> getAlignment(const gtirb::CodeBlock& Block) override;
  const gtirb::Symbol* getImageBase() {return ImageBase;}

  void addRelativeSymbol(gtirb::Symbol* sym)
  {
    rvaSymbols.insert(sym);
  }

  void printRvaSymbols(std::ostream &Stream);

private:
  bool TlsGdSequence = false;
  void computeFunctionAliases();
  const gtirb::Symbol* ImageBase;
  /* Keep track of all symbols that need to be IMAGEREL (windows) */
  std::set<gtirb::Symbol*> rvaSymbols;
};

class DEBLOAT_PRETTYPRINTER_EXPORT_API ElfPrettyPrinterFactory
    : public PrettyPrinterFactory {
public:
  ElfPrettyPrinterFactory();
  virtual ~ElfPrettyPrinterFactory() = default;

  bool isStaticBinary(const gtirb::Module& Module) const;

  /// Load the default printing policy.
  virtual const PrintingPolicy&
  defaultPrintingPolicy(const gtirb::Module& Module) const;
};

/**
Symbol is attached to the .plt, which can happen if the symbol has an address
in the ELF metadata. This seems to occur sometimes.

If the given symbol is such a symbol, return the section that it belongs to.
Otherwise, return null.
*/
const gtirb::Section* IsExternalPLTSym(const gtirb::Symbol& Sym);

} // namespace gtirb_pprint

#endif /* GTIRB_PP_ELF_PRINTER_H */
