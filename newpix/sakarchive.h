#ifndef SAKARCHIVE_H
#define SAKARCHIVE_H

#if defined(TARGET) || defined(_MSC_VER)
# include <BLUE/System.h>
#else
# include <cassert>
# define ASSERT(x) assert(x)
#endif

#include "filestream.h"
#include "sharedarray.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>


// abstract struct
class filedata_t
{
  friend class filestream; // allow encapsulation to be violated for speed
private:
  bool m_loaded;

protected:
  shared_arr<uint8_t> data;

public:
  filedata_t(uint32_t sz = 0) // number of bytes to allocate (0 by default)
    : m_loaded(false)
  { data.allocate(sz); } // doesn't allocate data if sz = 0

  bool isLoaded(void) const { return m_loaded; }
  void setLoaded(void) { m_loaded = true; }

  void setData(uint8_t* ndata, uint32_t sz)
  {
    data = ndata;
    data.setSize(sz);
  }

  uint32_t dataSize(void) const { return data.size(); }

  virtual void load (void) = 0;
};

// forward declaration of specialization
template<> filestream& filestream::operator >> (filedata_t& fdata);
template<> filestream& filestream::operator << (const filedata_t& fdata);


class SAKArchive // "Swiss Army Knife" Archive
{
public:
  // You MUST provide a valid filename of the SAK archive to open
  SAKArchive(const std::string& archivename);

  // Test if a file exists.  Other operations assume the file exists.
  bool fileExists(const std::string& filename) const { return m_lookup.find(filename) != m_lookup.end(); }

  // Returns a pointer to allocated memory.  Memory is allocated/freed ad hoc. (file must exist)
  template<typename T>
  std::shared_ptr<T> getFile(const std::string& filename)
  {
    ASSERT(fileExists(filename)); // The file must exist in this SAK archive
    std::shared_ptr<filedata_t> filedata; // agnostic data pointer
    std::unordered_map<std::string, SAKFile>::iterator iter = m_lookup.find(filename);

    if(iter->second.filedata.expired()) // expired data pointer means that all instances of it have gone out of scope and it's memory freed
    {
      filedata = std::make_shared<T>(*new T(iter->second.length)); // make a new data type with the file size in bytes as the constructor argument
      ASSERT(filedata.operator bool());
      m_file.seekg(iter->second.offset, std::ios::beg); // seek to the file within the SAK archive
      m_file >> *filedata; // fill filedata_t with the file within the SAK archive (specialized template functions in implementation file)
      iter->second.filedata = filedata; // store a weak copy so that we can test if it's in use later
    }
    else // data pointer is still valid
      filedata = iter->second.filedata.lock(); // make a shared pointer copy!
    return std::static_pointer_cast<T, filedata_t>(filedata); // return as the shared pointer type that we want
  }

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
#if !defined(TARGET) && !defined(_MSC_VER)
#include <list>

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
