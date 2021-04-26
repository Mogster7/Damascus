#pragma once



struct Job
{
	uint32_t ID;
};

class JobSystem
{
	using JobFunc = std::function<void(void)>;

public:
	inline static uint32_t ThreadCount = 1;

	struct JobData
	{
		Job job;
		JobFunc function;

		enum State
		{
			INVALID = -1,
			WAITING = 0,
			READY = 1,
			ACTIVE = 2,
			COMPLETE = 3
		} state;

		// Number of jobs dependent ON THIS one
		std::vector<Job> dependents;

		// Number of jobs this job is DEPENDENT ON to run
		uint32_t dependencyCount = 0;

		// 0 for awaiting execution
		// 1 for waiting or completed in worker
		// 2 for both of the above, needs erasure
		uint32_t accessCount = 0;
	};

	static void Initialize();

	// Add job to system
	static Job Push(const JobFunc jobFunction, 
					const Job* dependencies = nullptr,
					const uint32_t dependencyCount = 0);

	static Job Push(const JobFunc jobFunction,
					const Job dependency);


	static void AddDependency(const Job job, 
							  const Job dependency);

	// Execute the set of jobs currently pushed
	static void Execute();
	
	// Wait on completion
	static void Wait(const Job* jobs, const uint32_t jobCount);
	static void Wait(Job job);
	static void Wait(const std::vector<Job>& jobs);
	static void WaitAll();

	// Combine multiple jobs into one
	static Job Combine(const Job* jobs, const uint32_t jobCount);
	static Job Combine(const std::vector<Job>& jobs);


private:
	// Worker thread active
	inline static std::atomic_bool active = false;

	// Our dispatcher thread
	inline static std::unique_ptr<std::thread> dispatcher;

	// Unique job ID to assign, increments on assignment
	inline static uint32_t currentID = 0;

	// Mutex for locking the data below
	inline static std::mutex dataMutex;

	// Job-related data
	inline static std::unordered_map<uint32_t, JobData> pushedJobs;
	inline static std::unordered_map<uint32_t, JobData> activeJobs;
	inline static std::unordered_map<uint32_t, std::future<void>> futures;

	// Check on status of our job's future
	static bool IsFutureReady(const std::future<void>& future);

	// Determines deletion case
	static bool AccessJobData(JobData& data);
	static bool AccessJobData(std::unordered_map<const uint32_t, JobData>::iterator& it);

	// Worker thread dispatcher function
	static void Dispatch();
};
