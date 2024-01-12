#pragma once
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "kmstructs.h"
#include "general.h"
#include "pidtree.h"
#include "cbdefine.h"
#include "callbacks.h"
#include "managethread.h"



void MonProcMain(pid_t tid);
