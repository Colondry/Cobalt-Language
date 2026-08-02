#include "flib.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <set>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

void stripCR(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

void stripComment(std::string& line) {
    bool inString = false;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == '"') {
            inString = !inString;
        }
        else if (!inString && line[i] == '#') {
            line.erase(i);
            return;
        }
    }
}

static std::string importedFileName(const std::string& line) {
    size_t atPos = line.find('@');
    if (atPos == std::string::npos) return "";
    size_t importPos = line.find("import", atPos);
    if (importPos == std::string::npos) return "";
    size_t firstQuote = line.find('"', importPos);
    if (firstQuote == std::string::npos) return "";
    size_t secondQuote = line.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) return "";
    return line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

static std::string preprocessRecursive(const std::string& path, std::set<std::string>& visited) {
    if (visited.count(path)) {
        std::cerr << "Warning: circular @import of \"" << path << "\", skipping.\n";
        return "";
    }
    visited.insert(path);

    std::ifstream in(path);
    if (!in) {
        std::cerr << "Warning: cannot open imported file \"" << path << "\", skipping.\n";
        return "";
    }

    std::ostringstream result;
    std::string line;
    while (std::getline(in, line)) {
        stripCR(line);
        std::string importCheckLine = line;
        stripComment(importCheckLine);
        std::string importedFile = importedFileName(importCheckLine);

        if (!importedFile.empty()) {
            result << preprocessRecursive(importedFile, visited);
        }
        else {
            stripComment(line);
            result << line << "\n";
        }
    }
    return result.str();
}

std::string findFile(const std::string& filename)
{
#ifdef _WIN32
    DWORD drives = GetLogicalDrives();

    for (char drive = 'A'; drive <= 'Z'; drive++)
    {
        if (!(drives & (1 << (drive - 'A'))))
            continue;

        std::string root;
        root += drive;
        root += ":/\\";

        try
        {
            for (const auto& entry :
                fs::recursive_directory_iterator(
                    root,
                    fs::directory_options::skip_permission_denied))
            {
                if (!entry.is_regular_file())
                    continue;

                if (entry.path().filename() == filename)
                    return entry.path().string();
            }
        }
        catch (...)
        {
            // Ignore drives that can't be searched
        }
    }

    return "";
#else
    if (fs::exists(filename)) return fs::absolute(filename).string();
    return "";
#endif
}

std::string preprocessFile(const std::string& sourcePath) {
    std::set<std::string> visited;
    return preprocessRecursive(sourcePath, visited);
}

static std::string g_exePath;

std::string findLibraryDir(const std::string& name, const std::string& inputFileDir) {
    std::vector<fs::path> candidates;

    std::string exeDir = getExeDir();
    if (!exeDir.empty()) {
        candidates.push_back(fs::path(exeDir) / "lib" / name);
    }
    if (!inputFileDir.empty()) {
        candidates.push_back(fs::path(inputFileDir) / name);
    }

    for (const fs::path& candidate : candidates) {
        std::error_code ec;
        if (fs::exists(candidate, ec) && !ec && fs::is_directory(candidate, ec) && !ec) {
            fs::path abs = fs::absolute(candidate, ec);
            return ec ? candidate.string() : abs.string();
        }
    }
    return "";
}

std::vector<std::string> listCppFilesIn(const std::string& dir) {
    std::vector<std::string> result;
    if (dir.empty()) return result;

    std::error_code ec;
    if (!fs::is_directory(dir, ec) || ec) return result;

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".cpp") continue;

        std::error_code absEc;
        fs::path abs = fs::absolute(entry.path(), absEc);
        result.push_back(absEc ? entry.path().string() : abs.string());
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::string findLibraryLinkFlags(const std::string& bundleDir) {
    if (bundleDir.empty()) return "";
    fs::path linkFile = fs::path(bundleDir) / "link.txt";

    std::error_code ec;
    if (!fs::exists(linkFile, ec) || ec) return "";

    std::ifstream in(linkFile);
    if (!in) return "";
    std::ostringstream contents;
    contents << in.rdbuf();

    std::string flags = contents.str();
    // Trim leading/trailing whitespace (including trailing newline).
    size_t start = flags.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = flags.find_last_not_of(" \t\r\n");
    return flags.substr(start, end - start + 1);
}

void setExecutablePath(const std::string& argv0) {
    g_exePath = argv0;
}

std::string getExeDir() {
    if (g_exePath.empty()) return "";
    std::error_code ec;
    fs::path resolved = fs::absolute(g_exePath, ec);
    if (ec) return "";
    return resolved.parent_path().string();
}

std::string findLibraryFile(const std::string& name, const std::string& extension, const std::string& inputFileDir) {
    std::vector<fs::path> candidates;

    std::string exeDir = getExeDir();
    if (!exeDir.empty()) {
        candidates.push_back(fs::path(exeDir) / "lib" / (name + extension));
    }
    if (!inputFileDir.empty()) {
        candidates.push_back(fs::path(inputFileDir) / (name + extension));
    }
    candidates.push_back(fs::path(name + extension)); // current working directory

    for (const fs::path& candidate : candidates) {
        std::error_code ec2;
        if (fs::exists(candidate, ec2) && !ec2) {
            fs::path abs = fs::absolute(candidate, ec2);
            return ec2 ? candidate.string() : abs.string();
        }
    }
    return "";
}