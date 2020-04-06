#include <stdio.h>
#include <float.h>
#include <math.h>
#include <algorithm>
#include "Pdm03.h"

extern void itwo_alloc( int ***ptr, int x, int y);
extern void dtwo_alloc( double ***ptr, int x, int y);
extern void d_alloc(double **var,int size);
extern void i_alloc(int **var,int size);
extern void infil(int nsoil, double dt, double refkdt, double refdk, double kdt, double smcmax, double smcwlt, double *dz, double *zsoil, double *sh2o, double qinsur,double *runsrf,double *pddum);

int main(int argc, char* argv[])
{
FILE *in_fptr;
FILE *out_fptr;
int k,i;
double jdate,dsec;
long year,month,day,hour,minute;
int nsoil;
double dt;
double *dz;
double *sh2o;
double *zsoil;
double qinsur;
double runsrf;
double pddum;
double smcmax;
double smcwlt;
double dksat;
double refdk_data;
double refdk;
double refkdt_data;
double refkdt;
double kdt;  // super important parameter.


if((out_fptr=fopen("inf_vs_rainrate.out","w"))==NULL)
  {printf("Can't open output file\n");exit(0);}

nsoil=4;

d_alloc(&dz,nsoil);
d_alloc(&zsoil,nsoil);
d_alloc(&sh2o,nsoil);

dz[0]=0.0;
dz[1]=-0.10;
dz[2]=-0.30;
dz[3]=-0.60;
dz[4]=-1.00;

smcmax = 0.463999987;
smcwlt = 0.12;

zsoil[0]=0.0;
for(k=1;k<=nsoil;k++)
  {
  sh2o[k]=smcwlt+(smcmax-smcwlt)/2.0;  /* middle of the range from sat. to wilting point */
  zsoil[k]=zsoil[k-1]+dz[k];
  }

sh2o[0] = 0.0;
sh2o[1] = 0.285762608;
sh2o[2] = 0.292563140;
sh2o[3] = 0.272871971;
sh2o[4] = 0.306821257;

// jdate=greg_2_jul(year,month,day,hour,minute,dsec);

/*! ----------------------------------------------------------------------*/
/*! Set-up universal parameters (not dependent on SOILTYP, VEGTYP)        */
/*! ----------------------------------------------------------------------*/
/* DKSAT  = SATDK (SOILTYP) */
dksat=5.23E-6;       /* I think this is m/h? just picked a value from SOILPARM.TBL */
refdk_data=2.0e-06;
refdk=refdk_data;
refkdt_data=3.0;     /* from GENPARM.TBL.  A constant equal to three. */
refkdt = refkdt_data;
// kdt    = refkdt * dksat / refdk;
// the following parameter values are used to compare with Fortran code output
kdt = 3.06;
// dt=1800.0; //seconds
dt = 900.0;
/*! ----------------------------------------------------------------------*/
/* soil type dependent params.                                            */
/*! ----------------------------------------------------------------------*/
// smcmax=0.40;
smcmax = 0.464;
// smcwlt=0.26;
smcwlt = 0.120;
qinsur = 1.49491974E-10;	/* in units of [m/s] */

for(i=1;i<100;i++)
  {
  // qinsur=1.0/3600.0*((double)i/100.0);
  infil(nsoil,dt,refkdt,refdk,kdt,smcmax,smcwlt,dz,zsoil,sh2o,qinsur,&runsrf,&pddum);
  // fprintf(out_fptr,"%lf %lf\n",qinsur,runsrf);
  fprintf(out_fptr,"%e %e %e\n",qinsur,runsrf,pddum);
  }

fclose(out_fptr);

return(0);
}
