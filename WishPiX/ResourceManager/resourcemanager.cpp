#include "resourcemanager.h"

#include <cctype>

void Resources::normalize_path(char* path) noexcept
{
  for(; *path; ++path)
  {
    if(*path == '\\')
      *path = '/';
    else
      *path = tolower(*path);
  }
}
