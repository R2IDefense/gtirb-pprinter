import unittest
import sys
import gtirb
import os

from gtirb_helpers import (
    add_byte_block,
    add_code_block,
    add_section,
    add_symbol,
    add_text_section,
    create_test_module,
)
from pprinter_helpers import (
    PPrinterTest,
    can_mock_binaries,
    interesting_lines,
    run_binary_pprinter_mock,
    run_asm_pprinter,
    asm_lines,
    run_binary_pprinter_mock_out,
)

TEST_DIR = os.path.abspath(os.path.dirname(__file__))
FAKEBIN_LLVM = os.path.join(TEST_DIR, "fakebin_llvm")


@unittest.skipUnless(can_mock_binaries(), "cannot mock binaries")
class WindowsBinaryPrinterTests(PPrinterTest):
    def test_windows_subsystem_gui(self):
        # This tests the changes in MR 346.
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.PE,
            isa=gtirb.Module.ISA.X64,
            binary_type=["EXEC", "EXE", "WINDOWS_GUI"],
        )
        _, bi = add_text_section(m)
        block = add_code_block(bi, b"\xC3")
        m.entry_point = block

        tools = list(run_binary_pprinter_mock(ir))
        self.assertEqual(len(tools), 1)
        self.assertEqual(tools[0].name, "ml64.exe")
        self.assertIn("/SUBSYSTEM:windows", tools[0].args)

    def test_windows_subsystem_console(self):
        # This tests the changes in MR 346.
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.PE,
            isa=gtirb.Module.ISA.X64,
            binary_type=["EXEC", "EXE", "WINDOWS_CUI"],
        )
        _, bi = add_text_section(m)
        block = add_code_block(bi, b"\xC3")
        m.entry_point = block

        tools = list(run_binary_pprinter_mock(ir))
        self.assertEqual(len(tools), 1)
        self.assertEqual(tools[0].name, "ml64.exe")
        self.assertIn("/SUBSYSTEM:console", tools[0].args)

    def test_windows_dll(self):
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.PE,
            isa=gtirb.Module.ISA.X64,
            binary_type=["EXEC", "DLL", "WINDOWS_CUI"],
        )
        _, bi = add_text_section(m)
        add_code_block(bi, b"\xC3")

        tools = list(run_binary_pprinter_mock(ir))
        self.assertEqual(len(tools), 1)
        self.assertEqual(tools[0].name, "ml64.exe")
        self.assertIn("/DLL", tools[0].args)

    def test_windows_defs(self):
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.PE,
            isa=gtirb.Module.ISA.X64,
            binary_type=["EXEC", "EXE", "WINDOWS_CUI"],
        )
        m.aux_data["peImportEntries"].data.append(
            (0, -1, "GetMessageW", "USER32.DLL")
        )

        for tool in run_binary_pprinter_mock(ir):
            if tool.name == "lib.exe":
                def_arg = next(
                    (arg for arg in tool.args if arg.startswith("/DEF:")), None
                )
                self.assertIsNotNone(def_arg, "no /DEF in lib invocation")
                self.assertIn("/MACHINE:X64", tool.args)

                with open(def_arg[5:], "r") as f:
                    lines = interesting_lines(f.read())
                    self.assertEqual(
                        lines,
                        ['LIBRARY "USER32.DLL"', "EXPORTS", "GetMessageW"],
                    )
                break
        else:
            self.fail("did not see a lib.exe execution")

    def test_windows_defs_with_llvm(self):
        """
        Check that:
        - we find the llvm installation directory
          using our fake llvm-config
        - the directory gets added to our path
        - we find llvm-dlltool
        """
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.PE,
            isa=gtirb.Module.ISA.X64,
            binary_type=["EXEC", "EXE", "WINDOWS_CUI"],
        )
        m.aux_data["peImportEntries"].data.append(
            (0, -1, "GetMessageW", "USER32.DLL")
        )

        expected_calls = ["llvm-config", "llvm-dlltool", "ml64.exe"]
        for tool in run_binary_pprinter_mock(ir, fakebin_dir=FAKEBIN_LLVM):
            if tool.name in expected_calls:
                expected_calls.remove(tool.name)
            if len(expected_calls) == 0:
                break
        else:
            self.fail(
                "did not see the following executions: "
                + ",".join(expected_calls)
            )


