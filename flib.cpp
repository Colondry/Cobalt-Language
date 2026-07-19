#include "flib.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <set>

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
        } else if (!inString && line[i] == '#') {
            line.erase(i);
            return;
        }
    }
}

// Returns the quoted filename in a `@import "file.cb"` line, or "" if the
// line (after comment-stripping) isn't that form.
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
        } else {
            stripComment(line);
            result << line << "\n";
        }
    }
    return result.str();
}

// Searches for a file by name. On Windows this scans every drive letter
// (slow -- only use it when you genuinely don't know where a file is).
// On other platforms it only looks in the current directory, since that's
// the only case this project currently needs. Prefer just using a known
// relative path directly when you have one (see main.cpp's interpret(),
// which used to call this on the .cpp it had just written itself).
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