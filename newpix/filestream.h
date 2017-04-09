#ifndef FILESTREAM_H
#define FILESTREAM_H

#include <fstream>
#include <vector>
#include <string>

// STL fstream with lots of << and >> overloads and independant read and write positions
class filestream : public std::fstream
{
public:
  filestream(void) : m_readpos(0), m_writepos(0) { }
  filestream(const std::string& filename, std::ios::openmode mode = std::ios::in | std::ios::out | std::ios::binary)
    : std::fstream(filename, mode), m_readpos(0), m_writepos(0) { }

  // << and >> overloads for simple insertion and extraction (specialized versions in the implementation file)
  template<typename T> filestream& operator >> (T& data)
    { return read(reinterpret_cast<char*>(&data), sizeof(T)); }

  template<typename T> filestream& operator << (const T& data)
    { return write(reinterpret_cast<const char*>(&data), sizeof(T)); }

  template<typename T> filestream& operator >> (std::vector<T>& data) // read a vector of data
    { return read(const_cast<char*>(reinterpret_cast<const char*>(data.data())), sizeof(T) * data.size()); } // naughty! (the STL does not guarantee that this will work!)

  template<typename T> filestream& operator << (const std::vector<T>& data) // write a vector of data
    { return write(reinterpret_cast<const char*>(data.data()), (sizeof(T) * data.size())); }

  // replacement functions to make read and write positions independent

  void open(const std::string& filename, std::ios::openmode mode = std::ios::in | std::ios::out | std::ios::binary)
  {
    std::fstream::open(filename, mode);
    endRead();
    endWrite();
  }

  std::streamoff tellg(void) { return m_readpos; }
  std::streamoff tellp(void) { return m_writepos; }

  filestream& seekg(std::streamoff off, std::ios::seekdir dir = std::ios::beg)
    { std::fstream::seekg(off, dir); endRead(); return *this; }

  filestream& seekp(std::streamoff off, std::ios::seekdir dir = std::ios::beg)
    { std::fstream::seekp(off, dir); endWrite(); return *this; }

  template<typename T>
  filestream& read(T* s, std::streamsize n)
  {
    beginRead();
    std::fstream::read(reinterpret_cast<char_type*>(s), n * sizeof(T) / sizeof(char_type));
    endRead();
    return *this;
  }

  template<typename T>
  filestream& write(const T* s, std::streamsize n)
  {
    beginWrite();
    std::fstream::write(reinterpret_cast<const char_type*>(s), n * sizeof(T) / sizeof(char_type));
    endWrite();
    return *this;
  }

  std::streamsize size(void)
  {
    std::fstream::seekp(0, std::ios::end);
    return std::fstream::tellp();
  }

private:
  void beginRead (void) { std::fstream::seekg(m_readpos ); }
  void beginWrite(void) { std::fstream::seekp(m_writepos); }
  void endRead (void) { m_readpos  = std::fstream::tellg(); }
  void endWrite(void) { m_writepos = std::fstream::tellp(); }

  off_type m_readpos;
  off_type m_writepos;
};

#endif // FILESTREAM_H
