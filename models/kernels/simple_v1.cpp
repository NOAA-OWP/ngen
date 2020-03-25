/*
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
*/
#include <iostream>
#include <cmath>
#include <cstring>
using namespace std;

extern double greg_2_jul(long year, long mon, long day, long h, long mi,
                         double se);
extern void calc_date(double jd, long *y, long *m, long *d, long *h, long *mi,
               double *sec);

extern void itwo_alloc( int ***ptr, int x, int y);
extern void dtwo_alloc( double ***ptr, int x, int y);
extern void d_alloc(double **var,int size);
extern void i_alloc(int **var,int size);
extern void infil(int nsoil, double dt, double refkdt, double refdk, double kdt, double smcmax, double smcwlt, double *dz, double *zsoil, double *sh2o, double qinsur,double *runsrf,double *pddum);

// int main(int argc, char argv[])
// int main(int argc, char** argv)
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
qinsur = 1.49491974E-10;

for(i=1;i<100;i++)
  {
  // qinsur=1.0/3600.0*((double)i/100.0);   /* mm/s */ 
  infil(nsoil,dt,refkdt,refdk,kdt,smcmax,smcwlt,dz,zsoil,sh2o,qinsur,&runsrf,&pddum);
  // fprintf(out_fptr,"%lf %lf\n",qinsur,runsrf);
  fprintf(out_fptr,"%e %e %e\n",qinsur,runsrf,pddum);
  }

fclose(out_fptr);

return(0);
}

