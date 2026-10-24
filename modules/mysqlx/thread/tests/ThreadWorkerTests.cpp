#include <stdio.h>
#include <unistd.h>

#include "PosixThreads.h"
#include "ThreadWorker.h"

#define CHECK(expr) \
	do { \
		if (!(expr)) { \
			fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #expr); \
			return 1; \
		} \
	} while (0)

class FailingThreader : public PosixThreader
{
public:
	IThreadHandle *MakeThread(IThread *thread, const ThreadParams *params) override
	{
		if (m_failNext)
		{
			m_failNext = false;
			return NULL;
		}

		return PosixThreader::MakeThread(thread, params);
	}

private:
	bool m_failNext = true;
};

int main()
{
	alarm(10);

	PosixThreader threader;
	PosixThreader::PosixEventSignal signal;
	signal.Signal();
	signal.Wait();
	signal.Signal();
	signal.Wait();

	FailingThreader failingThreader;
	ThreadWorker retryWorker(&failingThreader, 0);
	CHECK(!retryWorker.Start());
	CHECK(retryWorker.Start());
	CHECK(retryWorker.Stop(true));

	ThreadWorker pausedWorker(&threader, 0);
	for (int i = 0; i < 10; i++)
	{
		CHECK(pausedWorker.Start());
		CHECK(pausedWorker.Pause());
		usleep(50000);
		CHECK(pausedWorker.Stop(true));
	}

	return 0;
}
