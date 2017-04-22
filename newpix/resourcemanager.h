#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <string>
#include <memory>

#include "sakarchive.h"

extern SAKArchive g_GameSAK;

template<typename T>
static bool rspGetResource(SAKArchive& archive,                   // In:  Archive to be used
                           const std::string& filename,           // In:  Resource name
                           std::shared_ptr<T>& res) noexcept      // Out: Resource data pointer
{
  if(!archive.fileExists(filename))
  {
    TRACE("Couldn't open resource: %s\n", filename.c_str());
    return false;
  }

  res = archive.getFile<T>(filename);
  reinterpret_cast<T*>(res.get())->load();
  return res.operator bool();
}

template<typename T>
static void rspReleaseResource(std::shared_ptr<T>& res) noexcept  // In:  Resource data pointer
{
  res.reset();
}

#endif // RESOURCEMANAGER_H
