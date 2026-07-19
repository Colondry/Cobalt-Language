#include <iostream>
#include <cstdio>
#include <filesystem>
#include "lexer.hpp"
#include "parser.hpp"
#include "flib.hpp"
#include "codeGen.hpp"

namespace fs = std::filesystem;

// The one place that decides what the built executable is called and
// where it lives. invokeCppCompiler() (the -o target), interpret()'s run
// step, and interpret()'s cleanup step all call this instead of each
// recomputing it slightly differently -- that mismatch is exactly what
// caused "'out' is not recognized": the build step and the run step used
// to compute two different paths.
fs::path resolveExePath(const std::string& inputFile, const std::string& outputFile) {
    std::string exeName = outputFile;
#ifdef _WIN32
    exeName += ".exe";
#endif
    fs::path folder = fs::path(inputFile).parent_path();
    fs::path result = folder.empty() ? fs::path(exeName) : folder / exeName;
    // Without this, a "./" in the input path (e.g. "./TestFile/main.cb")
    // survives into the joined path and ends up mixed with the backslash
    // that gets appended for the exe name -- "./TestFile\out.exe" -- which
    // is exactly the malformed-looking path that trips up cmd.exe.
    return result.lexically_normal();
}

// A path you can hand to std::system() to actually run. On POSIX, a bare
// filename with no directory component (e.g. "out") requires a "./"
// prefix or it's looked up on PATH instead of the current directory; a
// path that already contains a directory (e.g. "TestFile/out") does not.
// Windows needs neither.
std::string toRunnableCommand(const fs::path& exePath) {
    std::string p = exePath.string();
#ifndef _WIN32
    if (exePath.parent_path().empty()) p = "./" + p;
#endif
    // Only quote if the path actually contains a space -- unquoted is
    // simplest and sidesteps a cmd.exe quirk entirely in the common case.
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
    std::cout << "C++ code generated in " << outputFile << ".cpp\n";
    return program;
}

bool invokeCppCompiler(const Program& program, const std::string& inputFile, const std::string& outputFile) {
    std::string outcpp = outputFile + ".cpp";
    fs::path exePath = resolveExePath(inputFile, outputFile);

    std::string command = "g++ -o \"" + exePath.string() + "\" " + outcpp;
    for (const LibImport& imp : program.imports) {\
        command += " " + imp.libName + ".cpp";
    }

    int status = runSystemCommand(command);
    std::cout << "g++ exited with status " << status << ".\n";
    return status == 0;
}

void compile(const std::string& inputFile, const std::string& outputFile, bool isDeb, bool isRemCPP) {
    Program program = parseAndGenerate(inputFile, outputFile, isDeb);
    std::string outcpp = outputFile + ".cpp";

    if (!invokeCppCompiler(program, inputFile, outputFile)) {
        std::cerr << "Error: g++ failed to build " << outputFile << ".cpp\n";
        std::cerr << "(" << outcpp << " was left in place so you can inspect it.)\n";
        std::exit(EXIT_FAILURE);
    }

    if (isRemCPP) {
        std::remove(outcpp.c_str());
    }
    std::cout << "\nBuilt " << outputFile << "\n";
}

void interpret(const std::string& inputFile, const std::string& outputFile, bool isDeb, bool isRemCPP) {
    Program program = parseAndGenerate(inputFile, outputFile, isDeb);
    std::string outcpp = outputFile + ".cpp";

    if (!invokeCppCompiler(program, inputFile, outputFile)) {
        std::cerr << "Error: g++ failed to build " << outputFile << ".cpp\n";
        std::cerr << "(" << outcpp << " was left in place so you can inspect it.)\n";
        std::exit(EXIT_FAILURE);
    }

    fs::path exePath = resolveExePath(inputFile, outputFile);
    std::string runCommand = toRunnableCommand(exePath);
    std::cout << "Running " << runCommand << "\n";
\
    int status = runSystemCommand(runCommand);
    if (status != 0) {
        std::cerr << "Error: execution of " << runCommand << " failed (exit status " << status << ").\n";
        std::exit(EXIT_FAILURE);
    }
    std::cout << "\nExecution finished.\n";

    if (isRemCPP) {
        std::remove(outcpp.c_str());
        std::cout << "Removed " << outcpp << "\n";
    }
    std::cout << "Removed " << exePath << "\n";
    std::remove(exePath.string().c_str());
}

int main(int argc, char* argv[]) {
    std::string inputFile;
    std::string outputFile = "out"; // default name
    bool doBuild = false;
    bool doRun = false;
    bool isDeb = false;
    bool remcpp = true;

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
        } else if (cmd == "-debug") {
            isDeb = true;
        } else if (cmd == "-cpp") {
            remcpp = false;
        } else if (cmd == "-name") {
            if (i + 1 >= argc) {
                std::cerr << "Error: -name requires an argument.\n";
                return 1;
            }
            outputFile = argv[++i];
        } else if (cmd == "--version" || cmd == "version") {
            std::cout << "Cobalt Alpha v0.2\nPrototype 4;\n";
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
            -build          Compile file only
            -run            Compile and immediately run the result
            -name           Output executable name (default: out)
            -debug          Print parsed imports/functions before generating code
            -cpp            Keep the generated .cpp instead of deleting it
            --version       Show version
            --help          Show this help
        )" << "\n";
            return 0;
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
    }\

    if (doBuild) compile(inputFile, outputFile, isDeb, remcpp);
    else interpret(inputFile, outputFile, isDeb, remcpp);

    return 0;
}