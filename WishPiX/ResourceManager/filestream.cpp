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
