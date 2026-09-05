#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "bookshelf_core.h"

int run_organization_pipeline(BsInventory *inventory, int final_strategy);
long run_race_demo(int use_mutex);

#endif
