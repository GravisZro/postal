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

// forward declaration of specialization
template<> filestream& filestream::operator >> (std::string& data);
template<> filestream& filestream::operator << (const std::string& data);

#if 0

#include <fstream>
#include <vector>
#include <string>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

// STL fstream with lots of << and >> overloads and independant read and write positions
class filestream
{
public:
  filestream(void) : m_fd(-1), m_size(0), m_readpos(0), m_writepos(0) { }
  filestream(const std::string& filename, std::ios::openmode mode = std::ios::in | std::ios::out)
    : m_size(0), m_readpos(0), m_writepos(0) { open(filename, mode); }

  ~filestream(void)
  {
    if(is_open())
      close();
  }

  // << and >> overloads for simple insertion and extraction (specialized versions in the implementation file)
  template<typename T> filestream& operator >> (T& data)
    { return read(reinterpret_cast<char*>(&data), sizeof(T)); }

  template<typename T> filestream& operator << (const T& data)
    { return write(reinterpret_cast<const char*>(&data), sizeof(T)); }

  template<typename T> filestream& operator >> (std::vector<T>& data) // read a vector of data
    { return read(const_cast<char*>(reinterpret_cast<const char*>(data.data())), sizeof(T) * data.size()); } // naughty! (the STL does not guarantee that this will work!)

  template<typename T> filestream& operator << (const std::vector<T>& data) // write a vector of data
    { return write(reinterpret_cast<const char*>(data.data()), (sizeof(T) * data.size())); }

  constexpr bool is_open(void) const { return m_fd >= 0; }

  void open(const std::string& filename, std::ios::openmode mode = std::ios::in | std::ios::out)
  {

    int oflag = 0;
    oflag |= (mode & std::ios::in ) ? O_RDONLY : 0;
    oflag |= (mode & std::ios::out) ? O_WRONLY : 0;

    m_fd = ::open(filename.c_str(), oflag);
    if(is_open())
    {
      m_size = ::lseek(m_fd, 0, SEEK_END);
      ::lseek(m_fd, 0, SEEK_SET);
      m_readpos = 0;
      m_writepos = 0;
    }
    else
    {
      char buf[4096] = { 0 };
      m_errno = errno;
      fprintf(stderr, "cwd: \"%s\"\n", ::getcwd(buf, sizeof(buf)));
      fprintf(stderr, "failed to open %s - error: \"%s\"\n", filename.c_str(), std::strerror(errno));
      fflush(stderr);
    }
  }

  void close(void)
  {
    ::close(m_fd);
    m_fd = -1;
    m_size = 0;
    m_errno = 0;
  }

  constexpr std::streamoff tellg(void) const { return m_readpos; }
  constexpr std::streamoff tellp(void) const { return m_writepos; }
  constexpr int error(void) const { return m_errno; }

  filestream& seekg(std::streamoff off, std::ios::seekdir dir = std::ios::beg)
  {
    switch(dir)
    {
      case std::ios::beg: m_readpos = off; break;
      case std::ios::end: m_readpos = m_size - off; break;
      case std::ios::cur: m_readpos += off; break;
      default:
        break;
    }
    return *this;
  }

  filestream& seekp(std::streamoff off, std::ios::seekdir dir = std::ios::beg)
  {
    switch(dir)
    {
      case std::ios::beg: m_writepos = off; break;
      case std::ios::end: m_writepos = m_size - off; break;
      case std::ios::cur: m_writepos += off; break;
      default:
        break;
    }
    return *this;
  }

  template<typename T>
  filestream& read(T* s, std::streamsize n)
  {
    addRead(::pread(m_fd, reinterpret_cast<void*>(s), n * sizeof(T) / sizeof(char), m_readpos));
    return *this;
  }

  template<typename T>
  filestream& write(const T* s, std::streamsize n)
  {
    addWrite(::pwrite(m_fd, reinterpret_cast<const void*>(s), n * sizeof(T) / sizeof(char), m_writepos));
    return *this;
  }

  constexpr std::streamsize size(void) const { return m_size; }

private:
  inline void addRead (ssize_t off) { addOrError(m_readpos , off); }
  inline void addWrite(ssize_t off) { addOrError(m_writepos, off); }

  inline void addOrError(off_t& pos, ssize_t off)
  {
    if(off < 0)
      pos += off;
    else
      m_errno = errno;
  }

  int m_fd;
  std::streamsize m_size;
  off_t m_readpos;
  off_t m_writepos;
  int m_errno;
};

// forward declaration of specialization
template<> filestream& filestream::operator >> (std::string& data);
template<> filestream& filestream::operator << (const std::string& data);
#endif

#endif // FILESTREAM_H
