
#include "Job.h"

void JobSystem::Initialize()
{
	dispatcher = new std::thread(Dispatch);
	// -1 for our dispatcher
	ThreadCount = std::thread::hardware_concurrency() - 1;

	active = true;
}

Job JobSystem::Push(const JobFunc jobFunction, 
					const Job* dependencies /*= nullptr*/, 
					const uint32_t dependencyCount /*= 0*/)
{
	pushedJobs.insert({ currentID, {} });
	JobData& data = pushedJobs[currentID];
	data.job.ID = currentID;
	data.function = jobFunction;
	// If we have no dependencies, it can run immediately
	data.state = (dependencyCount == 0) ? JobData::State::READY : JobData::State::WAITING;
	data.dependencyCount = dependencyCount;

	for (int i = 0; i < dependencyCount; ++i)
	{
		Job dependency = dependencies[i];

		// Test dependency chain
		ASSERT(pushedJobs.find(dependency.ID) != pushedJobs.end(),
			   "Job dependency does not exist in previously pushed jobs");

		pushedJobs[dependency.ID].dependents.emplace_back(data.job);
	}

	++currentID;

	return data.job;
}

Job JobSystem::Push(const JobFunc jobFunction, const Job dependency)
{
	return Push(jobFunction, &dependency, 1);
}

void JobSystem::AddDependency(const Job job, const Job dependency)
{
	// Add to dependencies' dependent list
	pushedJobs[dependency.ID].dependents.emplace_back(job);

	// Increase the number of accesses necessary to run
	++pushedJobs[job.ID].dependencyCount;
	pushedJobs[job.ID].state = JobData::State::WAITING;
}

void JobSystem::Execute()
{
	// Lock and move 
	std::lock_guard<std::mutex> lock(dataMutex);

	// Move our data to active list
	std::move(pushedJobs.begin(), pushedJobs.end(), std::inserter(activeJobs, activeJobs.end()));

	pushedJobs.clear();
}

void JobSystem::Wait(const Job* jobs, const uint32_t jobCount)
{
	// Lock and wait on the addition and then completion
	dataMutex.lock();
	for (int i = 0; i < jobCount;)
	{
		uint32_t ID = jobs[i].ID;

		// Assert that we've actually executed the job
		ASSERT(pushedJobs.find(ID) == pushedJobs.end(), "Job has not been executed, invalid wait command");
		// Not in pushed or active, completely invalid execution
		ASSERT(activeJobs.find(ID) != activeJobs.end(), "Job does not exist (anymore?), invalid wait command");

		JobData::State state = activeJobs[ID].state;
		ASSERT(state != JobData::State::INVALID, "Invalid job state");

		// Not executed yet, yield and try again after letting worker execute
		if (state == JobData::State::WAITING ||
			state == JobData::State::READY)
		{
			dataMutex.unlock();
			std::this_thread::yield();
			dataMutex.lock();
			continue;
		}

		ASSERT(futures.find(ID) != futures.end(), "Future does not exist for active job, should not happen");

		if (state == JobData::State::ACTIVE)
			futures[ID].wait();

		// Update access, deleting if second accessor
		AccessJobData(activeJobs[ID]);
		++i;
	}
	dataMutex.unlock();
}

void JobSystem::Wait(Job job)
{
	Wait(&job, 1);
}

void JobSystem::Wait(const std::vector<Job>& jobs)
{
	Wait(jobs.data(), jobs.size());
}

void JobSystem::WaitAll()
{
	// TODO: Not rely on accessing job data
	dataMutex.lock();
	while (!activeJobs.empty())
	{
		for (auto it = activeJobs.begin(); it != activeJobs.end();)
		{
			JobData& data = (*it).second;
			uint32_t ID = data.job.ID;

			// Yield for now, not done
			if (data.state == JobData::State::WAITING ||
				data.state == JobData::State::READY)
			{
				dataMutex.unlock();
				std::this_thread::yield();
				dataMutex.lock();
				break;
			}

			ASSERT(futures.find(ID) != futures.end(), "Future does not exist for active job, should not happen");

			if (data.state == JobData::State::ACTIVE)
				futures[ID].wait();

			// If we didn't remove, move iterator forward
			if(!AccessJobData(it))
				++it;
		}
	}
	dataMutex.unlock();
}

Job JobSystem::Combine(const Job* jobs, const uint32_t jobCount)
{
	// TODO: Make this not just an empty function, optimize
	return Push(
		[] {}, jobs, jobCount
	);
}

Job JobSystem::Combine(const std::vector<Job>& jobs)
{
	return Combine(jobs.data(), jobs.size());
}

bool JobSystem::IsFutureReady(const std::future<void>& future)
{
	return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

bool JobSystem::AccessJobData(std::unordered_map<uint32_t, JobData>::iterator& it)
{
	bool toRemove = it->second.accessCount == 2;
	// If we have updated it in 2 places
	// Remove
	if (toRemove)
	{
		futures.erase(it->second.job.ID);
		it = activeJobs.erase(it);
	}
	else
	{
		++it->second.accessCount;
	}
	return toRemove;
}

bool JobSystem::AccessJobData(JobData& data)
{
	bool toRemove = data.accessCount == 2;
	// If we have updated it in 2 places
	// Remove
	if (toRemove)
	{
		futures.erase(data.job.ID);
		activeJobs.erase(data.job.ID);
	}
	else
	{
		++data.accessCount;
	}

	return toRemove;
}

void JobSystem::Dispatch()
{
	while (active)
	{
		// Try and lock our data mutex
		// If its being read from another thread
		// we can yield this thread for now
		if (!dataMutex.try_lock())
		{
			std::this_thread::yield();
			continue;
		}

		for (auto it = activeJobs.begin(); it != activeJobs.end();)
		{
			std::pair<const uint32_t, JobData>& kv = (*it);
			JobData::State& state = kv.second.state;

			// If job is ready for work, not active
			if (state == JobData::State::READY)
			{
				JobData& data = kv.second;

				// Get job and assign to future
				JobFunc jobFunc = data.function;
				auto future = std::async(std::launch::async, jobFunc);


				futures.insert({ kv.first,
					 std::move(future)
							   });

				state = JobData::State::ACTIVE;

				ASSERT(data.accessCount == 0, "Invalid access state for job data");
				AccessJobData(kv.second);
			}
			// Job has been executed, check if done
			else if (state == JobData::State::ACTIVE)
			{
				JobData& data = kv.second;
				uint32_t ID = data.job.ID;

				// Check if our future is ready
				if (IsFutureReady(futures[ID]))
				{
					// Set dependencies to ready for execution
					for (Job dependent : data.dependents)
					{
						ASSERT(activeJobs[dependent.ID].state == JobData::State::WAITING,
							   "Job dependencies in invalid state");

						auto& depData = activeJobs[dependent.ID];

						// If we have reached our dependency requirement, execute
						--depData.dependencyCount;
						if (depData.dependencyCount == 0)
							depData.state = JobData::State::READY;
					}

					// Will no longer be accessed by this function
					state = JobData::State::COMPLETE;

					ASSERT(data.accessCount != 0, "Invalid access state for job data");
					AccessJobData(it);
					continue;
				}
			}
			++it;
		}
		dataMutex.unlock();
	}
}

void JobSystem::Destroy()
{
	active = false;
	dispatcher->join();
}
