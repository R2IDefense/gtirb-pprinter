import gtirb

from gtirb_helpers import add_code_block, add_text_section, create_test_module
from pprinter_helpers import run_asm_pprinter, PPrinterTest


class IntelInstructionsTest(PPrinterTest):
    def test_unpack_dd(self):
        # This test ensures that we do not regress on the following issue:
        # git.grammatech.com/rewriting/gtirb-pprinter/-/merge_requests/439
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.ELF, isa=gtirb.Module.ISA.X64
        )
        s, bi = add_text_section(m)

        # vpgatherdd ymm1,DWORD PTR [r8+ymm5*4],ymm6
        add_code_block(bi, b"\xC4\xC2\x4D\x90\x0c\xA8")

        # We're specifically trying to see if the middle operand is a
        # DWORD PTR or a YMMWORD PTR.
        asm = run_asm_pprinter(ir, ["--syntax=intel"])
        self.assertIn("DWORD PTR", asm)

    def test_unpack_qd(self):
        # This test ensures that we do not regress on the following issue:
        # git.grammatech.com/rewriting/gtirb-pprinter/-/merge_requests/439
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.ELF, isa=gtirb.Module.ISA.X64
        )
        s, bi = add_text_section(m)

        # vpgatherqd xmm1,DWORD PTR [r8+xmm5*4],xmm6
        add_code_block(bi, b"\xC4\xC2\x49\x91\x0c\xa8")

        # We're specifically trying to see if the middle operand is a
        # DWORD PTR or a YMMWORD PTR.
        asm = run_asm_pprinter(ir, ["--syntax=intel"])
        self.assertIn("DWORD PTR", asm)

    def test_unpack_dq(self):
        # This test ensures that we do not regress on the following issue:
        # git.grammatech.com/rewriting/gtirb-pprinter/-/merge_requests/439
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.ELF, isa=gtirb.Module.ISA.X64
        )
        s, bi = add_text_section(m)

        # vpgatherdq ymm7,QWORD PTR [rcx+xmm4*1],xmm8
        add_code_block(bi, b"\xC4\xE2\xBD\x90\x3c\x21")

        # We're specifically trying to see if the middle operand is a
        # QWORD PTR or a YMMWORD PTR.
        asm = run_asm_pprinter(ir, ["--syntax=intel"])
        self.assertIn("QWORD PTR", asm)

    def test_unpack_qq(self):
        # This test ensures that we do not regress on the following issue:
        # git.grammatech.com/rewriting/gtirb-pprinter/-/merge_requests/439
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.ELF, isa=gtirb.Module.ISA.X64
        )
        s, bi = add_text_section(m)

        # vpgatherqq ymm12,QWORD PTR [r13+ymm10*1],xmm11
        add_code_block(bi, b"\xC4\x02\xA5\x91\x64\x15\x00")

        # We're specifically trying to see if the middle operand is a
        # QWORD PTR or a YMMWORD PTR.
        asm = run_asm_pprinter(ir, ["--syntax=intel"])
        self.assertIn("QWORD PTR", asm)
