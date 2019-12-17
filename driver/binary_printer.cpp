#include "ElfBinaryPrinter.hpp"
#include "Logger.h"
#include <boost/program_options.hpp>
#include <experimental/filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace fs = std::experimental::filesystem;
namespace po = boost::program_options;

int main(int argc, char** argv) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.");
  desc.add_options()("ir,i", po::value<std::string>(), "gtirb file to print.");
  desc.add_options()("binary,b", po::value<std::string>(),
                     "The name of the binary output file.");
  desc.add_options()("keep-functions,k",
                     po::value<std::vector<std::string>>()->multitoken(),
                     "Print the given functions even if they are skipped by "
                     "default (e.g. _start)");
  desc.add_options()("skip-functions,n",
                     po::value<std::vector<std::string>>()->multitoken(),
                     "Do not print the given functions.");
  desc.add_options()("compiler-args,c",
                     po::value<std::vector<std::string>>()->multitoken(),
                     "Additional arguments to pass to the compiler");
  desc.add_options()("library-paths,L",
                     po::value<std::vector<std::string>>()->multitoken(),
                     "Library paths to be passed to the linker");
  po::positional_options_description pd;
  pd.add("ir", -1);
  po::variables_map vm;
  try {
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(pd).run(),
        vm);
    if (vm.count("help") != 0) {
      std::cout << desc << "\n";
      return 1;
    }
  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << "\nTry '" << argv[0]
              << " --help' for more information.\n";
    return 1;
  }
  po::notify(vm);

  gtirb::Context ctx;
  gtirb::IR* ir;

  if (vm.count("ir") != 0) {
    fs::path irPath = vm["ir"].as<std::string>();
    if (fs::exists(irPath)) {
      LOG_INFO << std::setw(24) << std::left << "Reading IR: " << irPath
               << std::endl;
      std::ifstream in(irPath.string());
      ir = gtirb::IR::load(ctx, in);
    } else {
      LOG_ERROR << "IR not found: \"" << irPath << "\".";
      return EXIT_FAILURE;
    }
  } else {
    ir = gtirb::IR::load(ctx, std::cin);
  }

  // Perform the Pretty Printing step.
  gtirb_pprint::PrettyPrinter pp;
  pp.setDebug(vm.count("debug"));
  const std::string& format =
      gtirb_pprint::getModuleFileFormat(*ir->modules().begin());
  const std::string& syntax =
      gtirb_pprint::getDefaultSyntax(format).value_or("");
  auto target = std::make_tuple(format, syntax);
  if (gtirb_pprint::getRegisteredTargets().count(target) == 0) {
    LOG_ERROR << "Unsupported combination: format '" << format
              << "' and syntax '" << syntax << "'\n";
    std::string::size_type width = 0;
    for (const auto& [f, s] : gtirb_pprint::getRegisteredTargets())
      width = std::max({width, f.size(), s.size()});
    width += 2; // add "gutter" between columns
    LOG_ERROR << "Available combinations:\n";
    LOG_ERROR << "    " << std::setw(width) << "format"
              << "syntax\n";
    for (const auto& [f, s] : gtirb_pprint::getRegisteredTargets())
      LOG_ERROR << "    " << std::setw(width) << f << s << '\n';
    return EXIT_FAILURE;
  }
  pp.setTarget(std::move(target));

  if (vm.count("keep-functions") != 0) {
    for (const auto& keep :
         vm["keep-functions"].as<std::vector<std::string>>()) {
      pp.keepFunction(keep);
    }
  }

  if (vm.count("skip-functions") != 0) {
    for (const auto& skip :
         vm["skip-functions"].as<std::vector<std::string>>()) {
      pp.skipFunction(skip);
    }
  }

  if (vm.count("binary") != 0) {
    gtirb_bprint::ElfBinaryPrinter binaryPrinter(true);
    const auto binaryPath = fs::path(vm["binary"].as<std::string>());
    std::vector<std::string> extraCompilerArgs;
    if (vm.count("compiler-args") != 0)
      extraCompilerArgs = vm["compiler-args"].as<std::vector<std::string>>();
    std::vector<std::string> libraryPaths;
    if (vm.count("library-paths") != 0)
      libraryPaths = vm["library-paths"].as<std::vector<std::string>>();
    binaryPrinter.link(binaryPath.string(), extraCompilerArgs, libraryPaths, pp,
                       ctx, *ir);
  } else {
    LOG_INFO << "Please specify a binary name" << std::endl;
  }

  return EXIT_SUCCESS;
}
