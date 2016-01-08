#include "MaxSceneTimeManager.h"
#include "stdafx.h"

static TimeValue g_currTick = -1;

void SetMaxSceneTime(TimeValue ticks)
{
  if (g_currTick != ticks) {
    g_currTick = ticks;
    GET_MAX_INTERFACE()->SetTime(ticks);
  }
}