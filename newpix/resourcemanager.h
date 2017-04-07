#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "sakarchive.h"

#define SHELL_SAK_FILENAME				"res/shell/shell.sak"
#define GAME_SAK_FILENAME				"res/game/game.sak"
#define SAMPLES_SAK_FILENAME_FRMT	"res/game/%i%s%i.sak"
#define CUTSCENE_SAK_FILENAME_FRMT	"res/cutscene/cutscene%02d.sak"
#define CHECK_FOR_POSTALSD_FILENAME "res/hoods/ezmart.sak"
#define XMAS_SAK_FILENAME				"res/xmas/newgame.sak"
#define XMAS_SAK_SOUND					"res/xmas/new22050_16.sak"

namespace Resources
{
  //static std::unordered_map<std::string, SAKArchive> archives;

  static void normalize_path(std::string& path) noexcept;
}

template<typename T> // type T must inherit filedataio_t
struct Resource : std::shared_ptr<filedata_t>
{
  T& operator ->(void) const noexcept
  {
    ASSERT(*this); // must already be allocated
    return *reinterpret_cast<T*>(const_cast<uint8_t*>(get()->rawdata.data()));
  }

  virtual Resource<T>& operator=(const std::shared_ptr<filedata_t>& other) noexcept
  {
    ASSERT(!*this); // must not be previously allocated
    std::shared_ptr<filedata_t>::operator =(other);
    if(!other->loaded)
    {
      static_cast<filedataio_t*>(get())->Load();
      other->loaded = true;
    }
    return *this;
  }
};

template <class T>
bool rspGetResource(SAKArchive& archive,        // In:  Archive to be used
                    const std::string& filename,           // In:  Resource name
                    Resource<T>& res) noexcept      // Out: Resource data pointer
{
  if(archive.fileExists(filename))
    return false;

  res = archive.getFile(filename);
  return res;
}

template <class T>
void rspReleaseResource(Resource<T>& res) noexcept  // In:  Pointer to resource
{
  res.reset();
}

#endif // RESOURCEMANAGER_H
