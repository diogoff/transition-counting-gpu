// ---------------------------------------------------------------------------------------
// GPU version

int n_threads_per_block;

__global__ void kernel_transitions(Event* d_all_events, int n_transitions, int* d_transitions, int n_users)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n_transitions)
	{
		if (d_all_events[i].caseid == d_all_events[i+1].caseid)
		{
			int user0 = d_all_events[i].user;
			int user1 = d_all_events[i+1].user;
			d_transitions[i] = n_users*user0 + user1 + 1;
		}
	}
}

__global__ void kernel_matrix(int* d_results, int n_matrix, int* d_matrix)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n_matrix)
	{
		if (d_results[i] > 0)
		{
			d_matrix[d_results[i]-1] = d_results[n_matrix+i];
		}
	}
}

int* handover3()
{
	printf("Running GPU version (%d run%s, %d thread%s per block)\n", n_runs, (n_runs > 1) ? "s" : "", n_threads_per_block, (n_threads_per_block > 1) ? "s" : "");

	int* matrix = (int*)malloc(n_matrix*sizeof(int));
	
	try
	{
		Event* d_all_events;
		CUDA_CHECK(cudaMalloc(&d_all_events, n_events*sizeof(Event)));
		CUDA_CHECK(cudaMemcpy(d_all_events, all_events, n_events*sizeof(Event), cudaMemcpyHostToDevice));

		int n_transitions = n_events-1;
		thrust::device_vector<int> dv_transitions(n_transitions);
		int* d_transitions = thrust::raw_pointer_cast(&dv_transitions[0]);

		int threads1 = n_threads_per_block;
		int blocks1 = (int)ceil((double)n_transitions/(double)threads1);
		while ((blocks1 > 65535) && (threads1 + n_threads_per_block <= 1024))
		{
			threads1 += n_threads_per_block;
			blocks1 = (int)ceil((double)n_transitions/(double)threads1);
		}

		thrust::constant_iterator<int> const_iter(1);

		int n_results = 2*n_matrix;
		thrust::device_vector<int> dv_results(n_results);
		int* d_results = thrust::raw_pointer_cast(&dv_results[0]);

		int threads2 = n_threads_per_block;
		int blocks2 = (int)ceil((double)n_matrix/(double)threads2);
		while ((blocks2 > 65535) && (threads2 + n_threads_per_block <= 1024))
		{
			threads2 += n_threads_per_block;
			blocks2 = (int)ceil((double)n_matrix/(double)threads2);
		}

		int* d_matrix;
		CUDA_CHECK(cudaMalloc(&d_matrix, n_matrix*sizeof(int)));

		double total = 0.0;
		for(int r=0; r<n_runs; r++)
		{
			// -------------------------------------------------------------
			double t0 = seconds();

			CUDA_CHECK(cudaMemset(d_transitions, 0, n_transitions*sizeof(int)));
			CUDA_CHECK(cudaMemset(d_results, 0, n_results*sizeof(int)));
			CUDA_CHECK(cudaMemset(d_matrix, 0, n_matrix*sizeof(int)));

			kernel_transitions<<<blocks1, threads1>>>(d_all_events, n_transitions, d_transitions, n_users);
			
			thrust::sort(dv_transitions.begin(), dv_transitions.end());
			
			thrust::reduce_by_key(dv_transitions.begin(), dv_transitions.end(), const_iter, dv_results.begin(), dv_results.begin() + n_matrix);
			
			kernel_matrix<<<blocks2, threads2>>>(d_results, n_matrix, d_matrix);

			double t1 = seconds();
			// -------------------------------------------------------------
			total += (t1-t0);
		}
		printf("Time: %f\n", total/n_runs);

		CUDA_CHECK(cudaMemcpy(matrix, d_matrix, n_matrix*sizeof(int), cudaMemcpyDeviceToHost));

		CUDA_CHECK(cudaFree(d_all_events));
		CUDA_CHECK(cudaFree(d_matrix));
	}
	catch(thrust::system_error& error)
	{
		printf("Thrust Error: %s\n", error.what());
		exit(1);
	}
	catch(std::bad_alloc& error)
	{
		printf("Thrust Error: out of memory\n");
		exit(1);
	}

	return matrix;
}
