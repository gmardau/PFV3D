#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "pfv3d"

void example1(), example2(), example3(), example4(), example5(), example6();

int main(int argc, char const *argv[])
{
	void (*execute[6])() = {&example1, &example2, &example3, &example4, &example5, &example6};

	for(int i = 1; i < argc; ++i) execute[atoi(argv[i])-1]();

	return 0; 
}

void example1 ()
{
	int n_points, i;
	double *points;
	pfv3d visualiser;
	FILE *f = fopen("Input/surf05.dat", "r");
	
	fscanf(f, "%d", &n_points);
	points = (double *) malloc(n_points * sizeof(double) * 3);

	for(i = 0; i < n_points*3; i+=3) 
		fscanf(f, "%lf %lf %lf", &points[i], &points[i+1], &points[i+2]);
	
	fclose(f);

	visualiser.add_points(n_points, points);
	visualiser.compute();
	// visualiser.display();
	visualiser.export_file("output");

	free(points);
}

void example2 ()
{
	int n_points, i, j;
	double **points;
	pfv3d visualiser(0, 0, 0, 2);
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
		visualiser.compute();
	}
	visualiser.display();

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; i+=154) {
		for(j = 0; j < 154; ++j)
			visualiser.rem_point(points[i+j]);
		visualiser.compute();
	}
	visualiser.display();

	for(i = 0; i < n_points; ++i)
		free(points[i]);
	free(points);
}

void example3 ()
{
	int n_points, i;
	double **points;
	pfv3d visualiser(1, 1, 1, 1);
	FILE *f = fopen("Input/surf05-500.dat", "r");

	fscanf(f, "%d", &n_points);
	points = (double **) malloc(n_points * sizeof(double *));

	for(i = 0; i < n_points; ++i) {
		points[i] = (double *) malloc(3 * sizeof(double));
		fscanf(f, "%lf %lf %lf", &points[i][0], &points[i][1], &points[i][2]);
	}
	
	fclose(f);

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; ++i) {
		visualiser.add_point(points[i]);
		visualiser.compute();
	}

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; ++i) {
		visualiser.rem_point(points[i]);
		visualiser.compute();
	}
	visualiser.display();

	for(i = 0; i < n_points; ++i)
		free(points[i]);
	free(points);
}

void example4 ()
{
	int n_points, i, j;
	double **points;
	pfv3d visualiser(0, 0, 0, 1);
	FILE *f = fopen("Input/surf06.dat", "r");

	fscanf(f, "%d", &n_points);
	points = (double **) malloc(n_points * sizeof(double *));

	for(i = 0; i < n_points; ++i) {
		points[i] = (double *) malloc(3 * sizeof(double));
		fscanf(f, "%lf %lf %lf", &points[i][0], &points[i][1], &points[i][2]);
	}
	
	fclose(f);

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < 2000; ++i) {
		for(j = 0; j < rand()%10; ++j) visualiser.add_point(points[rand()%n_points]);
		for(j = 0; j < rand()%10; ++j) visualiser.rem_point(points[rand()%n_points]);
		for(j = 0; j < rand()%10; ++j) visualiser.add_point(points[rand()%n_points]);
		for(j = 0; j < rand()%10; ++j) visualiser.rem_point(points[rand()%n_points]);
		visualiser.compute();
	}
	visualiser.display();

	for(i = 0; i < n_points; ++i)
		free(points[i]);
	free(points);
}

void example5 ()
{
	int n_points, i;
	double **points;
	pfv3d visualiser(0, 0, 0, 1);
	FILE *f = fopen("Input/surf06-500.dat", "r");

	fscanf(f, "%d", &n_points);
	points = (double **) malloc(n_points * sizeof(double *));

	for(i = 0; i < n_points; ++i) {
		points[i] = (double *) malloc(3 * sizeof(double));
		fscanf(f, "%lf %lf %lf", &points[i][0], &points[i][1], &points[i][2]);
	}
	
	fclose(f);

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; ++i) {
		     if(i % 1000 == 0) visualiser.set_orientation(0, 0, 0);
		else if(i %  500 == 0) visualiser.set_orientation(1, 1, 1);
		visualiser.add_point(points[i]);
		visualiser.compute();
	}
	visualiser.display();

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; ++i) {
		     if(i % 1000 == 0) visualiser.set_orientation(0, 0, 0);
		else if(i %  500 == 0) visualiser.set_orientation(1, 1, 1);
		visualiser.rem_point(points[i]);
		visualiser.compute();
	}
	visualiser.display();

	for(i = 0; i < n_points; ++i)
		free(points[i]);
	free(points);
}

void example6 ()
{
	int n_points, i, j;
	double **points;
	pfv3d visualiser(0, 0, 0, 1);
	FILE *f = fopen("Input/surf05.dat", "r");

	fscanf(f, "%d", &n_points);
	points = (double **) malloc(n_points * sizeof(double *));

	for(i = 0; i < n_points; ++i) {
		points[i] = (double *) malloc(3 * sizeof(double));
		fscanf(f, "%lf %lf %lf", &points[i][0], &points[i][1], &points[i][2]);
	}
	
	fclose(f);

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < 2000; ++i) {
		     if(rand()%40 == 0) visualiser.set_orientation(rand()%3, 0);
		else if(rand()%40 == 0) visualiser.set_orientation(rand()%3, 1);
		for(j = 0; j < rand()%10; ++j) visualiser.add_point(points[rand()%n_points]);
		for(j = 0; j < rand()%10; ++j) visualiser.rem_point(points[rand()%n_points]);
		for(j = 0; j < rand()%10; ++j) visualiser.add_point(points[rand()%n_points]);
		for(j = 0; j < rand()%10; ++j) visualiser.rem_point(points[rand()%n_points]);
		visualiser.compute();
	}
	visualiser.display();

	for(i = 0; i < n_points; ++i)
		free(points[i]);
	free(points);
}
