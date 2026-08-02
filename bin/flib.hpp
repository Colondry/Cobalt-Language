#ifndef FLIB_HPP
#define FLIB_HPP

#include <string>
#include <vector>


void stripCR(std::string& line);
// Strips "# ...."
void stripComment(std::string& line);
std::string preprocessFile(const std::string& sourcePath);
std::string findFile(const std::string& filename);
void setExecutablePath(const std::string& argv0);
std::string getExeDir();
std::string findLibraryFile(const std::string& name, const std::string& extension, const std::string& inputFileDir);
std::string findLibraryDir(const std::string& name, const std::string& inputFileDir);
std::vector<std::string> listCppFilesIn(const std::string& dir);
std::string findLibraryLinkFlags(const std::string& bundleDir);

#endif