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


template<typename T>
struct shared_arr : std::shared_ptr<T>
{
  uint32_t count;

  bool allocate(uint32_t cnt)
  {
    std::shared_ptr<T>::reset();
    if(cnt > 0)
    {
      std::shared_ptr<T>::operator =(std::shared_ptr<T>(new T[cnt], std::default_delete<T[]>()));
      //std::make_shared<T>(new T[cnt], std::default_delete<T[]>());
      //std::shared_ptr<T>::operator =(
    }
    count = cnt;
    return true;
  }

  T* operator =(T* ptr)
  {
    std::shared_ptr<T>::operator =(std::shared_ptr<T>(ptr, [this](T const*) { count = 0; }));
    return ptr;
  }

  operator T*(void)
    { return std::shared_ptr<T>::get(); }

  T& operator [](uint32_t num)
    { ASSERT(num < count); return std::shared_ptr<T>::get()[num]; }

  bool operator =(const shared_arr<T>& ptr)
  {
    return count == ptr.count &&
        (!count || std::memcmp(std::shared_ptr<T>::get(), ptr.get(), sizeof(T) * count) == SUCCESS);  // memory does match))
  }
};

// abstract struct
struct filedata_t
{
  bool loaded;
  shared_arr<uint8_t> data;

  filedata_t(uint32_t sz = 0)
    : loaded(false)
  { data.allocate(sz); }

  virtual void load (void) = 0;
};

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
    std::shared_ptr<filedata_t> filedata;
    std::unordered_map<std::string, SAKFile>::iterator iter = m_lookup.find(filename);

    if(iter->second.filedata.expired()) // expired data pointer means that all instances of it have gone out of scope and it's memory freed
    {
      filedata = std::make_shared<T>(*new T(iter->second.length)); // make a new data array with the file size
      ASSERT(filedata.operator bool());
      m_file.seekg(iter->second.offset, std::ios::beg); // seek to the file within the SAK archive
      m_file >> *filedata; // fill the data vector with the file within the SAK archive
      iter->second.filedata = filedata; // store a weak copy so that we can test if it's in use later
    }
    else // data pointer is still valid
      filedata = iter->second.filedata.lock();
    return std::static_pointer_cast<T, filedata_t>(filedata);
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
/*
template<typename T>
std::shared_ptr<T> SAKArchive::getFile(const std::string& filename)
{
  ASSERT(fileExists(filename)); // The file must exist in this SAK archive
  std::shared_ptr<T> filedata;
  std::unordered_map<std::string, SAKFile>::iterator iter = m_lookup.find(filename);

  if(iter->second.filedata.expired()) // expired data pointer means that all instances of it have gone out of scope and it's memory freed
  {
    filedata = std::make_shared<T>(*new T(iter->second.length)); // make a new data array with the file size
    ASSERT(filedata.operator bool());
    m_file.seekg(iter->second.offset, std::ios::beg); // seek to the file within the SAK archive
    m_file >> *filedata; // fill the data vector with the file within the SAK archive
    iter->second.filedata = std::static_pointer_cast<T, filedata_t>(filedata); // store a weak copy so that we can test if it's in use later
  }
  else // data pointer is still valid
    filedata = std::static_pointer_cast<filedata_t, T>(iter->second.filedata.lock()); // copy data pointer
  return filedata;
}
*/
#endif // SAKARCHIVE_H
