#include <math.h>


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
