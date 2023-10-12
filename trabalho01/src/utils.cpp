#include "utils.hpp"

namespace utils
{
  void createDirectory(const std::string &path)
  {
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
      if (mkdir(path.c_str(), 0777) == 0)
      {
        return;
      }
      else if (info.st_mode & S_IFDIR)
      {
        return;
      }
    }
  }

  std::string currentUnixTime(void)
  {
    return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count());
  }
} // namespace utils