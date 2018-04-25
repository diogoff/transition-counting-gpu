// ---------------------------------------------------------------------------------------
// CPU single-threaded version

int n_runs;
int n_matrix;

int* handover1()
{
	printf("Running CPU single-threaded version (%d run%s)\n", n_runs, (n_runs > 1) ? "s" : "");

	int* matrix = (int*)malloc(n_matrix*sizeof(int));

	double total = 0.0;
	for(int r=0; r<n_runs; r++)
	{
		// -------------------------------------------------------------
		double t0 = seconds();

		memset(matrix, 0, n_matrix*sizeof(int));
		
		for (int i=0; i<n_caseids; i++)
		{
			for (int j=pos_caseid[i].begin; j<pos_caseid[i].end-1; j++)
			{
				int user0 = all_events[j].user;
				int user1 = all_events[j+1].user;
				matrix[n_users*user0+user1]++;
			}
		}

		double t1 = seconds();
		// -------------------------------------------------------------
		total += (t1-t0);
	}
	printf("Time: %f\n", total/n_runs);

	return matrix;
}
