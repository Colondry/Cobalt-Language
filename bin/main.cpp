#include <iostream>
#include <cstdio>
#include <filesystem>
#include <set>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
#include "lexer.hpp"
#include "parser.hpp"
#include "flib.hpp"
#include "codeGen.hpp"
#include "Interpreter.hpp"
#include "libmanage.hpp"

namespace fs = std::filesystem;

fs::path resolveExePath(const std::string& inputFile, const std::string& outputFile) {
    std::string exeName = outputFile;
#ifdef _WIN32
    exeName += ".exe";
#endif
    fs::path folder = fs::path(inputFile).parent_path();
    fs::path result = folder.empty() ? fs::path(exeName) : folder / exeName;
    return result.lexically_normal();
}

std::string toRunnableCommand(const fs::path& exePath) {
    std::string p = exePath.string();
#ifndef _WIN32
    if (exePath.parent_path().empty()) p = "./" + p;
#endif
    // Only quote if the path actually contains a space
    if (p.find(' ') == std::string::npos) return p;
    return "\"" + p + "\"";
}
\
int runSystemCommand(const std::string& command) {
#ifdef _WIN32
    if (!command.empty() && command.front() == '"') {
        return std::system(("\"" + command + "\"").c_str());
    }
#endif
    return std::system(command.c_str());
}

Program parseAndGenerate(const std::string& inputFile, const std::string& outputFile, bool isDebug) {
    std::string source = preprocessFile(inputFile);
    if (source.empty()) {
        std::cerr << "Error: " << inputFile << " is empty or could not be read.\n";
        std::exit(EXIT_FAILURE);
    }

    std::vector<Token> tokens = tokenize(source);

    Parser parser(tokens, source);
    Program program = parser.parse(); // exits with the full error list

    if (isDebug) {
        std::cout << "Parsed " << program.imports.size() << " import(s) and "
            << program.functions.size() << " function(s):\n";
        for (const LibImport& imp : program.imports) {
            std::cout << "  import <" << imp.libName << ">\n";
        }
        for (const FunctionDecl& fn : program.functions) {
            std::cout << "  fn " << fn.name << "(" << fn.params.size() << " param(s)): " << fn.returnType << "\n";
        }
        std::cout << "\n\nOutput:\n";
    }

    codeGen(program, outputFile, fs::path(inputFile).parent_path().string());
    return program;
}

bool invokeCppCompiler(const Program& program, const std::string& inputFile, const std::string& outputFile, const std::string& optimize, const std::string& extraFlags) {
    std::string outcpp = outputFile + ".cpp";
    fs::path exePath = resolveExePath(inputFile, outputFile);
    std::string inputFileDir = fs::path(inputFile).parent_path().string();

    std::string command = "g++ -std=c++23 -o \"" + exePath.string() + "\" " + outcpp + " -w " + optimize + extraFlags;

    std::set<std::string> libCpps; // dedupe: multiple imports may share a bundle dir's files
    std::set<std::string> linkFlagsSeen; // dedupe: multiple imports may share a bundle dir's link.txt
    std::string extraLinkFlags;
    for (const LibImport& imp : program.imports) {
        std::string bundleDir = findLibraryDir(imp.libName, inputFileDir);
        if (!bundleDir.empty()) {
            // Bundle directory: 
            for (const std::string& cpp : listCppFilesIn(bundleDir)) {
                libCpps.insert(cpp);
            }
            std::string flags = findLibraryLinkFlags(bundleDir);
            if (!flags.empty() && linkFlagsSeen.insert(bundleDir).second) {
                extraLinkFlags += " " + flags;
            }
        }
        else {
            std::string libCpp = findLibraryFile(imp.libName, ".cpp", inputFileDir);
            if (!libCpp.empty()) {
                libCpps.insert(libCpp);
            }
            else {
                bool headerExists = !findLibraryFile(imp.libName, ".hpp", inputFileDir).empty()
                    || !findLibraryFile(imp.libName, ".h", inputFileDir).empty();
                if (headerExists) {
                    // Header-only library -- nothing to compile/link separately.
                }
                else {
                    // Found neither header nor .cpp anywhere
                    libCpps.insert(imp.libName + ".cpp");
                }
            }
        }
    }
    for (const std::string& cpp : libCpps) {
        command += " \"" + cpp + "\"";
    }
    command += extraLinkFlags;
#ifdef _WIN32
    command += " -lstdc++exp";
#endif
    std::cout << command << "\n";
    int status = runSystemCommand(command);
    return status == 0;
}

void compile(const std::string& inputFile, const std::string& outputFile, bool isDeb, bool isRemCPP, const std::string& optimize, const std::string& extraFlags) {
    Program program = parseAndGenerate(inputFile, outputFile, isDeb);
    std::string outcpp = outputFile + ".cpp";

    if (!invokeCppCompiler(program, inputFile, outputFile, optimize, extraFlags)) {
        std::cerr << "Error: g++ failed to build " << outputFile << ".cpp\n";
        std::cerr << "(" << outcpp << " was left in place so you can inspect it.)\n";
        std::exit(EXIT_FAILURE);
    }

    if (isRemCPP) {
        std::remove(outcpp.c_str());
    }
}

