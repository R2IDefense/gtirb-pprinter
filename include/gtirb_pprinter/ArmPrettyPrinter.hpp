//===- ArmPrettyPrinter.h ---------------------------------------*- C++ -*-===//
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
#ifndef GTIRB_PP_ARM_PRINTER_H
#define GTIRB_PP_ARM_PRINTER_H

#include "ElfPrettyPrinter.hpp"

namespace gtirb_pprint {

class ArmSyntax : public ElfSyntax {
public:
  const std::string& attributePrefix() const override {
    return AttributePrefix;
  }

  SyntaxAlignmentStyle alignmentStyle() const override {
    return SyntaxAlignmentZeros;
  };

  const std::string& byteData() const override { return ByteDirective; }
  const std::string& longData() const override { return WordDirective; }
  const std::string& quadData() const override { return QuadDirective; }
  const std::string& wordData() const override { return HWordDirective; }

  // Formatting helpers
  std::string formatSectionName(const std::string& x) const override;
  std::string formatFunctionName(const std::string& x) const override;
  std::string formatSymbolName(const std::string& x) const override;

private:
  const std::string AttributePrefix{"%"};
};

class ArmPrettyPrinter : public ElfPrettyPrinter {
public:
  ArmPrettyPrinter(gtirb::Context& context, const gtirb::Module& module,
                   const ArmSyntax& syntax, const PrintingPolicy& policy);

protected:
  const ArmSyntax& armSyntax;

  void fixupInstruction(cs_insn& inst) override;
  std::string getRegisterName(unsigned int reg) const override;
  void printHeader(std::ostream& os) override;
  void setDecodeMode(std::ostream& os, const gtirb::CodeBlock& x) override;
  void printBlockContents(std::ostream& os, const gtirb::CodeBlock& x,
                          uint64_t offset) override;
  void printInstruction(std::ostream& os, const gtirb::CodeBlock& block,
                        const cs_insn& inst,
                        const gtirb::Offset& offset) override;

  void printOperandList(std::ostream& os, const gtirb::CodeBlock& block,
                        const cs_insn& inst) override;
  void printOperand(std::ostream& os, const gtirb::CodeBlock& block,
                    const cs_insn& inst, uint64_t index) override;
  void printOpRegdirect(std::ostream& os, const cs_insn& inst,
                        uint64_t index) override;
  void printOpImmediate(std::ostream& os,
                        const gtirb::SymbolicExpression* symbolic,
                        const cs_insn& inst, uint64_t index) override;
  void printOpIndirect(std::ostream& os,
                       const gtirb::SymbolicExpression* symbolic,
                       const cs_insn& inst, uint64_t index) override;
  bool printSymbolReference(std::ostream& os,
                            const gtirb::Symbol* symbol) override;
  void printSymExprSuffix(std::ostream& OS, const gtirb::SymAttributeSet& Attrs,
                          bool IsNotBranch = false) override;
};

class ArmPrettyPrinterFactory : public ElfPrettyPrinterFactory {
public:
  ArmPrettyPrinterFactory();
  std::unique_ptr<PrettyPrinterBase>
  create(gtirb::Context& context, const gtirb::Module& module,
         const PrintingPolicy& policy) override;
};

} // namespace gtirb_pprint

#endif /* GTIRB_PP_ARM_PRINTER_H */
