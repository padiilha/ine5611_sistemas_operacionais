#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <semaphore.h>
#include <sys/stat.h>

#include <chrono>
#include <iostream>
#include <vector>

namespace utils
{
  std::string currentUnixTime(void);

  const int INPUT_INTERVAL = 20000; // uSECS
  const int MAX_MISSILE_CAPACITY = 6;
  const int MISSILE_BATTERY_MOV_SPEED_FACT = 100000; // uSECS
  const int MISSILE_GENERATOR_INTERVAL = 1;          // SECS
  const int MISSILE_MOV_SPEED_FACT = 62500;          // uSECS

  namespace Types
  {
    typedef std::vector<std::vector<Element *>> Board;

    enum EntityEnum
    {
      HELICOPTER = 1,
      MISSILE_BATTERY = 2,
      MISSILE = 3
    };

    struct CriticalResource
    {
      int readCount = 0;
      int writeCount = 0;
      sem_t readerSem, writerSem, readTrySem, resourceSem;
    };

    struct Element : CriticalResource
    {
      int displayValue = 0;
      int entityId = 0;
    };

    struct Helicopter : CriticalResource
    {
      bool alive = true;
      std::pair<int, int> pos = {0, 0};
    };

    struct MissileBattery : CriticalResource
    {
      int id = 0;
      int missiles = 6;
      int pos = 0;
    };

    struct AvailableMissiles : CriticalResource
    {
      int n = 6;
    };

    struct GameState
    {
      Board boardState;
      Helicopter *helicopter;
      std::vector<MissileBattery *> missileBattery;
      AvailableMissiles *missiles;
      int difficulty;
      int over;
    };

    struct MissileBatteryProps
    {
      int id;
      GameState *state;
    };

    struct MissileProps
    {
      int missileBatteryPos;
      GameState *state;
    };
  } // namespace Types
} // namespace utils

#endif