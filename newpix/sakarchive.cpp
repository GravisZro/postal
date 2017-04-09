#include "sakarchive.h"


template<> filestream& filestream::operator >> (filedata_t& data) // read filedata_t data
{
  beginRead();
  read(data.dataptr, data.size);
  endRead();
  return *this;
}

template<> filestream& filestream::operator << (const filedata_t& data) // write filedata_t data
  { return write(data.dataptr, data.size); }



template<typename A, typename B>
constexpr bool pair_second_greater(const std::pair<A, B>& x, const std::pair<A, B>& y)
  { return x.second > y.second; }

namespace consts
{
  const uint32_t magic_number = 0x204B4153; // "SAK "
  const uint32_t version = 1;
}

SAKArchive::SAKArchive(const std::string& archivename)
  : m_file(archivename)
{
  uint32_t magic_number, version;
  uint16_t file_count;

  ASSERT(m_file.is_open()); // if the file exists, it will be open, so it must be.

  m_file >> magic_number;
  ASSERT(magic_number == consts::magic_number); // "SAK "

  m_file >> version;
  ASSERT(version == consts::version); // only one version exists, so make sure we have it

  m_file >> file_count;
  ASSERT(file_count > 0); // having 0 files is pointless and wouldn't parse properly

  std::string filename;
  uint32_t offset;
  std::list<std::pair<std::string, uint32_t>> list; // temporary storage until the file length is computable

  do // read all filename and file offset pairs in the header
  {
    m_file >> filename; // read a NULL terminated string
    m_file >> offset; // 4 byte offset limits the archive size to 4GB
    list.emplace_back(std::make_pair(filename, offset)); // store data in temporary list
  } while(--file_count);

  m_file.seekg(0, std::ios::end); // seek to the end of the file
  offset = m_file.tellg(); // file length is used to compute the last file length

  list.sort(pair_second_greater<std::string, uint32_t>);

  for(auto pos = list.begin(); pos != list.end(); ++pos) // compute file lengths and store info in the hash map
  {
    m_lookup.emplace(std::make_pair(pos->first, SAKFile(pos->second, offset - pos->second))); // store data (subtract from previous offset to compute the length)
    offset = pos->second; // store current offset for next time around
  }
}

std::shared_ptr<filedata_t> SAKArchive::getFile(const std::string& filename)
{
  ASSERT(fileExists(filename)); // The file must exist in this SAK archive
  std::shared_ptr<filedata_t> filedata;
  std::unordered_map<std::string, SAKFile>::iterator iter = m_lookup.find(filename);

  if(iter->second.filedata.expired()) // expired data pointer means that all instances of it have gone out of scope and it's memory freed
  {
    filedata = std::make_shared<filedata_t>(*new filedata_t(iter->second.length)); // make a new data array with the file size
    m_file.seekg(iter->second.offset, std::ios::beg); // seek to the file within the SAK archive
    m_file >> filedata; // fill the data vector with the file within the SAK archive
    iter->second.filedata = filedata; // store a weak copy so that we can test if it's in use later
  }
  else // data pointer is still valid
    filedata = static_cast<std::shared_ptr<filedata_t>>(iter->second.filedata); // copy data pointer
  return filedata;
}

//=======================================================================================
#if !defined(TARGET)

#define FILE_COUNT_OFFSET  8

template<typename A, typename B>
constexpr bool pair_first_less(const std::pair<A, B>& x, const std::pair<A, B>& y)
  { return x.first < y.first; }

std::list<std::string> SAKArchiveExt::listFiles(void) const
{
  std::list<std::string> filelist;
  for(auto& pair : m_lookup)
    filelist.push_back(pair.first);
  filelist.sort();
  return filelist;
}

uint32_t SAKArchiveExt::fileSize(const std::string& filename) const
{
  ASSERT(fileExists(filename)); // The file must exist in this SAK archive
  return m_lookup.find(filename)->second.length;
}

uint32_t SAKArchiveExt::fileOffset(const std::string& filename) const
{
  ASSERT(fileExists(filename)); // The file must exist in this SAK archive
  return m_lookup.find(filename)->second.offset;
}

