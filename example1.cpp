#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "pfv3d"

int main(int argc, char const *argv[])
{
	int i; 
	pfv3d a(0);
	FILE *f = fopen("Input/surf06-500.dat", "r");
	fscanf(f, "%d", &i);
	double c[i][3];
	int d[i];
	for(int j = 0; j < i; j++) {
		d[j] = j;
		fscanf(f, "%lf %lf %lf", &c[j][0], &c[j][1], &c[j][2]);
	}
	fclose(f);

	std::random_shuffle(&d[0], &d[i]);
	for(int j = 0; j < i; j+=1){
		for(int k = 0; k < 1; k++)
			a.add_point(c[d[j+k]]);
		a.compute(1);
	}
	// a.display();

	std::random_shuffle(&d[0], &d[i]);
	for(int j = 0; j < i; j+=1) {
		for(int k = 0; k < 1; k++)
			a.rem_point(c[d[j+k]]);
		a.compute(0);
	}
	// a.display();

	// std::random_shuffle(&d[0], &d[i]);
	// for(int j = 0; j < 1000; j++) {
	// 	for(int k = 0; k < rand()%10; ++k)
	// 		a.add_point(c[d[rand()%i]]);
	// 	for(int k = 0; k < rand()%10; ++k)
	// 		a.rem_point(c[d[rand()%i]]);
	// 	for(int k = 0; k < rand()%10; ++k)
	// 		a.add_point(c[d[rand()%i]]);
	// 	for(int k = 0; k < rand()%10; ++k)
	// 		a.rem_point(c[d[rand()%i]]);
	// 	a.compute(2);
	// }
	// a.display();
	
	return 0; 
}
