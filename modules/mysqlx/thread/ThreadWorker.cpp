// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

#include "ThreadWorker.h"

ThreadWorker::ThreadWorker() : 
	m_Threader(NULL),
	m_QueueLock(NULL),
	m_StateLock(NULL),
	m_PauseSignal(NULL),
	m_AddSignal(NULL),
	me(NULL),
	m_think_time(DEFAULT_THINK_TIME_MS)
{
	m_state = Worker_Invalid;
}

ThreadWorker::ThreadWorker(IThreader *pThreader, unsigned int thinktime) : 
	m_Threader(pThreader),
	m_QueueLock(NULL),
	m_StateLock(NULL),
	m_PauseSignal(NULL),
	m_AddSignal(NULL),
	me(NULL),
	m_think_time(thinktime)
{
	if (m_Threader)
	{
		m_state = Worker_Stopped;
	} else {
		m_state = Worker_Invalid;
	}
}

ThreadWorker::~ThreadWorker()
{
	if (m_state != Worker_Stopped && m_state != Worker_Invalid)
		Stop(true);

	if (m_ThreadQueue.length())
		Flush(true);
}

void ThreadWorker::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
	//we don't particularly care
	return;
}

void ThreadWorker::RunThread(IThreadHandle *pHandle)
{
	WorkerState this_state = Worker_Running;
	size_t num;
    
	while (true)
	{
		/**
		 * Check number of items in the queue
		 */
		m_StateLock->Lock();
		this_state = m_state;
		m_StateLock->Unlock();
		if (this_state != Worker_Stopped)
		{
			m_QueueLock->Lock();
			num = m_ThreadQueue.length();
			if (!num)
			{
				/** 
				 * if none, wait for an item
				 */
				m_Waiting = true;
				m_QueueLock->Unlock();
				/* first check if we should end again */
				if (this_state == Worker_Stopped)
				{
					break;
				}
				m_AddSignal->Wait();
				m_Waiting = false;
			} else {
				m_QueueLock->Unlock();
			}
		}
		m_StateLock->Lock();
		this_state = m_state;
		m_StateLock->Unlock();
		if (this_state != Worker_Running)
		{
			if (this_state == Worker_Paused || this_state == Worker_Stopped)
			{
				//wait until the lock is cleared.
				if (this_state == Worker_Paused)
				{
					m_PauseSignal->Wait();
					m_StateLock->Lock();
					this_state = m_state;
					m_StateLock->Unlock();
				}
				if (this_state == Worker_Stopped)
				{
					//if we're supposed to flush cleanrly, 
					// run all of the remaining frames first.
					if (!m_FlushType)
					{
						while (m_ThreadQueue.length())
							RunFrame();
					}
					break;
				}
			}
		}
		/**
		 * Run the frame.
		 */
		RunFrame();

		/**
		 * wait in between threads if specified
		 */
		if (m_think_time)
			m_Threader->ThreadSleep(m_think_time);
	}
}

SWThreadHandle *ThreadWorker::PopThreadFromQueue()
{
	if (m_state <= Worker_Stopped && !m_QueueLock)
		return NULL;

	SWThreadHandle *swt;
	m_QueueLock->Lock();
	swt = BaseWorker::PopThreadFromQueue();
	m_QueueLock->Unlock();

	return swt;
}

void ThreadWorker::AddThreadToQueue(SWThreadHandle *pHandle)
{
	if (m_state <= Worker_Stopped)
		return;

	m_QueueLock->Lock();
	BaseWorker::AddThreadToQueue(pHandle);
	if (m_Waiting)
	{
		m_AddSignal->Signal();
	}
	m_QueueLock->Unlock();
}

WorkerState ThreadWorker::GetStatus(unsigned int *threads)
{
	WorkerState state;

	m_StateLock->Lock();
	state = BaseWorker::GetStatus(threads);
	m_StateLock->Unlock();

	return state;
}

bool ThreadWorker::Start()
{
	if (m_state == Worker_Invalid)
	{
		if (m_Threader == NULL)
			return false;
	} else if (m_state != Worker_Stopped) {
		return false;
	}

	m_Waiting = false;
	m_QueueLock = m_Threader->MakeMutex();
	m_StateLock = m_Threader->MakeMutex();
	m_PauseSignal = m_Threader->MakeEventSignal();
	m_AddSignal = m_Threader->MakeEventSignal();
	if (!m_QueueLock || !m_StateLock || !m_PauseSignal || !m_AddSignal)
	{
		DestroySyncObjects();
		return false;
	}

	m_state = Worker_Running;
	ThreadParams pt;
	pt.flags = Thread_Default;
	pt.prio = ThreadPrio_Normal;
	me = m_Threader->MakeThread(this, &pt);
	if (!me)
	{
		m_state = Worker_Stopped;
		DestroySyncObjects();
		return false;
	}

	return true;
}

bool ThreadWorker::Stop(bool flush_cancel)
{
	if (m_state == Worker_Invalid || m_state == Worker_Stopped)
		return false;

	//set new state
	m_StateLock->Lock();
	m_state = Worker_Stopped;
	m_FlushType = flush_cancel;
	m_StateLock->Unlock();

	m_PauseSignal->Signal();
	m_AddSignal->Signal();

	me->WaitForThread();
	//destroy it
	me->DestroyThis();
	//flush all remaining events
	Flush(true);

	DestroySyncObjects();
	me = NULL;

	return true;
}

void ThreadWorker::DestroySyncObjects()
{
	if (m_QueueLock)
		m_QueueLock->DestroyThis();
	if (m_StateLock)
		m_StateLock->DestroyThis();
	if (m_PauseSignal)
		m_PauseSignal->DestroyThis();
	if (m_AddSignal)
		m_AddSignal->DestroyThis();

	m_QueueLock = NULL;
	m_StateLock = NULL;
	m_PauseSignal = NULL;
	m_AddSignal = NULL;
}

bool ThreadWorker::Pause()
{
	if (m_state != Worker_Running)
		return false;

	m_StateLock->Lock();
	m_state = Worker_Paused;
	m_StateLock->Unlock();

	return true;
}


bool ThreadWorker::Unpause()
{
	if (m_state != Worker_Paused)
		return false;

	m_StateLock->Lock();
	m_state = Worker_Running;
	m_StateLock->Unlock();
	m_PauseSignal->Signal();
	if (m_Waiting)
	{
		m_AddSignal->Signal();
	}

	return true;
}
