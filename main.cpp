#include <stdio.h>
#include <stdlib.h>
#include "pfv3d.h"

int main(int argc, char const *argv[])
{
	pfv3d a;
	FILE *f = fopen("Input/surf05-500.dat", "r");
	int i; fscanf(f, "%d", &i);
	double c[3];
	for(int j = 0; j < i; j++) {
		fscanf(f, "%lf %lf %lf", &c[0], &c[1], &c[2]);
		a.add_point(c);
	}
	a.compute();
	a.display();
	return 0; 
}
