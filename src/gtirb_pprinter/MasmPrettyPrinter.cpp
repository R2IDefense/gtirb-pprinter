//===- MasmPrinter.cpp ------------------------------------------*- C++ -*-===//
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

#include "MasmPrettyPrinter.hpp"

#include "AuxDataSchema.hpp"
#include "regex"
#include "string_utils.hpp"
#include <boost/algorithm/string/replace.hpp>

namespace gtirb_pprint {

std::string MasmSyntax::formatSectionName(const std::string& x) const {
  std::string name(x);
  if (name[0] == '.')
    name[0] = '_';
  return ascii_str_toupper(name);
}

std::string MasmSyntax::formatFunctionName(const std::string& x) const {
  std::string name(x);
  if (name[0] == '.')
    name[0] = '$';
  return name;
}

std::string MasmSyntax::formatSymbolName(const std::string& x) const {
  std::string name = avoidRegNameConflicts(x);
  if (name[0] == '.')
    name[0] = '$';
  return name;
}

MasmPrettyPrinter::MasmPrettyPrinter(gtirb::Context& context_,
                                     gtirb::Module& module_,
                                     const MasmSyntax& syntax_,
                                     const PrintingPolicy& policy_)
    : PePrettyPrinter(context_, module_, syntax_, policy_),
      masmSyntax(syntax_) {

  BaseAddress = module.getPreferredAddr();
  if (auto It = module.findSymbols("__ImageBase"); !It.empty()) {
    ImageBase = &*It.begin();
    ImageBase->setReferent(module.addProxyBlock(context));
  }

  if (gtirb::CodeBlock* Block = module.getEntryPoint(); Block->getAddress()) {
    auto* EntryPoint =
        gtirb::Symbol::Create(context, *(Block->getAddress()), "__EntryPoint");
    EntryPoint->setReferent<gtirb::CodeBlock>(Block);
    module.addSymbol(EntryPoint);
    Exports.insert(EntryPoint->getUUID());
  }

  const auto* ImportedSymbols =
      module.getAuxData<gtirb::schema::PeImportedSymbols>();
  if (ImportedSymbols) {
    for (const auto& UUID : *ImportedSymbols) {
      Imports.insert(UUID);
    }
  }

  const auto* ExportedSymbols =
      module.getAuxData<gtirb::schema::PeExportedSymbols>();
  if (ExportedSymbols) {
    for (const auto& UUID : *ExportedSymbols) {
      Exports.insert(UUID);
    }
  }
}

void MasmPrettyPrinter::printIncludes(std::ostream& os) {
  const auto* libraries = module.getAuxData<gtirb::schema::Libraries>();
  if (libraries) {
    for (const auto& library : *libraries) {
      // Include replacement libs.
      bool replaced = false;
      for (const auto& [pattern, replacements] : dllLibraries) {
        std::regex re(pattern, std::regex::icase);
        if (std::regex_match(library, re)) {
          for (const auto& lib : replacements) {
            os << "INCLUDELIB " << lib << '\n';
          }
          replaced = true;
        }
      }
      // Include DLL as LIB.
      if (!replaced) {
        os << "INCLUDELIB "
           << boost::ireplace_last_copy(library, ".dll", ".lib") << '\n';
      }
    }
  }
  os << '\n';
}

void MasmPrettyPrinter::printExterns(std::ostream& os) {
  // Declare EXTERN symbols
  if (const auto* symbolForwarding =
          module.getAuxData<gtirb::schema::SymbolForwarding>()) {
    std::set<std::string> externs;
    for (auto& forward : *symbolForwarding) {
      if (const auto* symbol = dyn_cast_or_null<gtirb::Symbol>(
              gtirb::Node::getByUUID(context, forward.second))) {
        externs.insert(getSymbolName(*symbol));
      }
    }
    for (auto& name : externs) {
      os << masmSyntax.extrn() << ' ' << name << ":PROC\n";
    }
  }

  os << '\n' << masmSyntax.extrn() << " __ImageBase:BYTE\n";

  os << '\n';
}

void MasmPrettyPrinter::printHeader(std::ostream& os) {
  printIncludes(os);
  printExterns(os);
}

void MasmPrettyPrinter::printSectionHeaderDirective(
    std::ostream& os, const gtirb::Section& section) {
  std::string section_name = syntax.formatSectionName(section.getName());
  os << section_name << ' ' << syntax.section();
}
void MasmPrettyPrinter::printSectionProperties(std::ostream& os,
                                               const gtirb::Section& section) {
  const auto* peSectionProperties =
      module.getAuxData<gtirb::schema::PeSectionProperties>();
  if (!peSectionProperties)
    return;
  const auto sectionProperties = peSectionProperties->find(section.getUUID());
  if (sectionProperties == peSectionProperties->end())
    return;
  uint64_t flags = sectionProperties->second;

  if (flags & IMAGE_SCN_MEM_READ)
    os << " READ";
  if (flags & IMAGE_SCN_MEM_WRITE)
    os << " WRITE";
  if (flags & IMAGE_SCN_MEM_EXECUTE)
    os << " EXECUTE";
  if (flags & IMAGE_SCN_MEM_SHARED)
    os << " SHARED";
  if (flags & IMAGE_SCN_MEM_NOT_PAGED)
    os << " NOPAGE";
  if (flags & IMAGE_SCN_MEM_NOT_CACHED)
    os << " NOCACHE";
  if (flags & IMAGE_SCN_MEM_DISCARDABLE)
    os << " DISCARD";
  if (flags & IMAGE_SCN_CNT_CODE)
    os << " 'CODE'";
  if (flags & IMAGE_SCN_CNT_INITIALIZED_DATA)
    os << " 'DATA'";
};

void MasmPrettyPrinter::printSectionFooterDirective(
    std::ostream& os, const gtirb::Section& section) {
  std::string section_name = syntax.formatSectionName(section.getName());

  // Special .CODE .DATA and .DATA? directives do not need footers.
  if (section_name == "_TEXT" || section_name == "_DATA" ||
      section_name == "_BSS") {
    os << syntax.comment() << ' ' << section_name << ' ' << masmSyntax.ends()
       << '\n';
    return;
  }

  os << section_name << ' ' << masmSyntax.ends() << '\n';
}

void MasmPrettyPrinter::printFunctionHeader(std::ostream& /* os */,
                                            gtirb::Addr /* addr */) {
  // TODO
}

void MasmPrettyPrinter::printFunctionFooter(std::ostream& /* os */,
                                            gtirb::Addr /* addr */) {
  // TODO
}

void MasmPrettyPrinter::fixupInstruction(cs_insn& inst) {
  cs_x86& Detail = inst.detail->x86;

  // Change GAS-specific MOVABS opcode to equivalent MOV opcode.
  if (inst.id == X86_INS_MOVABS) {
    std::string_view mnemonic(inst.mnemonic);
    if (mnemonic.size() > 3) {
      inst.mnemonic[3] = '\0';
    }
  }

  // PBLENDVB/BLENDVPS have an implicit third argument (XMM0) required by MASM
  if (inst.id == X86_INS_PBLENDVB || inst.id == X86_INS_BLENDVPS) {
    if (Detail.op_count == 2) {
      Detail.op_count = 3;
      cs_x86_op& Op = Detail.operands[2];
      Op.type = X86_OP_REG;
      Op.reg = X86_REG_XMM0;
    }
  }

  PrettyPrinterBase::fixupInstruction(inst);
}

void MasmPrettyPrinter::printSymbolHeader(std::ostream& os,
                                          const gtirb::Symbol& symbol) {
  // Print public definitions
  if (Exports.count(symbol.getUUID())) {
    if (symbol.getReferent<gtirb::DataBlock>()) {
      os << '\n' << syntax.global() << ' ' << getSymbolName(symbol) << '\n';
    } else {
      os << symbol.getName() << ' ' << masmSyntax.proc() << " EXPORT\n"
         << symbol.getName() << ' ' << masmSyntax.endp() << '\n';
    }
  }
}

void MasmPrettyPrinter::printSymbolFooter(std::ostream& os,
                                          const gtirb::Symbol& symbol) {
  // Print name for data block
  if (symbol.getReferent<gtirb::DataBlock>()) {
    os << 'N' << getSymbolName(symbol).substr(2);
  }
}

void MasmPrettyPrinter::printSymbolDefinition(std::ostream& os,
                                              const gtirb::Symbol& symbol) {
  auto ea = *symbol.getAddress();
  if (ea == gtirb::Addr(0)) {
    return;
  }

  printSymbolHeader(os, symbol);
  if (symbol.getReferent<gtirb::DataBlock>() ||
      Exports.count(symbol.getUUID()) == 0) {
    PrettyPrinterBase::printSymbolDefinition(os, symbol);
  }
  printSymbolFooter(os, symbol);
}

void MasmPrettyPrinter::printSymbolDefinitionRelativeToPC(
    std::ostream& os, const gtirb::Symbol& symbol, gtirb::Addr pc) {
  auto symAddr = *symbol.getAddress();
  if (symAddr == gtirb::Addr(0)) {
    return;
  }

  printSymbolHeader(os, symbol);

  os << getSymbolName(symbol) << " = " << syntax.programCounter();
  if (symAddr > pc) {
    os << " + " << (symAddr - pc);
  } else if (symAddr < pc) {
    os << " - " << (pc - symAddr);
  }
  os << "\n";

  printSymbolFooter(os, symbol);
}

void MasmPrettyPrinter::printIntegralSymbol(std::ostream& os,
                                            const gtirb::Symbol& symbol) {
  if (*symbol.getAddress() == gtirb::Addr(0)) {
    return;
  }

  printSymbolHeader(os, symbol);

  os << getSymbolName(symbol) << " = " << *symbol.getAddress() << '\n';

  printSymbolFooter(os, symbol);
}

void MasmPrettyPrinter::printOpRegdirect(std::ostream& os,
                                         const cs_insn& /*inst*/,
                                         const cs_x86_op& op) {
  assert(op.type == X86_OP_REG &&
         "printOpRegdirect called without a register operand");
  os << getRegisterName(op.reg);
}

void MasmPrettyPrinter::printOpImmediate(
    std::ostream& os, const gtirb::SymbolicExpression* symbolic,
    const cs_insn& inst, uint64_t index) {
  const cs_x86_op& op = inst.detail->x86.operands[index];
  assert(op.type == X86_OP_IMM &&
         "printOpImmediate called without an immediate operand");

  bool is_call = cs_insn_group(this->csHandle, &inst, CS_GRP_CALL);
  bool is_jump = cs_insn_group(this->csHandle, &inst, CS_GRP_JUMP);

  if (const gtirb::SymAddrConst* s = this->getSymbolicImmediate(symbolic)) {
    // The operand is symbolic.

    // Symbols for skipped addresses degrade to literals.
    if (!is_call && !is_jump && !shouldSkip(*s->Sym))
      os << masmSyntax.offset() << ' ';

    printSymbolicExpression(os, s, !is_call && !is_jump);
  } else {
    // The operand is just a number.
    os << op.imm;
  }
}

void MasmPrettyPrinter::printOpIndirect(
    std::ostream& os, const gtirb::SymbolicExpression* symbolic,
    const cs_insn& inst, uint64_t index) {
  const cs_x86& detail = inst.detail->x86;
  const cs_x86_op& op = detail.operands[index];
  assert(op.type == X86_OP_MEM &&
         "printOpIndirect called without a memory operand");
  bool first = true;

  // Replace indirect reference to EXTERN with direct reference.
  //   e.g.  call QWORD PTR [puts]
  //         call puts
  if (const auto* s = std::get_if<gtirb::SymAddrConst>(symbolic)) {
    std::optional<std::string> forwardedName =
        getForwardedSymbolName(s->Sym, true);
    if (forwardedName) {
      os << *forwardedName;
      return;
    }
  }

  uint64_t size = op.size;

  //////////////////////////////////////////////////////////////////////////////
  // Capstone incorrectly gives memory operands XMMWORD size.
  if (inst.id == X86_INS_COMISD || inst.id == X86_INS_VCOMISD) {
    size = 8;
  }
  if (inst.id == X86_INS_COMISS) {
    size = 4;
  }
  //////////////////////////////////////////////////////////////////////////////

  if (std::optional<std::string> sizeName = syntax.getSizeName(size * 8))
    os << *sizeName << " PTR ";

  if (op.mem.segment != X86_REG_INVALID)
    os << getRegisterName(op.mem.segment) << ':';

  os << '[';

  if (op.mem.base != X86_REG_INVALID && op.mem.base != X86_REG_RIP) {
    first = false;
    os << getRegisterName(op.mem.base);
  }

  if (op.mem.base == X86_REG_RIP && symbolic == nullptr) {
    if (gtirb::Addr(inst.address + inst.size + op.mem.disp) == BaseAddress) {
      os << "__ImageBase]";
      return;
    }
  }

  if (op.mem.index != X86_REG_INVALID) {
    if (!first)
      os << '+';
    first = false;
    os << getRegisterName(op.mem.index) << '*' << std::to_string(op.mem.scale);
  }

  if (const auto* s = std::get_if<gtirb::SymAddrConst>(symbolic)) {
    if (!first)
      os << '+';

    printSymbolicExpression(os, s, false);
  } else if (const auto* rel = std::get_if<gtirb::SymAddrAddr>(symbolic)) {
    if (std::optional<gtirb::Addr> Addr = rel->Sym1->getAddress(); Addr) {
      os << "+(" << masmSyntax.imagerel() << ' ' << 'N'
         << getSymbolName(*rel->Sym1).substr(2) << ")";
    }
  } else {
    printAddend(os, op.mem.disp, first);
  }
  os << ']';
}

void MasmPrettyPrinter::printSymbolicExpression(
    std::ostream& os, const gtirb::SymAddrConst* sexpr, bool inData) {
  PrettyPrinterBase::printSymbolicExpression(os, sexpr, inData);
}

void MasmPrettyPrinter::printSymbolicExpression(std::ostream& os,
                                                const gtirb::SymAddrAddr* sexpr,
                                                bool inData) {
  if (inData && sexpr->Sym2 == ImageBase) {
    os << masmSyntax.imagerel() << ' ';
    printSymbolReference(os, sexpr->Sym1, inData);
    return;
  }

  PrettyPrinterBase::printSymbolicExpression(os, sexpr, inData);
}

void MasmPrettyPrinter::printByte(std::ostream& os, std::byte byte) {
  // Byte constants must start with a number for the MASM assembler.
  os << syntax.byteData() << " 0" << std::hex << std::setfill('0')
     << std::setw(2) << static_cast<uint32_t>(byte) << 'H' << std::dec << '\n';
}

void MasmPrettyPrinter::printZeroDataBlock(std::ostream& os,
                                           const gtirb::DataBlock& dataObject,
                                           uint64_t offset) {
  os << syntax.tab();
  os << "DB " << (dataObject.getSize() - offset) << " DUP(0)" << '\n';
}

void MasmPrettyPrinter::printString(std::ostream& os, const gtirb::DataBlock& x,
                                    uint64_t offset) {

  std::string Chunk{""};

  auto Range = x.bytes<uint8_t>();
  for (uint8_t b :
       boost::make_iterator_range(Range.begin() + offset, Range.end())) {
    // NOTE: MASM only supports strings smaller than 256 bytes.
    //  and  MASM only supports statements with 50 comma-separated items.
    if (Chunk.size() >= 64) {
      boost::replace_all(Chunk, "'", "''");
      os << syntax.tab() << syntax.string() << " '" << Chunk << "'\n";
      Chunk.clear();
    }

    // Aggegrate printable characters
    if (std::isprint(b)) {
      Chunk.append(1, b);
      continue;
    }

    // Found non-printable character, output previous chunk and print byte
    if (!Chunk.empty()) {
      boost::replace_all(Chunk, "'", "''");
      os << syntax.tab() << syntax.string() << " '" << Chunk << "'\n";
      Chunk.clear();
    }
    os << syntax.tab();
    printByte(os, static_cast<std::byte>(b));
  }
}

void MasmPrettyPrinter::printFooter(std::ostream& os) {
  os << '\n' << masmSyntax.end();
}

const PrintingPolicy& MasmPrettyPrinterFactory::defaultPrintingPolicy() const {
  return MasmPrettyPrinter::defaultPrintingPolicy();
}

std::unique_ptr<PrettyPrinterBase>
MasmPrettyPrinterFactory::create(gtirb::Context& context, gtirb::Module& module,
                                 const PrintingPolicy& policy) {
  static const MasmSyntax syntax{};
  return std::make_unique<MasmPrettyPrinter>(context, module, syntax, policy);
}

volatile bool MasmPrettyPrinter::registered = registerPrinter(
    {"pe"}, {"masm"}, std::make_shared<MasmPrettyPrinterFactory>(), true);

} // namespace gtirb_pprint
