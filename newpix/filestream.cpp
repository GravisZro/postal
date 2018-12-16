#include "filestream.h"

// template specialization

template<> filestream& filestream::operator >> (std::string& data) // read a NULL terminated string
{
  beginRead();
  std::getline(*this, data, '\0');
  endRead();
  return *this;
}

template<> filestream& filestream::operator << (const std::string& data) // write a NULL terminated string
  { return write(data.c_str(), data.size() + 1); }

#if 0

#include <string.h> // for strnlen

// template specialization

template<> filestream& filestream::operator >> (std::string& data) // read a NULL terminated string
{
  char buffer[4096] = { 0 };
  ssize_t count = ::pread(m_fd, buffer, sizeof(buffer), m_readpos);
  if(count > 0)
  {
    size_t len = ::strnlen(buffer, sizeof(buffer));
    data.append(buffer, len);
    addRead(len);
  }
  return *this;
}

template<> filestream& filestream::operator << (const std::string& data) // write a NULL terminated string
  { return write(data.c_str(), data.size() + 1); }
#endif