void interpret(const std::string& inputFile, const std::string& outputFile, bool isDeb, bool isRemCPP, std::string optimize) {
    Program program = parseAndGenerate(inputFile, outputFile, isDeb);
    std::string outcpp = outputFile + ".cpp";

    if (!invokeCppCompiler(program, inputFile, outputFile, optimize, " -pipe -fuse-ld=bfd -flto ")) {
        std::cerr << "Error: g++ failed to build " << outputFile << ".cpp\n";
        std::cerr << "(" << outcpp << " was left in place so you can inspect it.)\n";
        std::exit(EXIT_FAILURE);
    }

    fs::path exePath = resolveExePath(inputFile, outputFile);
    std::string runCommand = toRunnableCommand(exePath);

    int status = runSystemCommand(runCommand);
    if (status != 0) {
        std::cerr << "Error: execution of " << runCommand << " failed (exit status " << status << ").\n";
        std::exit(EXIT_FAILURE);
    }

    if (isRemCPP) {
        std::remove(outcpp.c_str());
    }
    std::remove(exePath.string().c_str());
}

void InterpretExperimental(const std::string& inputFile, const std::string& outputFile, bool isDeb) {
    Program program = parseAndGenerate(inputFile, outputFile, isDeb);
    Interpreter(program);
}

#include <fstream>
using json = nlohmann::json;
std::string ReadJSON(std::string libName) {
    std::ifstream f("./lib/" + libName + "/config.json");
    if (!f.is_open()) return "  Name : " + libName + ".\n  Version : Unknown.\n";

    json data = json::parse(f);

    std::string version = data["version"];
    std::string name = data["name"];

    std::string rv = "  Name : " + name + ".\n  Version : " + version + ".\n";
    return rv;
}

void listsLib() {
    // Set your target directory path
    std::string target_path = "./lib";

    // Check if path exists
    if (!fs::exists(target_path)) {
        std::cout << "Directory does not exist.\n";
        fs::create_directories("./lib");
    }
    std::cout << "Library Lists :\n";

    // Loop through the directory
    for (const auto& entry : fs::directory_iterator(target_path)) {

        // Check if the entry is a folder
        if (fs::is_directory(entry.status())) {
            auto old_path = entry.path();
            std::string name = old_path.filename().string();
            std::string out = ReadJSON(name);

            std::cout << out
                      << "---------------------------\n";
        }
    }
}

