#ifndef SAKARCHIVE_H
#define SAKARCHIVE_H

#include "filestream.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <list>
#include <cstdint>

#if !defined(TARGET)
# include <RSPiX/BLUE/System.h>
#endif

class SAKArchive // "Swiss Army Knife" Archive
{
public:
  // You MUST provide a valid filename of the SAK archive to open
  SAKArchive(const char* archivename);

  // Test if a file exists.  Other operations assume the file exists.
  bool fileExists(const char* filename) const { return m_lookup.find(filename) != m_lookup.end(); }

  // Returns a pointer to allocated memory.  Memory is allocated/freed ad hoc. (file must exist)
  std::shared_ptr<std::vector<uint8_t>> getFile(const char* filename);

protected:
  struct SAKFile
  {
    uint32_t offset;
    uint32_t length;
    std::weak_ptr<std::vector<uint8_t>> data;

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
  SAKArchiveExt(const char* archivename) : SAKArchive(archivename), m_archivename(archivename) { }

  std::string archiveName(void) const { return m_archivename; }

  // Returns an list of sorted filename strings
  std::list<std::string> listFiles(void) const;

  // Returns the size of the file. (file must exist)
  uint32_t fileSize(const char* filename) const;

  // Returns the offset of the file within the archive. (file must exist)
  uint32_t fileOffset(const char* filename) const;

  bool removeFile(const char* filename);
  bool appendFile(const char* filename, const uint8_t* filedata, uint32_t datalength);

private:
  bool shiftEntries(int32_t index_diff, int32_t offset_diff, uint32_t offset);
  bool resizeFile(uint32_t new_size);

  std::string m_archivename;
};

#endif

#endif // SAKARCHIVE_H
