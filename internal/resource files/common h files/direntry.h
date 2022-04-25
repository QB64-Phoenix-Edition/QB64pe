
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
 
const int IS_DIR_FLAG = 1, IS_FILE_FLAG = 2;
 
DIR *pdir;
struct dirent *next_entry;
struct stat statbuf1;
 
char current_dir[FILENAME_MAX];
#ifdef QB64_WINDOWS
  #define GetCurrentDir _getcwd
#else
  #define GetCurrentDir getcwd
#endif
 
int load_dir (char * path) {
  struct dirent *pent;
  struct stat statbuf1;
//Open current directory
pdir = opendir(path);
if (!pdir) {
return 0; //Didn't open
}
return -1;
}
 
int has_next_entry () {
  next_entry = readdir(pdir);
  if (next_entry == NULL) return -1;
   
  stat(next_entry->d_name, &statbuf1);
  return strlen(next_entry->d_name);
}
 
void get_next_entry (char * nam, int * flags, int * file_size) {
  strcpy(nam, next_entry->d_name);
  if (S_ISDIR(statbuf1.st_mode)) {
    *flags = IS_DIR_FLAG;
  } else {
    *flags = IS_FILE_FLAG;
  }
  *file_size = statbuf1.st_size;
  return ;
}
 
void close_dir () {
  closedir(pdir);
  pdir = NULL;
  return ;
}
 
int current_dir_length () {
  GetCurrentDir(current_dir, sizeof(current_dir));
  return strlen(current_dir);
}
 
void get_current_dir(char *dir) {
  memcpy(dir, current_dir, strlen(current_dir));
  return ;
}
