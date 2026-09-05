#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "inventory.h"
#include "synchronization.h"

int run_organization_pipeline(Inventory *inv, SyncPrimitives *sync,
                              int final_strategy);
long run_race_demo(int use_mutex);

#endif
