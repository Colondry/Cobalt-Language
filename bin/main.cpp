#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define TokenType Win32_TokenType // Rename Windows enum symbol before it gets declared
  #include <windows.h>
  #include <psapi.h>
  #undef TokenType                  // Restore 'TokenType' for your lexer/parser
#elif defined(__APPLE__) || defined(__linux__)
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/resource.h>
    #include <unistd.h>
    #if defined(__APPLE__)
        #include <libproc.h>
    #elif defined(__linux__)
        #include <fstream>
    #endif
#endif

#include <iostream>
#include <cstdio>
#include <filesystem>
#include <set>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <thread>
#include <chrono>
#include <numeric>

#include <nlohmann/json.hpp>
#include "lexer.hpp"
#include "parser.hpp"
#include "flib.hpp"
#include "codeGen.hpp"
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
    std::string outFile = outputFile.empty() ? fs::path(inputFile).stem().string() : outputFile;
    size_t exePos = outFile.find(".exe");
    if (exePos != std::string::npos) {
        outFile.erase(exePos, 4);
    }
    codeGen(program, outFile, fs::path(inputFile).parent_path().string());
    return program;
}

#if defined(__APPLE__) || defined(__linux__)
static double getProcessMemoryMB(pid_t pid) {
#if defined(__APPLE__)
    struct proc_taskinfo pti;
    if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti)) == sizeof(pti)) {
        return static_cast<double>(pti.pti_resident_size) / (1024.0 * 1024.0);
    }
#elif defined(__linux__)
    std::ifstream statm("/proc/" + std::to_string(pid) + "/statm");
    if (statm.is_open()) {
        long totalPages = 0, rssPages = 0;
        statm >> totalPages >> rssPages;
        long pageSize = sysconf(_SC_PAGESIZE);
        return static_cast<double>(rssPages * pageSize) / (1024.0 * 1024.0);
    }
#endif
    return 0.0;
}
#endif

void executeAndMeasure(const std::string& exePath) {
    std::vector<double> ramSamplesMB;
    double totalCpuSeconds = 0.0;

#ifdef _WIN32
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(exePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to start process for profiling.\n";
        return;
    }

    PROCESS_MEMORY_COUNTERS pmc;
    while (WaitForSingleObject(pi.hProcess, 15) == WAIT_TIMEOUT) {
        if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc))) {
            ramSamplesMB.push_back(pmc.WorkingSetSize / (1024.0 * 1024.0));
        }
    }

    // Final sample check before process handle closes
    if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc))) {
        ramSamplesMB.push_back(pmc.WorkingSetSize / (1024.0 * 1024.0));
    }

    FILETIME ftCreate, ftExit, ftKernel, ftUser;
    GetProcessTimes(pi.hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser);
    
    uint64_t kTime = ((uint64_t)ftKernel.dwHighDateTime << 32) | ftKernel.dwLowDateTime;
    uint64_t uTime = ((uint64_t)ftUser.dwHighDateTime << 32) | ftUser.dwLowDateTime;
    totalCpuSeconds = (kTime + uTime) / 10000000.0;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

