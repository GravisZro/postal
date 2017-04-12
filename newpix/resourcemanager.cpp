#include "resourcemanager.h"

#include <cctype>
#include <algorithm>

SAKArchive g_GameSAK("res/game/game.sak");

template<typename T>
T normalize(T c)
{
  if(c == '\\')
    return '/';
  return std::tolower(c);
}

void Resources::normalize_path(std::string& path) noexcept
{
  std::transform(path.begin(), path.end(), path.begin(), normalize<std::string::value_type>);
}