int main(int argc, char* argv[]) {
    setExecutablePath(argv[0]);

    std::string inputFile;
    std::string outputFile = "out"; // default name
    bool doBuild = false;
    bool doRun = false;
    bool isDeb = false;
    std::string optimizeCode = "";
    bool isOptimizeFast = false;
    bool isOptimizel1 = false;
    bool isOptimizel2 = false;
    bool remcpp = true;
    std::string extraFlags = "";

    for (int i = 1; i < argc; i++) {
        std::string cmd = argv[i];

        if (cmd == "build" || cmd == "run") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a file argument.\n";
                return 1;
            }
            inputFile = argv[++i];
            if (cmd == "build") doBuild = true;
            else doRun = true;
        }
        else if (cmd == "lists") {
            listsLib();
            return 0;
        }
        else if (cmd == "remove") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a library argument.\n";
                return 1;
            }
            std::string lib = argv[i++];
            if (lib == "remove") {
                lib = argv[i++];
            }
            if (!fs::exists("./lib/" + lib + "/")) {
                std::cout << lib << "is not exist on library lists.\n";
                return 0;
            }
            else {
                std::string command = "rmdir /s /q \"./lib/" + lib + "\"";
                std::cout << command << "\n";
                int status = std::system(command.c_str());

                if (status == 1) {
                    std::cerr << "remove operation was unsuccessful.\n";
                    return 1;
                }
                else {
                    std::cout << "remove operation was successful\n";
                    return 0;
                }
            }
        }
        else if (cmd == "check") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a library argument.\n";
                return 1;
            }
            std::string lib = argv[i++];
            if (lib == "check") {
                lib = argv[i++];
            }
            if (!fs::exists("./lib")) {
                fs::create_directories("./lib");
            }
            if (fs::exists("./lib/" + lib + "/") && fs::is_directory("./lib/" + lib + "/")) {
                std::cout << lib + " library do exists.\n";
                return 0;
            }
            std::string input;
            std::cout << lib << " library is not found.\nDo you want to install them? (yes or no): \n";
            std::cin >> input;
            std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
                return std::tolower(c);
                });
            if (input == "yes") {
                if (installPackage(lib)) {
                    if (fs::exists("./lib/" + lib + "/") && fs::is_directory("./lib/" + lib + "/")) {
                        return 0;
                    }
                    std::cout << lib << " successfully installed.\n";
                }
                else {
                    std::cerr << "Error: failed to install \"" << lib << "\" (not found in Cobalt-Package, a dependency failed, or no network access).\n";
                    return 1;
                }
            }
            else if (input == "y") {
                if (installPackage(lib)) {
                    std::cout << lib << " successfully installed.\n";
                }
                else {
                    std::cerr << "Error: failed to install \"" << lib << "\" (not found in Cobalt-Package, a dependency failed, or no network access).\n";
                    return 1;
                }
            }
            else {
                return 0;
            }
        }
        else if (cmd == "install") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a library argument.\n";
                return 1;
            }
            std::string lib = argv[++i];
            if (!fs::exists("./lib")) {
                fs::create_directories("./lib");
            }

            if (installPackage(lib)) {
                if (fs::exists("./lib/" + lib + "/") && fs::is_directory("./lib/" + lib + "/")) {
                    return 0;
                }
                std::cout << lib << " successfully installed.\n";
            }
            else {
                std::cerr << "Error: failed to install \"" << lib << "\" (not found in Cobalt-Package, a dependency failed, or no network access).\n";
                return 1;
            }
            return 0;
        }
        else if (cmd == "run-fast") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a file argument.\n";
                return 1;
            }
            inputFile = argv[++i];
            std::cout << "Warning: Interpreter mode is experimental.\n";
            InterpretExperimental(inputFile, outputFile, isDeb);
            return 0;
        }
        else if (cmd == "-debug") {
            isDeb = true;
        }
        else if (cmd == "-cpp") {
            remcpp = false;
        }
        else if (cmd == "-O0") {
            optimizeCode = " -O0 "; // no optimization
        }
        else if (cmd == "-O1") {
            isOptimizel1 = true;
        }
        else if (cmd == "-O2") {
            isOptimizel2 = true;
        }
        else if (cmd == "-O3" || cmd == "-OPerformance") {
            isOptimizeFast = true;
        }
        else if (cmd == "as") {
            if (i + 1 >= argc) {
                std::cerr << "Error: -name requires an argument.\n";
                return 1;
            }
            outputFile = argv[++i];
        }
        else if (cmd == "--version" || cmd == "version") {
            std::cout << "Cobalt Beta v0.6.1 \"Onyx\"";
            return 0;
        }
        else if (cmd == "--help" || cmd == "help") {
            std::cout <<
                R"(Cobalt Compiler

Usage:
    cobalt -build <input_file> -name <output_file_name>
    cobalt -run <input_file> -name <output_file_name>
    cobalt --version
    cobalt --help

Options:
    build                  Compile file only
    run                    Compile and immediately run the result
    -as                    Output executable name (default: out)
    -debug                 Print parsed imports/functions before generating code
    -run-fast              Run the interpreter directly (experimental)
    -OX                    No optimization (default)
    -O1                    Basic optimization
    -O2                    More optimization
    -O3                    Aggressive optimization (may break some code)
    -OPerformance          Same as -optimize-lvl-3
    -pipe                  Use pipe for linking (may speed up linking)
    -fuse-bfd              Use BFD linker instead of default (may speed up linking)
    -flto                  Enable link-time optimization (may speed up linking)
    -cpp                   Keep the generated .cpp instead of deleting it
    
    version                Show version
    help                   Show this help
    
        )" << "\n";
            return 0;
        }
        else if (cmd == "-pipe") {
            extraFlags += " -pipe ";
        }
        else if (cmd == "-fuse-bfd ") {
            extraFlags += " -fuse-ld=bfd ";
        }
        else if (cmd == "-flto") {
            extraFlags += " -flto ";
        }
        else {
            std::cerr << "Unknown argument '" << cmd << "'. See --help.\n";
            return 1;
        }
    }

    if (doBuild && doRun) {
        std::cerr << "Error: pass either -build or -run, not both.\n";
        return 1;
    }
    if (!doBuild && !doRun) {
        std::cerr << "Error: pass -build <file> or -run <file>. See --help.\n";
        return 1;
    }

    if (isOptimizeFast) {
        optimizeCode = " -Ofast ";
    }
    else if (isOptimizel2) {
        optimizeCode = " -O2 ";
    }
    else if (isOptimizel1) {
        optimizeCode = " -O1  ";
    }
    else {
        optimizeCode = " -O0 "; // default to no optimization
    }
    if (doRun && extraFlags != "") {
        std::cerr << "Warning: extra flags '" << extraFlags << "' will be ignored in -run mode.\n";
    }

    if (doBuild) compile(inputFile, outputFile, isDeb, remcpp, optimizeCode, extraFlags);
    else if (doRun) {
        interpret(inputFile, outputFile, isDeb, remcpp, optimizeCode);
    }
    else {
        std::cerr << "Error: no action specified. Use -build or -run. See --help.\n";
        return 1;
    }

    return 0;
}