#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "pfv3d"

int main(int argc, char const *argv[])
{
	int n_points, i, j;
	double **points;
	pfv3d visualiser(0, 0, 0);
	FILE *f = fopen("Input/surf06.dat", "r");

	fscanf(f, "%d", &n_points);
	points = (double **) malloc(n_points * sizeof(double *));

	for(i = 0; i < n_points; ++i) {
		points[i] = (double *) malloc(3 * sizeof(double));
		fscanf(f, "%lf %lf %lf", &points[i][0], &points[i][1], &points[i][2]);
	}
	
	fclose(f);

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; i+=154) {
		for(j = 0; j < 154; ++j)
			visualiser.add_point(points[i+j]);
		visualiser.compute(4);
	}
	visualiser.display();

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; i+=154) {
		for(j = 0; j < 154; ++j)
			visualiser.rem_point(points[i+j]);
		visualiser.compute(4);
	}
	visualiser.display();

	for(i = 0; i < n_points; ++i)
		free(points[i]);
	free(points);
	
	return 0; 
}
