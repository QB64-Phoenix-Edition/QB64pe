#ifndef INCLUDE_LIBQB_FILEPATH_H
#define INCLUDE_LIBQB_FILEPATH_H

#include <string>

struct qbs;

// Takes a path + filename, and returns just the filename portion
// Returns either NULL or empty string if it has none.
const char *filepath_get_filename(const char *path);

// Takes a filename and returns just the extension at the end
// Returns either NULL or empty string if it has none.
const char *filepath_get_extension(const char *fliename);

// Returns true if the path is to a file that matches the provided extension
bool filepath_has_extension(const char *path, const char *extension);

// The following overloaded functions changes the path separators in path based on the OS (path is modified)
const char *filepath_fix_directory(char *path);
const char *filepath_fix_directory(qbs *path);
const char *filepath_fix_directory(std::string &path);

// Splits a file path into directory and file name
void filepath_split(const std::string &filePath, std::string &directory, std::string &fileName);

// Joins a directory and file name into a file path
void filepath_join(std::string &filePath, const std::string &directory, const std::string &fileName);

#endif
