#include "sakarchive.h"

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdint>

#include <sys/stat.h>
#include <unistd.h>

class anyfile : public filedata_t
{
public:
  using filedata_t::filedata_t;
  using filedata_t::data;
  void load (void){}
};

bool path_exists(const char* path)
{
  static struct stat state;
  return stat(path, &state) == 0;
}

// make directory and parents
static int mkdir_p(const std::string& name, mode_t mode)
{
  char* pname = new char[name.size() + 1];
  char* path = std::strcpy(pname, name.c_str());
  do
  {
    if (*path == '/' || *path == '\\')
    {
      *path = '\0';
      if(!path_exists(pname)) // if it doesn't exist
        mkdir(pname, mode); // allowed to fail some of the time, so don't bail out
      *path = '/';
    }
  } while(*path++);
  delete pname;
  return 0;
}

int main(int argc, char **argv)
{
  std::list<std::string> list;
  filestream file;
  std::vector<uint8_t> data;
  std::string argfilename;
  SAKArchiveExt* archive = nullptr;
  int action = 0;

  for(int opt = 0; opt != -1 && action != -1; opt = getopt(argc, argv, ":f:lx:r:a:"))
  {
    switch (opt)
    {
      case 0:
        continue;

      case 'f':
        archive = new SAKArchiveExt(std::string(optarg));
        break;

      case 'l':
        action = opt;
        break;

      case 'x':
      case 'r':
      case 'a':
        argfilename = optarg;
        action = opt;
        break;

      case ':':
        switch (optopt)
        {
          case 'x':
            action = optopt;
            break;
          default:
            action = -1;
            break;
        }
        break;

      default:
        action = -1;
        break;
    }
  }

  if(archive == nullptr)
    action = -1;

  switch(action)
  {
    case 'l':
    {
      int maxw = 0;
      for(const std::string& filename : archive->listFiles())
        if(filename.size() > maxw)
          maxw = filename.size();

      std::cout << std::setw(10)   << std::left  << " Offset"
                << std::setw(maxw) << std::left  << "Filename"
                << std::setw(10)   << std::right << "Bytes"
                << std::endl;

      std::cout << std::setw(maxw + 20) << std::setfill('-') << "-" << std::endl;

      for(const std::string& filename : archive->listFiles())
        std::cout << std::setw(8)    << std::setfill('0') << std::hex << std::right << archive->fileOffset(filename) << "  "
                  << std::setw(maxw) << std::setfill('.') << std::left << filename
                  << std::setw(10)   << std::setfill('.') << std::dec << std::right << archive->fileSize(filename)
                  << std::endl;
      break;
    }
    case 'x':
    {
      if(argfilename.empty())
        list = archive->listFiles();
      else
      {
        if(archive->fileExists(argfilename))
          list.push_back(argfilename);
        else
          std::cout << "archive does not contain a file named: " << argfilename << std::endl;
      }

      for(const std::string& filename : list)
      {
        std::cout << "Extracting: " << filename << std::endl;
        if(!mkdir_p(filename.c_str(), S_IRWXU))
        {
          file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
          std::shared_ptr<anyfile> subfile = archive->getFile<anyfile>(filename);
          file.write<uint8_t>(subfile->data, subfile->dataSize());
          file.close();
        }
      }
      break;
    }
    case 'r':
      if(!archive->removeFile(argfilename))
        return -1;
      break;
    case 'a':
    {
      file.open(argfilename);
      if(!file.is_open())
      {
        std::cout << "could not open file: " << argfilename << std::endl;
        return -1;
      }
      data.resize(file.size());
      file >> data;
      file.close();
      if(!archive->appendFile(argfilename, data.data(), data.size()))
        return -1;
      break;
    }

    default:
      std::cout << "Usage:" << std::endl
                << " list:    " << argv[0] << " -f <sak_file> -l"              << std::endl
                << " extract: " << argv[0] << " -f <sak_file> -x [<filename>]" << std::endl
                << " remove:  " << argv[0] << " -f <sak_file> -r <filename>"   << std::endl
                << " append:  " << argv[0] << " -f <sak_file> -a <filename>"   << std::endl;
      return -1;
  }

  return 0;
}
