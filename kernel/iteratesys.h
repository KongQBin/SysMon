#pragma once
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "general.h"
#include <unistd.h>
#include <stdlib.h>
int iterateSysThreads(pid_t **pids);
