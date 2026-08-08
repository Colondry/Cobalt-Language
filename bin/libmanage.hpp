#ifndef LIBM
#define LIBM

#include <string>
#include <vector>

bool downloadGitHubLibrary(
    const std::string& repo,
    const std::string& remotePath,
    const std::string& localPath
);

// Extracts "<fileName>.7z" (via the bundled 7-Zip) into outputDir.
bool extract(const std::string& fileName, const std::string& outputDir);

struct RFile { std::string name; std::string dest; }; // name: a bare Cobalt-Package rfile name, or a full http(s):// URL
struct Requirements {
    std::vector<std::string> libs;  // further Cobalt-Package libraries to install
    std::vector<RFile> rfiles;      // standalone archives to fetch + extract
};

Requirements parseRequirementsText(const std::string& text);
Requirements parseRequirementsFile(const std::string& path);

bool installPackage(const std::string& libName);

#endif