bool SAKArchiveExt::removeFile(const std::string& filename)
{
  ASSERT(fileExists(filename)); // The file must exist in this SAK archive

  std::pair<std::string, SAKFile> fileinfo = *m_lookup.find(filename);
  m_lookup.erase(fileinfo.first);

  uint32_t old_filesize = m_file.size();
  int32_t entry_size = fileinfo.first.size() + 1 + sizeof(uint32_t);
  if(!shiftEntries(0 - entry_size,
                   0 - fileinfo.second.length,
                   fileinfo.second.offset))
    return false;

  return resizeFile(old_filesize - entry_size - fileinfo.second.length);
}

bool SAKArchiveExt::appendFile(const std::string& filename, const uint8_t* filedata, uint32_t datalength)
{
  uint32_t old_filesize = m_file.size();

  auto result = m_lookup.emplace(std::make_pair(filename, SAKFile(UINT32_MAX, datalength)));
  if(!result.second)
    return false;
  const std::pair<std::string, SAKFile>& fileinfo = *result.first;

  int32_t entry_size = fileinfo.first.size() + 1 + sizeof(uint32_t);
  uint32_t new_offset = old_filesize + entry_size;

  if(!shiftEntries(entry_size,
                   fileinfo.second.length,
                   fileinfo.second.offset))
    return false;


  std::string tmp_filename;
  uint32_t tmp_offset = 0;
  uint16_t file_count = 0;

  m_file.seekg(FILE_COUNT_OFFSET);
  m_file >> file_count;

  for(uint16_t i = 0; i < file_count; ++i)
  {
    m_file >> tmp_filename;
    m_file >> tmp_offset;
    if(tmp_offset == UINT32_MAX)
    {
      m_file.seekp(m_file.tellg() - sizeof(uint32_t));
      m_file << new_offset;
      break;
    }
  }

  m_file.seekp(new_offset);
  m_file.write(filedata, fileinfo.second.length);
  return true;
}

bool SAKArchiveExt::shiftEntries(int32_t index_diff, int32_t offset_diff, uint32_t offset)
{
  assert(index_diff != 0);
  struct oldnewlength
  {
    const std::string filename;
    uint32_t old_offset;
    uint32_t new_offset;
    uint32_t length;
    bool operator < (const oldnewlength& onl) const { return old_offset < onl.old_offset; }
    bool operator > (const oldnewlength& onl) const { return old_offset > onl.old_offset; }
    oldnewlength(const std::string& fn, uint32_t o, uint32_t n, uint32_t l) : filename(fn), old_offset(o), new_offset(n), length(l) { }
  };

  std::list<std::pair<std::string, uint32_t>> names_offsets;
  std::list<oldnewlength> shift_data;

  for(auto& iter : m_lookup)
  {
    uint32_t original_offset = iter.second.offset;
    if(original_offset < UINT32_MAX) // do not modify identifier/bad address
    {
      iter.second.offset += index_diff;
      if(iter.second.offset > offset)
        iter.second.offset += offset_diff;
    }

    shift_data.emplace_back(iter.first, original_offset, iter.second.offset, iter.second.length); // save original offset, new offset and length
    names_offsets.emplace_back(std::make_pair(iter.first, iter.second.offset)); // save name and offset
  }

  if(index_diff > 0) // add file
    shift_data.sort(std::greater<oldnewlength>());
  else // remove file
    shift_data.sort(std::less<oldnewlength>());

  std::vector<uint8_t> data_buffer;
  for(auto& iter : shift_data)
  {
    if(iter.old_offset != iter.new_offset) // if file needs to be moved
    {
      m_file.seekg(iter.old_offset); // move get to original offset
      m_file.seekp(iter.new_offset); // move put to new offset

      data_buffer.resize(iter.length);
      m_file >> data_buffer; // get file data
      m_file << data_buffer; // put file data
    }
  }

  names_offsets.sort(pair_first_less<std::string, uint32_t>);
  m_file.seekp(FILE_COUNT_OFFSET, std::ios::beg); // seek to file count data

  // write file index
  m_file << uint16_t(m_lookup.size());
  for(auto& iter : names_offsets)
    m_file << iter.first << iter.second;

  return true;
}

bool SAKArchiveExt::resizeFile(uint32_t new_size)
{
  std::vector<uint8_t> data_buffer(m_file.size());
  m_file.seekg(0);
  m_file >> data_buffer;
  data_buffer.resize(new_size);
  m_file.close();
  m_file.open(m_archivename, std::ios::out | std::ios::binary | std::ios::trunc);
  m_file << data_buffer;
  m_file.close();
  m_file.open(m_archivename, std::ios::in | std::ios::out | std::ios::binary);
  return m_file.good();
}

#endif
