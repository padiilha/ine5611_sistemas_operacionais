#include "EntityThreadFunctions.hpp"

namespace EntityThreadFunctions
{
  namespace Sync
  {
    void writerEnterCriticalSection(utils::Types::CriticalResource *resource)
    {
      sem_wait(&resource->writerSem); // bloqueia o semafaro de escrita
      resource->writeCount++;         // aumenta o numero de escritores
      if (resource->writeCount == 1)
        sem_wait(&resource->readTrySem); // se o numero de escritores for 1, bloqueia tentativa de leitura
      sem_post(&resource->writerSem);    // libera o semafaro de escrita
      sem_wait(&resource->resourceSem);  // bloqueia o recurso
    }

    void writerExitCriticalSection(utils::Types::CriticalResource *resource)
    {
      sem_post(&resource->resourceSem); // libera o recurso
      sem_wait(&resource->writerSem);   // bloqueia o escritor
      resource->writeCount--;           // diminui o numero de escritores
      if (resource->writeCount == 0)
        sem_post(&resource->readTrySem); // se o numero de escritores for 0, libera tentativa de leitura
      sem_post(&resource->writerSem);    // libera o escritor
    }

    void autoWriteCriticalSection(utils::Types::CriticalResource *resource, std::function<void()> op)
    {
      writerEnterCriticalSection(resource);
      op();
      writerExitCriticalSection(resource);
    }

    utils::Types::CriticalResource readerEnterCriticalSection(utils::Types::CriticalResource *resource)
    {
      sem_wait(&resource->readTrySem); // bloqueia a tentativa de leitura
      sem_wait(&resource->readerSem);  // bloqueia a leitura
      resource->readCount++;           // aumenta o numero de leitores
      if (resource->readCount == 1)
        sem_wait(&resource->resourceSem); // se o numero de leitores for 1, bloqueia o recurso
      sem_post(&resource->readerSem);     // libera a leitura
      sem_post(&resource->readTrySem);    // libera a tentativa de leitura
      return *resource;
    }

    void readerExitCriticalSection(utils::Types::CriticalResource *resource)
    {
      sem_wait(&resource->readerSem); // bloqueia a leitura
      resource->readCount--;          // diminui o numero de leitores
      if (resource->readCount == 0)
        sem_post(&resource->resourceSem); // se o numero de leitores for 0, libera o recurso
      sem_post(&resource->readerSem);     // libera a leitura
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
    utils::Types::GameState *state = (utils::Types::GameState *)arg; // atribui a um ponteiro do tipo GameState o que?
    utils::Types::Board &board = state->boardState;                  // atribui ao local onde aponta o ponteiro board o valor de boardState
    bool alive = true;

    // completar
  }

  void *missileBattery(void *arg)
  {
    int id;
    utils::Types::GameState *state;
    utils::Types::MissileBattery *self;

    utils::Types::MissileBatteryProps *props = (utils::Types::MissileBatteryProps *)arg;

    id = props->id;
    state = props->state;
    self = state->missileBattery[id];

    delete props;

    while (true)
    {
      auto &board = state->boardState;
      auto pos = self->pos;
      auto x = pos.first;
      auto y = pos.second;

      // completar
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