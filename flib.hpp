#ifndef FLIB_HPP
#define FLIB_HPP

#include <string>

// Removes a trailing '\r' left over from Windows-style line endings.
void stripCR(std::string& line);

// Strips a trailing "# comment", ignoring '#' that appears inside a string.
void stripComment(std::string& line);

/* Reads sourcePath and returns its full text with every
 `@import "file.cb"` line replaced by the contents of that file
  (recursively, so an imported file can itself import others).
  A missing file prints a warning and is skipped rather than failing
  the whole build. `@import <lib>` lines are left untouched -- those
  are handled later by the lexer/parser as library imports. */
std::string preprocessFile(const std::string& sourcePath);
std::string findFile(const std::string& filename);

#endif