class WindowsBinaryPrinterTests_NoMock(PPrinterTest):
    def test_windows_includelib(self):
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.PE,
            isa=gtirb.Module.ISA.X64,
            binary_type=["EXEC", "EXE", "WINDOWS_CUI"],
        )

        _, bi = add_text_section(m)
        m.aux_data["libraries"].data.append(("WINSPOOL.DRV"))
        m.aux_data["libraries"].data.append(("USER32.DLL"))

        asm = run_asm_pprinter(ir)

        self.assertContains(asm_lines(asm), ["INCLUDELIB WINSPOOL.lib"])
        self.assertContains(asm_lines(asm), ["INCLUDELIB USER32.lib"])

        self.assertNotContains(asm_lines(asm), ["INCLUDELIB WINSPOOL.DRV"])
        self.assertNotContains(asm_lines(asm), ["INCLUDELIB USER32.DLL"])

    def test_windows_dll_exports(self):
        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.PE,
            isa=gtirb.Module.ISA.IA32,
            binary_type=["EXEC", "DLL", "WINDOWS_CUI"],
        )
        _, bi = add_text_section(m, 0x4000A8)
        block = add_code_block(bi, b"\xC3")
        sym = add_symbol(m, "__glutInitWithExit", block)

        m.aux_data["peExportedSymbols"].data.append(sym.uuid)

        asm = run_asm_pprinter(ir)

        self.assertContains(
            asm_lines(asm), ["___glutInitWithExit PROC EXPORT"]
        )

    def make_pe_resource_data(self) -> gtirb.IR:

        ir, m = create_test_module(
            file_format=gtirb.Module.FileFormat.PE,
            isa=gtirb.Module.ISA.X64,
            binary_type=["EXEC", "EXE", "WINDOWS_GUI"],
        )

        _, bi = add_section(m, ".text")
        entry = add_code_block(bi, b"\xC3")
        m.entry_point = entry

        resource_data = b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\
        \x00\x00\x00\x00\x00\x02\x00\x06\x00\x00\x00 \x00\x00\x80\
        \x18\x00\x00\x008\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\
        \x00\x00\x00\x00\x00\x00\x00\x01\x00\x07\x00\x00\x00P\x00\
        \x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
        \x00\x00\x01\x00\x01\x00\x00\x00h\x00\x00\x80\x00\x00\x00\
        \x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\t\x04\
        \x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
        \x00\x00\x00\x00\x00\x01\x00\t\x04\x00\x00\x90\x00\x00\x00\
        \xa0`\x00\x00H\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
        \xe8`\x00\x00}\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
        \x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x14\x00T\x00e\x00s\
        \x00t\x00 \x00r\x00e\x00s\x00o\x00u\x00r\x00c\x00e\x00 \x00s\
        \x00t\x00r\x00i\x00n\x00g\x00\x00\x00\x00\x00\x00\x00\x00\
        \x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
        <?xml version='1.0' encoding='UTF-8' standalone='yes'?>\
        \r\n<assembly xmlns='urn:schemas-microsoft-com:asm.v1\
        ' manifestVersion='1.0'>\r\n  \
        <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">\r\n\
            <security>\r\n      <requestedPrivileges>\r\n        \
            <requestedExecutionLevel level='asInvoker' uiAccess='false' />\r\n\
          </requestedPrivileges>\r\n    </security>\r\n  </trustInfo>\
                 \r\n</assembly>\r\n\x00\x00\x00')"

        _, bi = add_section(m, ".rsrc")
        _ = add_byte_block(bi, gtirb.block.DataBlock, resource_data)
        off1 = gtirb.Offset(bi, 0)
        off2 = gtirb.Offset(bi, 72)
        entry1 = (
            [
                72,
                0,
                0,
                0,
                32,
                0,
                0,
                0,
                255,
                255,
                6,
                0,
                255,
                255,
                7,
                0,
                0,
                0,
                0,
                0,
                48,
                16,
                9,
                4,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
            ],
            off1,
            72,
        )
        entry2 = (
            [
                125,
                1,
                0,
                0,
                32,
                0,
                0,
                0,
                255,
                255,
                24,
                0,
                255,
                255,
                1,
                0,
                0,
                0,
                0,
                0,
                48,
                16,
                9,
                4,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
            ],
            off2,
            381,
        )
        m.aux_data["peResources"] = gtirb.AuxData(
            [entry1, entry2],
            "sequence<tuple<sequence<uint8_t>,Offset,uint64_t>>",
        )
        return ir

    def test_windows_pe_resource_data(self):
        # checks that the IR gets turned into a binary
        ir = self.make_pe_resource_data()
        output = run_binary_pprinter_mock_out(ir, []).stdout.decode(
            sys.stdout.encoding
        )
        self.assertTrue(output)

    @unittest.skipUnless(can_mock_binaries(), "cannot mock binaries")
    def test_windows_pe_resource_data_mock(self):
        # checks that a resource file is compiled from the resource data
        ir = self.make_pe_resource_data()
        has_resource_file = False
        for output in run_binary_pprinter_mock(ir):
            if any(".res" in arg for arg in output.args):
                has_resource_file = True

        self.assertTrue(has_resource_file, "did not produce resource file")
