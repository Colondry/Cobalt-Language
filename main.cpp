#include <iostream>
#include <cstdio>
#include <filesystem>
#include "lexer.hpp"
#include "parser.hpp"
#include "flib.hpp"
#include "codeGen.hpp"
#include "Interpreter.hpp"

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

    Parser parser(tokens);
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

    codeGen(program, outputFile);
    return program;
}

bool invokeCppCompiler(const Program& program, const std::string& inputFile, const std::string& outputFile, const std::string& optimize, const std::string& extraFlags) {
    std::string outcpp = outputFile + ".cpp";
    fs::path exePath = resolveExePath(inputFile, outputFile);

    std::string command = "g++ -o \"" + exePath.string() + "\" " + outcpp + optimize + extraFlags;
    for (const LibImport& imp : program.imports) {
        command += " " + imp.libName + ".cpp";
    }

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

int main(int argc, char* argv[]) {
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

        if (cmd == "-build" || cmd == "-run") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a file argument.\n";
                return 1;
            }
            inputFile = argv[++i];
            if (cmd == "-build") doBuild = true;
            else doRun = true;
        } else if (cmd == "-run-experimental") {
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
        } else if (cmd == "-cpp") {
            remcpp = false;
        } else if (cmd == "-optimize-lvl-0") {
            optimizeCode = " -O0 "; // no optimization
        } else if (cmd == "-optimize-lvl-1") {
            isOptimizel1 = true;
        } else if (cmd == "-optimize-lvl-2") {
            isOptimizel2 = true;
        } else if (cmd == "-optimize-lvl-3" || cmd == "-optimize-performance") {
            isOptimizeFast = true;
        } else if (cmd == "-name") {
            if (i + 1 >= argc) {
                std::cerr << "Error: -name requires an argument.\n";
                return 1;
            }
            outputFile = argv[++i];
        } else if (cmd == "--version" || cmd == "version") {
            std::cout << "Cobalt Alpha v0.3";
            return 0;
        } else if (cmd == "--help") {
            std::cout <<
R"(Cobalt Compiler

Usage:
    cobalt -build <input_file> -name <output_file_name>
    cobalt -run <input_file> -name <output_file_name>
    cobalt --version
    cobalt --help

Options:
    -build                  Compile file only
    -run                    Compile and immediately run the result
    -name                   Output executable name (default: out)
    -debug                  Print parsed imports/functions before generating code
    -run-experimental       Run the interpreter directly (experimental)
    -optimize-lvl-0         No optimization (default)
    -optimize-lvl-1         Basic optimization
    -optimize-lvl-2         More optimization
    -optimize-lvl-3         Aggressive optimization (may break some code)
    -optimize-performance   Same as -optimize-lvl-3
    -pipe                   Use pipe for linking (may speed up linking)
    -fuse-bfd               Use BFD linker instead of default (may speed up linking)
    -flto                   Enable link-time optimization (may speed up linking)
    -cpp                    Keep the generated .cpp instead of deleting it
    
    --version               Show version
    --help                  Show this help
    
        )" << "\n";
            return 0;
        } else if (cmd == "-pipe") {
            extraFlags += " -pipe ";
        } else if (cmd == "-fuse-bfd ") {
            extraFlags += " -fuse-ld=bfd ";
        } else if (cmd == "--changelog") {
            std::cout <<
R"(Cobalt Alpha v0.3 Changelog :

# 🐞 Bug Fixes

- Fixed various parser, compiler, and code generation issues.
- Improved compiler stability and reliability.

# New Operators
- "+="
- "-="
- "-="
- "/="

# New Syntax =
- "inputstr([prompt] >> [variable], [text_limit]);" — Reads an entire line of text into a string.
- "continue;" — Skips to the next iteration of a loop.
- "break;" — Exits the current loop.
- "elif" — Else-if conditional branch.
- "else" — Default conditional branch.
- "clear();" — Clears the console screen.
- Dot (".") operator for object member access and method invocation.
  - Example:
    "window.wnew();"

# Syntax Changes =
- Generic list syntax has been updated:
  - Old: "List::Type"
  - New: "List<Type>"

⚙️ New Command-Line Options =
- "-run-experimental" — Runs programs using the experimental interpreter.
- "-changelog" — Displays the current changelog.
- "-optimize":
  - "-lvl0"
  - "-lvl1"
  - "-lvl2"
  - "-lvl3"
  - "-performance"
- "-pipes"
- "-fuse-bfd"

        )" << "\n";
            return 0;
        } else if (cmd == "-flto") {
            extraFlags += " -flto ";
        } else {
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
    } else if (isOptimizel2) {
        optimizeCode = " -O2 ";
    } else if (isOptimizel1) {
        optimizeCode = " -O1  ";
    } else {
        optimizeCode = " -O1 "; // default to no optimization
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
