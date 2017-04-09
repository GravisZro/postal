#ifndef SAKARCHIVE_H
#define SAKARCHIVE_H

#if defined(TARGET)
# include <RSPiX/BLUE/System.h>
#else
# include <cassert>
# define ASSERT(x) assert(x)
#endif


#include "filestream.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <list>
#include <cstdint>

struct filedata_t
{
  bool loaded;
  bool allocated;
  uint8_t* dataptr;
  uint32_t size;

  filedata_t(uint32_t sz)
    : loaded(false), allocated(true), dataptr(new uint8_t[sz]), size(sz) { }

  filedata_t(void)
    : loaded(false), allocated(false), dataptr(nullptr), size(0) { }

  ~filedata_t(void)
  {
    if(allocated)
      delete[] dataptr;
    dataptr = nullptr;
  }

  virtual void load (void) { loaded = true; }
};

class SAKArchive // "Swiss Army Knife" Archive
{
public:
  // You MUST provide a valid filename of the SAK archive to open
  SAKArchive(const std::string& archivename);

  // Test if a file exists.  Other operations assume the file exists.
  bool fileExists(const std::string& filename) const { return m_lookup.find(filename) != m_lookup.end(); }

  // Returns a pointer to allocated memory.  Memory is allocated/freed ad hoc. (file must exist)
  std::shared_ptr<filedata_t> getFile(const std::string& filename);

protected:
  struct SAKFile
  {
    uint32_t offset;
    uint32_t length;
    std::weak_ptr<filedata_t> filedata;

    SAKFile(uint32_t off, uint32_t len) : offset(off), length(len) { }
  };

  std::unordered_map<std::string, SAKFile> m_lookup;
  filestream m_file;
};

//=======================================================================================
#if !defined(TARGET)

// Extended operations not required for the game
class SAKArchiveExt : public SAKArchive
{
public:
  // You MUST provide a valid filename of the SAK archive to open
  SAKArchiveExt(const std::string& archivename) : SAKArchive(archivename), m_archivename(archivename) { }

  std::string archiveName(void) const { return m_archivename; }

  // Returns an list of sorted filename strings
  std::list<std::string> listFiles(void) const;

  // Returns the size of the file. (file must exist)
  uint32_t fileSize(const std::string& filename) const;

  // Returns the offset of the file within the archive. (file must exist)
  uint32_t fileOffset(const std::string& filename) const;

  bool removeFile(const std::string& filename);
  bool appendFile(const std::string& filename, const uint8_t* filedata, uint32_t datalength);

private:
  bool shiftEntries(int32_t index_diff, int32_t offset_diff, uint32_t offset);
  bool resizeFile(uint32_t new_size);

  std::string m_archivename;
};

#endif

#endif // SAKARCHIVE_H