void infil(int nsoil, double dt, double refkdt, double refdk, double kdt, double smcmax, double smcwlt, 
           double *dz, double *zsoil, double *sh2o, double qinsur,double *runsrf,double *pddum)
{


/*! ===============================================================================
  SUBROUTINE INFIL (NSOIL  ,DT     ,ZSOIL  ,SH2O   ,SICE   , & !in
                    SICEMAX,QINSUR ,                         & !in
                    PDDUM  ,RUNSRF )                           !out
! --------------------------------------------------------------------------------
! compute inflitration rate at soil surface and surface runoff
! --------------------------------------------------------------------------------
    IMPLICIT NONE
! --------------------------------------------------------------------------------
! inputs
  INTEGER,                  INTENT(IN) :: NSOIL  !no. of soil layers
  REAL,                     INTENT(IN) :: DT     !time step (sec)
  REAL, DIMENSION(1:NSOIL), INTENT(IN) :: ZSOIL  !depth of soil layer-bottom [m]
  REAL, DIMENSION(1:NSOIL), INTENT(IN) :: SH2O   !soil liquid water content [m3/m3]
  REAL, DIMENSION(1:NSOIL), INTENT(IN) :: SICE   !soil ice content [m3/m3]
  REAL,                     INTENT(IN) :: QINSUR !water input on soil surface [mm/s]
  REAL,                     INTENT(IN) :: SICEMAX!maximum soil ice content (m3/m3)

! outputs
  REAL,                    INTENT(OUT) :: RUNSRF !surface runoff [mm/s] 
  REAL,                    INTENT(OUT) :: PDDUM  !infiltration rate at surface

! locals
  INTEGER :: IALP1, J, JJ,  K
  REAL                     :: VAL
  REAL                     :: DDT
  REAL                     :: PX
  REAL                     :: DT1, DD, DICE
  REAL                     :: FCR
  REAL                     :: SUM
  REAL                     :: ACRT
  REAL                     :: WDF
  REAL                     :: WCND
  REAL                     :: SMCAV
  REAL                     :: INFMAX
  REAL, DIMENSION(1:NSOIL) :: DMAX
  INTEGER, PARAMETER       :: CVFRZ = 3
--------------------------------------------------------------------------------*/
int k;
double dt1,dd,val,ddt,px,smcav,infmax;
double dmax[5];

if (qinsur >  0.0) 
  {
  dt1 = dt /86400.0;
  smcav = smcmax - smcwlt;

// maximum infiltration rate

  dmax[1]= -zsoil[1] * smcav;
  dmax[1]= dmax[1]* (1.0-(sh2o[1] - smcwlt)/smcav);
  for(k=1;k<=nsoil;k++)
  for(k=1;k<=nsoil;k++)

  dd = dmax[1];

  for(k = 2;k<=nsoil;k++)
    {
    dmax[k] = (zsoil[k-1] - zsoil[k]) * smcav;
    dmax[k] = dmax[k] * (1.0-(sh2o[k] - smcwlt)/smcav);
    dd      = dd + dmax[k];
    }

  val = (1.0 - exp ( - kdt * dt1));
  ddt = dd * val;

  if(qinsur>0.0) px=qinsur*dt;
  else           px=0.0;
  
  /* px  = max(0.,qinsur * dt) */
  infmax = (px * (ddt / (px + ddt)))/ dt;


  if((qinsur-infmax)>0.0)  *runsrf= qinsur - infmax;
  else                     *runsrf=0.0;
  *pddum = qinsur - (*runsrf);
  }

return;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/* ALL THE STUFF BELOW HERE IS JUST UTILITY MEMORY AND TIME FUNCTION CODE */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

/****************************************/
void itwo_alloc(int ***array,int rows, int cols)
{
int  i,frows,fcols, numgood=0;
int error=0;

if ((rows==0)||(cols==0))
  {
  printf("Error: Attempting to allocate array of size 0\n");
  exit;
  }

frows=rows+1;  /* added one for FORTRAN numbering */
fcols=cols+1;  /* added one for FORTRAN numbering */

*array=(int **)malloc(frows*sizeof(int *));
if (*array) 
  {
  memset((*array), 0, frows*sizeof(int*));
  for (i=0; i<frows; i++)
    {
    (*array)[i] =(int *)malloc(fcols*sizeof(int ));
    if ((*array)[i] == NULL)
      {
      error = 1;
      numgood = i;
      i = frows;
      }
     else memset((*array)[i], 0, fcols*sizeof(int )); 
     }
   }
return;
}



void dtwo_alloc(double ***array,int rows, int cols)
{
int  i,frows,fcols, numgood=0;
int error=0;

if ((rows==0)||(cols==0))
  {
  printf("Error: Attempting to allocate array of size 0\n");
  exit;
  }

frows=rows+1;  /* added one for FORTRAN numbering */
fcols=cols+1;  /* added one for FORTRAN numbering */

*array=(double **)malloc(frows*sizeof(double *));
if (*array) 
  {
  memset((*array), 0, frows*sizeof(double *));
  for (i=0; i<frows; i++)
    {
    (*array)[i] =(double *)malloc(fcols*sizeof(double ));
    if ((*array)[i] == NULL)
      {
      error = 1;
      numgood = i;
      i = frows;
      }
     else memset((*array)[i], 0, fcols*sizeof(double )); 
     }
   }
return;
}



void d_alloc(double **var,int size)
{
  size++;  /* just for safety */

   *var = (double *)malloc(size * sizeof(double));
   if (*var == NULL)
      {
      printf("Problem allocating memory for array in d_alloc.\n");
      return;
      }
   else memset(*var,0,size*sizeof(double));
   return;
}

void i_alloc(int **var,int size)
{
   size++;  /* just for safety */

   *var = (int *)malloc(size * sizeof(int));
   if (*var == NULL)
      {
      printf("Problem allocating memory in i_alloc\n");
      return; 
      }
   else memset(*var,0,size*sizeof(int));
   return;
}

/*
 * convert Gregorian days to Julian date
 *
 * Modify as needed for your application.
 *
 * The Julian day starts at noon of the Gregorian day and extends
 * to noon the next Gregorian day.
 *
 */
/*
** Takes a date, and returns a Julian day. A Julian day is the number of
** days since some base date  (in the very distant past).
** Handy for getting date of x number of days after a given Julian date
** (use jdate to get that from the Gregorian date).
** Author: Robert G. Tantzen, translator: Nat Howard
** Translated from the algol original in Collected Algorithms of CACM
** (This and jdate are algorithm 199).
*/


double greg_2_jul(
long year, 
long mon, 
long day, 
long h, 
long mi, 
double se)
{
    long m = mon, d = day, y = year;
    long c, ya, j;
    double seconds = h * 3600.0 + mi * 60 + se;

    if (m > 2)
	m -= 3;
    else {
	m += 9;
	--y;
    }
    c = y / 100L;
    ya = y - (100L * c);
    j = (146097L * c) / 4L + (1461L * ya) / 4L + (153L * m + 2L) / 5L + d + 1721119L;
    if (seconds < 12 * 3600.0) {
	j--;
	seconds += 12.0 * 3600.0;
    }
    else {
	seconds = seconds - 12.0 * 3600.0;
    }
    return (j + (seconds / 3600.0) / 24.0);
}

/* Julian date converter. Takes a julian date (the number of days since
** some distant epoch or other), and returns an int pointer to static space.
** ip[0] = month;
** ip[1] = day of month;
** ip[2] = year (actual year, like 1977, not 77 unless it was  77 a.d.);
** ip[3] = day of week (0->Sunday to 6->Saturday)
** These are Gregorian.
** Copied from Algorithm 199 in Collected algorithms of the CACM
** Author: Robert G. Tantzen, Translator: Nat Howard
**
** Modified by FLO 4/99 to account for nagging round off error 
**
*/
void calc_date(double jd, long *y, long *m, long *d, long *h, long *mi,
               double *sec)
{
    static int ret[4];

    long j;
    double tmp; 
    double frac;

    j=(long)jd;
    frac = jd - j;

    if (frac >= 0.5) {
	frac = frac - 0.5;
        j++;
    }
    else {
	frac = frac + 0.5;
    }

    ret[3] = (j + 1L) % 7L;
    j -= 1721119L;
    *y = (4L * j - 1L) / 146097L;
    j = 4L * j - 1L - 146097L * *y;
    *d = j / 4L;
    j = (4L * *d + 3L) / 1461L;
    *d = 4L * *d + 3L - 1461L * j;
    *d = (*d + 4L) / 4L;
    *m = (5L * *d - 3L) / 153L;
    *d = 5L * *d - 3 - 153L * *m;
    *d = (*d + 5L) / 5L;
    *y = 100L * *y + j;
    if (*m < 10)
	*m += 3;
    else {
	*m -= 9;
	*y=*y+1; /* Invalid use: *y++. Modified by Tony */
    }

    /* if (*m < 3) *y++; */
    /* incorrectly repeated the above if-else statement. Deleted by Tony.*/

    tmp = 3600.0 * (frac * 24.0);
    *h = (long) (tmp / 3600.0);
    tmp = tmp - *h * 3600.0;
    *mi = (long) (tmp / 60.0);
    *sec = tmp - *mi * 60.0;
}

int dayofweek(double j)
{
    j += 0.5;
    return (int) (j + 1) % 7;
}

