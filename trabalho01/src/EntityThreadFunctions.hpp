#ifndef __ENTITY_HPP__
#define __ENTITY_HPP__

#include <unistd.h>

#include <functional>

#include "utils.hpp"

namespace EntityThreadFunctions
{
  namespace Sync
  {
    void writerEnterCriticalSection(utils::Types::CriticalResource *resource);
    void writerExitCriticalSection(utils::Types::CriticalResource *resource);
    void autoWriteCriticalSection(utils::Types::CriticalResource *resource, std::function<void()> op);

    utils::Types::CriticalResource readerEnterCriticalSection(utils::Types::CriticalResource *resource);
    void readerExitCriticalSection(utils::Types::CriticalResource *resource);
    void autoReadCriticalSection(utils::Types::CriticalResource *resource, std::function<void(utils::Types::CriticalResource)> op);
  } // namespace Sync

  void *helicopter(void *arg);
  void *missileBattery(void *arg);
  void *missile(void *arg);
  void *warehouse(void *arg);
} // namespace EntityThreadFunctions

#endif