#elif defined(__APPLE__) || defined(__linux__)
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork process.\n";
        return;
    }

    if (pid == 0) {
        execl(exePath.c_str(), exePath.c_str(), nullptr);
        _exit(127);
    }

    int status = 0;
    struct rusage ru {};

    while (true) {
        double ram = getProcessMemoryMB(pid);
        if (ram > 0.0) {
            ramSamplesMB.push_back(ram);
        }

        // Non-blocking wait to check process exit and populate rusage
        pid_t res = wait4(pid, &status, WNOHANG, &ru);
        if (res == pid || res < 0) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    totalCpuSeconds = (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) +
                       (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1000000.0;
#endif

    if (!ramSamplesMB.empty()) {
        auto [minIt, maxIt] = std::minmax_element(ramSamplesMB.begin(), ramSamplesMB.end());
        double avgRam = std::accumulate(ramSamplesMB.begin(), ramSamplesMB.end(), 0.0) / ramSamplesMB.size();

        std::cout << "\n--- Resource Report ---" << std::endl;
        std::cout << "RAM Peak : " << *maxIt << " MB\n";
        std::cout << "RAM Avg  : " << avgRam << " MB\n";
        std::cout << "RAM Min  : " << *minIt << " MB\n";
        std::cout << "Total CPU Time Spent: " << totalCpuSeconds << " seconds\n";
    } else {
        std::cout << "\n--- Resource Report ---" << std::endl;
        std::cout << "No memory samples collected.\n";
        std::cout << "Total CPU Time Spent: " << totalCpuSeconds << " seconds\n";
    }
}

bool invokeCppCompiler(const Program& program, const std::string& inputFile, const std::string& outputFile, const std::string& optimize, const std::string& extraFlags) {
    std::string outcpp = outputFile + ".cpp";
    fs::path exePath = resolveExePath(inputFile, outputFile);
    std::string inputFileDir = fs::path(inputFile).parent_path().string();

    std::string command = "g++ -std=c++23 -o \"" + exePath.string() + "\" " + outcpp + " -w -I\"./Bin\" " + optimize + extraFlags;

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
void noCompile(const std::string& inputFile, const std::string& outputFile, bool isDeb, bool nfile, bool genASM, const std::string& optimize, const std::string& extraFlags) {
    fs::path inputP(inputFile);
    fs::path parentDir = inputP.has_parent_path() ? inputP.parent_path() : fs::current_path();
    fs::path targetPath = parentDir / outputFile;

    parseAndGenerate(inputFile, targetPath.string(), isDeb);
    if (genASM && !nfile) {
        std::string outcpp = targetPath.string() + ".cpp";
        std::string asmFile = targetPath.string() + ".asm";
        std::string command = "g++ -std=c++23 -S -o \"" + asmFile + "\" " + outcpp + " -w -I\"./Bin\" " + optimize + extraFlags;
        std::cout << command << "\n";
        int status = runSystemCommand(command);
        if (status != 0) {
            std::cerr << "Error: g++ failed to generate assembly for " << outcpp << "\n";
            std::exit(EXIT_FAILURE);
        }
    } else if (genASM && nfile) {
        std::string outcpp = targetPath.string() + ".cpp";
        std::string asmFile = targetPath.string() + ".asm";
        std::string command = "g++ -std=c++23 -S -o \"" + asmFile + "\" " + outcpp + " -w -I\"./Bin\" " + optimize + extraFlags;
        std::cout << command << "\n";
        int status = runSystemCommand(command);
        if (status != 0) {
            std::cerr << "Error: g++ failed to generate assembly for " << outcpp << "\n";
            std::exit(EXIT_FAILURE);
        }
        std::ifstream asmFileStream(asmFile);
        if (!asmFileStream.is_open()) {
            std::cerr << "Error: could not open generated assembly file " << asmFile << "\n";
            std::exit(EXIT_FAILURE);
        }
        std::cout << asmFileStream.rdbuf();
        std::remove(asmFile.c_str());
    }
    else if ((!genASM && nfile)) {
        std::ifstream ofile(targetPath.string() + ".cpp");
        if (!ofile.is_open()) {
            std::cerr << "Error: could not open generated file " << targetPath.string() + ".cpp" << "\n";
            std::exit(EXIT_FAILURE);
        }
        std::cout << ofile.rdbuf();
    }
}

void interpret(const std::string& inputFile, const std::string& outputFile, bool isDeb, bool isRemCPP, std::string optimize, const std::string& extraFlags, bool ms) {
    Program program = parseAndGenerate(inputFile, outputFile, isDeb);
    std::string outcpp = outputFile + ".cpp";

    if (!invokeCppCompiler(program, inputFile, outputFile, optimize, extraFlags)) {
        std::cerr << "Error: g++ failed to build " << outputFile << ".cpp\n";
        std::cerr << "(" << outcpp << " was left in place so you can inspect it.)\n";
        std::exit(EXIT_FAILURE);
    }

    fs::path exePath = fs::absolute(resolveExePath(inputFile, outputFile));
    fs::path originalDir = fs::current_path();
    fs::path targetDir = exePath.parent_path();

    if (!targetDir.empty()) {
        fs::current_path(targetDir);
    }
    if (ms) executeAndMeasure(exePath.string());
    else {
        std::string runCommand = toRunnableCommand(exePath.filename());
        int status = runSystemCommand(runCommand);

        if (status != 0) {
            std::cerr << "Error: execution of " << runCommand << " failed (exit status " << status << ").\n";
            std::exit(EXIT_FAILURE);
        }
    }
    fs::current_path(originalDir);
    if (isRemCPP) std::remove(outcpp.c_str());
    std::remove(exePath.string().c_str());
}

void InterpretExperimental(const std::string& inputFile, const std::string& outputFile, bool isDeb) {
    // Program program = parseAndGenerate(inputFile, outputFile, isDeb);
    // Interpreter(program);
    std::cout << "Cobalt Interpreter support has been depreceated.\n";
    std::exit(EXIT_FAILURE);
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

int cobaltMain(int argc, char* argv[]) {
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
    bool nfile = false;
    bool runAndCompile = true;
    bool genASM = false;
    bool measure = false;
    int_least8_t config = 0;
    std::string extraFlags = "";

    for (int i = 1; i < argc; i++) {
        std::string cmd = argv[i];

        if (cmd == "build" || cmd == "run") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a file argument.\n";
                return 1;
            }
            inputFile = argv[++i];
            if (fs::path(inputFile).extension() != ".cb") {
                std::cerr << "Error: " << cmd << " requires a .cobalt file argument.\n";
                return 1;
            }
            if (cmd == "build") doBuild = true;
            else doRun = true;
            continue;
        }
        else if (cmd == "conv") {
            runAndCompile = false;
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a file argument.\n";
                return 1;
            }
            inputFile = argv[++i];
            outputFile = fs::path(inputFile).stem().string();
            if (fs::path(inputFile).extension() != ".cb") {
                std::cerr << "Error: " << cmd << " requires a .cobalt file argument.\n";
                return 1;
            }
        }
        else if (cmd == "conv-nfile") {
            runAndCompile = false;
            nfile = true;
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a file argument.\n";
                return 1;
            }
            inputFile = argv[++i];
            outputFile = fs::path(inputFile).stem().string();
            if (fs::path(inputFile).extension() != ".cb") {
                std::cerr << "Error: " << cmd << " requires a .cobalt file argument.\n";
                return 1;
            }
        }
        else if (cmd == "conv=asm") {
            runAndCompile = false;
            genASM = true;
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a file argument.\n";
                return 1;
            }
            inputFile = argv[++i];
            outputFile = fs::path(inputFile).stem().string();
            if (fs::path(inputFile).extension() != ".cb") {
                std::cerr << "Error: " << cmd << " requires a .cobalt file argument.\n";
                return 1;
            }
        }
        else if (cmd == "conv-nfile=asm") {
            runAndCompile = false;
            genASM = true;
            nfile = true;
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << " requires a file argument.\n";
                return 1;
            }
            inputFile = argv[++i];
            outputFile = fs::path(inputFile).stem().string();
            if (fs::path(inputFile).extension() != ".cb") {
                std::cerr << "Error: " << cmd << " requires a .cobalt file argument.\n";
                return 1;
            }
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
        else if (cmd == "update") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << cmd << "requires a library argument.\n";
                return 1;
            }
            std::string lib = argv[++i];
            if (!fs::exists("./lib")) {
                fs::create_directories("./lib");
            }
            if (lib == "update") {
                lib = argv[i++];
            }
            if (!fs::exists("./lib/" + lib + "/")) {
                std::cout << lib << "is not exist on library path.\n";
                return 0;
            }
            else {
                std::string command = "rmdir /s /q \"./lib/" + lib + "\"";
                std::cout << command << "\n";
                std::system(command.c_str());
            }
            if (installPackage(lib)) {
                if (fs::exists("./lib/" + lib + "/") && fs::is_directory("./lib/" + lib + "/")) {
                    return 0;
                }
                std::cout << lib << " successfully updated.\n";
            }
            else {
                std::cerr << "Error: failed to update \"" << lib << "\" (not found in Cobalt-Package, a dependency failed, or no network access).\n";
                return 1;
            }
            return 0;
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
        else if (cmd == "-measure") {
            measure = true;
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
        else if (cmd == "-as") {
            if (i + 1 >= argc) {
                std::cerr << "Error: -name requires an argument.\n";
                return 1;
            }
            outputFile = argv[++i];
        }
        else if (cmd == "-config=debug") {
            config = 0;
        }
        else if (cmd == "-config=release") {
            config = 1;
        }
        else if (cmd == "-config=secure") {
            config = 2;
        }
        else if (cmd == "-config=more!") {
            config = 3;
        }
        else if (cmd == "--version" || cmd == "version") {
            std::cout << "Cobalt Stable v0.8 \"Azurit\"";
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
    install                Install a package via online
    remove                 Removes a package from library PATH
    update                 Updates a packafe from library PATH via online
    lists                  Lists all library in current library PATH
    build                  Compile file only
    run                    Compile and immediately run the result
    run-fast               Run the interpreter directly (experimental)
    conv                   Convert Cobalt code into C++ code
    conv-nfile             Convert Cobalt code into C++ code & prints them
    conv=asm               Convert Cobalt code into Assembly code
    conv-nfile=asm         Comvert Cobalt code into Assembly code & prints them
    -as                    Output executable name (default: out)
    -debug                 Print parsed imports/functions before generating code
    -OX                    No optimization (default)
    -O1                    Basic optimization
    -O2                    More optimization
    -O3                    Aggressive optimization (may break some code)
    -OPerformance          Same as -optimize-lvl-3
    -cpp                   Keep the generated .cpp instead of deleting it
    -config=debug          Use debug configuration (default) which is the most verbose configuration
    -config=release        Use release configuration which is the most stable configuration
    -config=secure         Use secure configuration which is the most secure configuration
    -config=more!          Use more! configuration which is the most optimized configuration
    
    version                Show version
    help                   Show this help
    
        )" << "\n";
            return 0;
        }
        else {
            std::cerr << "Unknown argument '" << cmd << "'. See --help.\n";
            return 1;
        }
    }
    
    extraFlags += " -pipe ";
    switch (config) {
        case 0: // Debug - Fast Compilation & Debugging
            optimizeCode = "-O0";
            extraFlags += " -g -g3 -DDEBUG";
            break;

        case 1: // Release - Fast & Balanced
            optimizeCode = "-O3";
            extraFlags += " -flto=auto -march=native -mtune=native";
    #ifndef _WIN32
            extraFlags += " -fno-plt";
    #endif
            break;

        case 2: // Secure - Hardened Production Build
            optimizeCode = "-O2";
            extraFlags += " -fstack-protector-strong";
    #ifndef _WIN32
            extraFlags += " -fstack-clash-protection -D_FORTIFY_SOURCE=2 -fPIE -pie";
    #else
            extraFlags += " -D_FORTIFY_SOURCE=2";
    #endif
            break;

        case 3: // High Performance / Maximum Aggressive Optimization
            optimizeCode = "-O3";
            extraFlags += " -flto=auto -march=native -mtune=native -funroll-loops "
                        "-finline-functions -fomit-frame-pointer";
    #ifndef _WIN32
            extraFlags += " -fno-plt -fstack-clash-protection -fcf-protection=full "
                        "-D_FORTIFY_SOURCE=3";
    #else
            // Windows/MinGW safe hardening flags
            extraFlags += " -fstack-protector-strong -D_FORTIFY_SOURCE=2";
    #endif
            break;

        default:
            optimizeCode = "-O2";
            break;
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
    else if (optimizeCode == " -O0 ") {
        optimizeCode = " -O0 ";
    }
    else {
        optimizeCode = " -O2 ";
    }

    if (runAndCompile) {
        if (doBuild && doRun) {
            std::cerr << "Error: pass either -build or -run, not both.\n";
            return 1;
        }
        if (!doBuild && !doRun) {
            std::cerr << "Error: pass -build <file> or -run <file>. See --help.\n";
            return 1;
        }
        if (doBuild) compile(inputFile, outputFile, isDeb, remcpp, optimizeCode, extraFlags);
        else if (doRun) {
            interpret(inputFile, outputFile, isDeb, remcpp, optimizeCode, extraFlags, measure);
        }
        else {
            std::cerr << "Error: no action specified. Use -build or -run. See --help.\n";
            return 1;
        }
    } else {
        noCompile(inputFile, outputFile, isDeb, nfile, genASM, optimizeCode, extraFlags);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    try {
        return cobaltMain(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Internal compiler error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Internal compiler error: unknown exception.\n";
        return 1;
    }
}