#include <stdio.h>

/*---------------------btxutil.c------------------*/
float readflowrate(int, int);   /*get pipe flow rate*/
int getADJ();					/*node neighborhood information*/	
int prepareBT(int, float);      /*prepare for backtracking*/
int initparticle(int, float);	/*particle initilization*/
int addpath(int, float, float);
int freepath();
float inflow(int *, int *, float *, float *, int);
float outflow(int);
float netflow(int);
int readhydraulics();
struct Sparticle * generateparticle();
int clearactivelist();
int eraseparticle(struct Sparticle *);
float getpiperate(int, float, float, float);

/*---------------------backtrack.c-----------------*/
int backtrack();
int moveparticle(struct Sparticle*);
int generate_particle_in_tank(float *, float, float, float, float *, int);
int particletotank(struct Sparticle * , int);
void updatetankimpact(int, float);
float flowofmasssource(int);
int splitparticle(struct Sparticle *, int);
