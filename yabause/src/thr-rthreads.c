/*  src/thr-rthreads.c: Thread functions for Libretro
    Copyright 2019 barbudreadmon

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef _WIN32
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "core.h"
#include "threads.h"
#include "rthreads/rthreads.h"
#include <stdlib.h>

struct thd_s {
	int running;
	sthread_t *thd;
	slock_t *mutex;
	scond_t *cond;
};

static struct thd_s thread_handle[YAB_NUM_THREADS];
static sthread_tls_t hnd_key;
static int hnd_key_once = 0;

//////////////////////////////////////////////////////////////////////////////

int YabThreadStart(unsigned int id, void (*func)(void *), void *arg)
{
	if (hnd_key_once == 0)
	{
		if(sthread_tls_create(&hnd_key));
			hnd_key_once = 1;
	}

	if (thread_handle[id].running)
	{
		return -1;
	}
	if ((thread_handle[id].mutex = slock_new()) == NULL)
	{
		return -1;
	}
	if ((thread_handle[id].cond = scond_new()) == NULL)
	{
		slock_free(thread_handle[id].mutex);
		return -1;
	}

	if ((thread_handle[id].thd = sthread_create((void *)func, arg)))
	{
		scond_free(thread_handle[id].cond);
		slock_free(thread_handle[id].mutex);
		return -1;
	}

	thread_handle[id].running = 1;

	return 0;
}

void YabThreadWait(unsigned int id)
{
	if (!thread_handle[id].thd)
		return;  // Thread wasn't running in the first place

	sthread_join(thread_handle[id].thd);
	scond_free(thread_handle[id].cond);
	slock_free(thread_handle[id].mutex);
	thread_handle[id].thd = NULL;
	thread_handle[id].running = 0;
}

void YabThreadYield(void)
{
#ifdef _WIN32
	SleepEx(0, 0);
#else
	sched_yield();
#endif
}

void YabThreadSleep(void)
{
	struct thd_s *thd = (struct thd_s *)sthread_tls_get(&hnd_key);
	scond_wait(thd->cond, thd->mutex);
}

void YabThreadWake(unsigned int id)
{
	if (!thread_handle[id].thd)
		return;  // Thread wasn't running in the first place

	scond_signal(thread_handle[id].cond);
}

//////////////////////////////////////////////////////////////////////////////
