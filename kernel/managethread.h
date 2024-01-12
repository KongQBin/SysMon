#pragma once
#include "general.h"
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "kmstructs.h"

pthread_t createManageThread(InitInfo *info);
int sendManageInfo(ManageInfo *info);
