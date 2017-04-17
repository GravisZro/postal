#ifndef SAKARCHIVE_H
#define SAKARCHIVE_H

#if defined(TARGET)
# include <BLUE/System.h>
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

  bool allocate(uint32_t cnt) // allocate cnt copies of T
  {
    std::shared_ptr<T>::reset(); // release old data
    if(cnt > 0) // if actually allocating data
      std::shared_ptr<T>::operator =(std::shared_ptr<T>(new T[cnt], std::default_delete<T[]>())); // allocate new data with auto deleter
    count = cnt; // save the number of copies created
    return true;
  }

  const shared_arr<T>& operator =(const shared_arr<T>& other)
  {
    std::shared_ptr<T>::operator =(other);
    count = other.count;
    return other;
  }

  T* operator =(T* ptr) // use external pointer data
  {
    std::shared_ptr<T>::operator =(std::shared_ptr<T>(ptr, [](T const*) { })); // use pointer but do not deallocate
    return ptr;
  }

  operator T*(void) const
    { return std::shared_ptr<T>::get(); } // get pointer to all data

  T& operator [](uint32_t num)
    { ASSERT(num < count); return std::shared_ptr<T>::get()[num]; } // get data

  T* operator +(uint32_t num) const
    { ASSERT(num < count); return std::shared_ptr<T>::get() + num; } // get pointer to data

  bool operator ==(const shared_arr<T>& ptr) // compare pointers first by size and then by data
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

  filedata_t(uint32_t sz = 0) // number of bytes to allocate (0 by default)
    : loaded(false)
  { data.allocate(sz); } // doesn't allocate data if sz = 0

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
    std::shared_ptr<filedata_t> filedata; // agnostic data pointer
    std::unordered_map<std::string, SAKFile>::iterator iter = m_lookup.find(filename);

    if(iter->second.filedata.expired()) // expired data pointer means that all instances of it have gone out of scope and it's memory freed
    {
      filedata = std::make_shared<T>(*new T(iter->second.length)); // make a new data type with the file size in bytes as the constructor argument
      ASSERT(filedata.operator bool());
      m_file.seekg(iter->second.offset, std::ios::beg); // seek to the file within the SAK archive
      m_file >> *filedata; // fill the data vector with the file within the SAK archive
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
