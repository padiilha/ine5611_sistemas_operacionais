#include "EntityThreadFunctions.hpp"

namespace EntityThreadFunctions
{
  namespace Sync
  {
    void writerEnterCriticalSection(utils::Types::CriticalResource *resource)
    {
      sem_wait(&resource->writerSem);
      resource->writeCount++;
      if (resource->writeCount == 1)
        sem_wait(&resource->readTrySem);
      sem_post(&resource->writerSem);
      sem_wait(&resource->resourceSem);
    }

    void writerExitCriticalSection(utils::Types::CriticalResource *resource)
    {
      sem_post(&resource->resourceSem);
      sem_wait(&resource->writerSem);
      resource->writeCount--;
      if (resource->writeCount == 0)
        sem_post(&resource->readTrySem);
      sem_post(&resource->writerSem);
    }

    void autoWriteCriticalSection(utils::Types::CriticalResource *resource, std::function<void()> op)
    {
      writerEnterCriticalSection(resource);
      op();
      writerExitCriticalSection(resource);
    }

    utils::Types::CriticalResource readerEnterCriticalSection(utils::Types::CriticalResource *resource)
    {
      sem_wait(&resource->readTrySem);
      sem_wait(&resource->readerSem);
      resource->readCount++;
      if (resource->readCount == 1)
        sem_wait(&resource->resourceSem);
      sem_post(&resource->readerSem);
      sem_post(&resource->readTrySem);
      return *resource;
    }

    void readerExitCriticalSection(utils::Types::CriticalResource *resource)
    {
      sem_wait(&resource->readerSem);
      resource->readCount--;
      if (resource->readCount == 0)
        sem_post(&resource->resourceSem);
      sem_post(&resource->readerSem);
    }

    void autoReadCriticalSection(utils::Types::CriticalResource *resource, std::function<void(utils::Types::CriticalResource resource)> op)
    {
      op(readerEnterCriticalSection(resource));
      readerExitCriticalSection(resource);
    }
  } // namespace Sync

  namespace
  {
    void handleMissileLaunch(utils::Types::GameState *state, utils::Types::MissileProps *props)
    {
      bool empty;
      Sync::autoWriteCriticalSection(
          state->missiles,
          [&props, &state, &empty]()
          {
            if (state->missiles->n > 0)
            {
              state->missiles->n--;
              empty = false;
            }
            else
            {
              empty = true;
            }
          });

      if (!empty)
      {
        pthread_t thread;
        pthread_create(&thread, NULL, missile, props);
      }
    }
  } // namespace

  void *helicopter(void *arg)
  {
    utils::Types::GameState *state = (utils::Types::GameState *)arg;
    utils::Types::Board &board = state->boardState;
    std::pair<int, int> &pos = state->helicopter->pos;
    int posX = pos.first;
    int posY = pos.second;
    int width = state->boardState[0].size();
    int height = state->boardState.size();
    std::vector<utils::Types::Element *, utils::Types::Element *> &flightArea = board[width][height];
    bool alive = true;

    while (true)
    {
      auto &board = state->boardState;

      int ch;
      int newPosX;
      int newPosY;

      if ((ch = getch()) != ERR)
      {
        if (ch >= 'A' && ch <= 'Z')
          ch += 32;

        switch (ch)
        {
        case 'w':
          newPosY = posY - 1;
          break;
        case 'a':
          newPosX = posX - 1;
          break;
        case 's':
          newPosY = posY + 1;
          break;
        case 'd':
          newPosX = posX + 1;
          break;
        }
      }

      if ((newPosX != posX || newPosY != posY) && (newPosX > 0 && newPosY > 0) && (newPosX < width && newPosY < height))
      {
        flightArea[posX][posY]->displayValue = 0;
        posX = newPosX;
        posY = newPosY;
      }

      flightArea[posX][posY]->displayValue = utils::Types::EntityEnum::HELICOPTER;
      usleep(utils::INPUT_INTERVAL);
    }
  }

  void *missileBattery(void *arg)
  {
    int id;
    utils::Types::GameState *state;
    utils::Types::MissileBattery *self;
    utils::Types::MissileBatteryProps *props = (utils::Types::MissileBatteryProps *)arg;
    int width = state->boardState[0].size();
    int posX = self->pos;

    id = props->id;
    state = props->state;
    self = state->missileBattery[id];
    posX = width / 2;

    delete props;

    while (true)
    {
      auto &board = state->boardState;
      int newPosX;
      utils::Types::MissileProps *props = new utils::Types::MissileProps;

      props->state = state;
      props->missileBatteryPos = posX;
      handleMissileLaunch(state, props);

      if (id % 2 == 0)
      {
        newPosX = posX + 1 % width;
      }
      else
      {
        newPosX = posX - 1 % width;
      }

      Sync::autoWriteCriticalSection( // oqq faz
          state->boardState[newPosX],
          [&state, &self, posX, newPosX, id]()
          {
            if (state->boardState[newPosX]->displayValue == 0)
            {
              state->boardState[newPosX]->displayValue = utils::Types::EntityEnum::MISSILE_BATTERY;
              state->boardState[newPosX]->entityId = id;
              state->boardState[posX]->entityId = 0;
              self->pos = newPosX;
              return;
            }
            state->boardState[newPosX]->displayValue = 0;
            state->boardState[newPosX]->entityId = 0;
            state->boardState[posX]->displayValue = utils::Types::EntityEnum::MISSILE_BATTERY;
          });

      usleep(utils::MISSILE_BATTERY_MOV_SPEED_FACT / state->difficulty);
    }
  }

  void *missile(void *arg)
  {
    utils::Types::MissileProps *props = (utils::Types::MissileProps *)arg;
    utils::Types::GameState *state = props->state;
    std::pair<int, int> initialPos = {state->boardState.size() - 2, props->missileBatteryPos};
    int prev;

    delete props;

    for (int i = initialPos.first; i >= 0; i--)
    {
      bool hit = false;

      if (prev)
      {
        Sync::autoWriteCriticalSection(
            state->boardState[prev][initialPos.second],
            [&state, prev, initialPos]()
            {
              state->boardState[prev][initialPos.second]->displayValue = 0;
            });
      }

      Sync::autoWriteCriticalSection(
          state->boardState[i][initialPos.second],
          [&state, &hit, i, initialPos]()
          {
            if (state->boardState[i][initialPos.second]->displayValue == 2)
            {
              auto helicopter = state->helicopter[state->boardState[i][initialPos.second]->entityId];
              Sync::autoWriteCriticalSection(
                  helicopter,
                  [&helicopter]()
                  {
                    helicopter->alive = false;
                  });
              hit = true;
            }
          });

      if (hit)
        break;

      prev = i;
      usleep(utils::MISSILE_MOV_SPEED_FACT);
    }

    state->boardState[prev][initialPos.second]->displayValue = 0;
  }

  void *warehouse(void *arg)
  {
    utils::Types::GameState *state = (utils::Types::GameState *)arg;

    while (true)
    {
      Sync::autoWriteCriticalSection(
          state->missiles,
          [&state]()
          {
            if (state->missiles->n < utils::MAX_MISSILE_CAPACITY)
            {
              state->missiles->n++;
            }
          });

      sleep(utils::MISSILE_GENERATOR_INTERVAL);
    }
  }
} // namespace EntityThreadFunctions