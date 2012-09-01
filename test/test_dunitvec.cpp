/************************************************************************
 * MechSys - Open Library for Mechanical Systems                        *
 * Copyright (C) 2005 Dorival M. Pedroso, Raúl D. D. Farfan             *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * any later version.                                                   *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>  *
 ************************************************************************/

// Std Lib
#include <iostream>

// GSL
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_odeiv.h>

// MechSys
#include <mechsys/numerical/numdiff.h>
#include <mechsys/linalg/matvec.h>
#include <mechsys/util/fatal.h>
#include <mechsys/util/numstreams.h>

using std::cout;
using std::endl;
using Util::_8s;
using Util::PI;
using Util::SQ2;
using Util::SQ3;
using Util::SQ6;
using Numerical::Diff;

class Problem
{
public:
    // Constructor
    Problem () : test (2)
    {
        n0    . change_dim(3);
        n     . change_dim(3);
        nu    . change_dim(3);
        dndt  . change_dim(3);
        dnudt . change_dim(3);
        dnudn . change_dim(3,3);
        m     . change_dim(3,3);
        dmdt  . change_dim(3,3);
        mI    . change_dim(3,3);
        n0    =  1.0, 2.0, 3.0;
        mI    =  1.0, 0.0, 0.0,
                 0.0, 1.0, 0.0,
                 0.0, 0.0, 1.0;
    }

    // Methods
    void CalcState (double t)
    {
        if (test==1)
        {
            n    = (1.+t*t)*n0;
            dndt = (2.*t)*n0;
        }
        else
        {
            m    = t,    0.,      0.,
                   0.,  t*t,      0.,
                   0.,   0.,  sin(t);
            dmdt = 1.,   0.,      0.,
                   0., 2.*t,      0.,
                   0.,   0.,  cos(t);
            n    = 0.1*n0 + m*n0;
            dndt = dmdt*n0;
        }

        // derivative of unit vector
        UnitVecDeriv (n, nu, dnudn);
        dnudt = dnudn * dndt;

        // blitz++
        Vec3_t tmp_n, tmp_nu;
        Mat3_t tmp_dnudn;
        tmp_n = n(0), n(1), n(2);
        UnitVecDeriv (tmp_n, tmp_nu, tmp_dnudn);
        for (size_t i=0; i<3; ++i)
        {
            double error = fabs(tmp_nu(i)-nu(i));
            if (error>1.0e-15) throw new Fatal("Problem::CalcState: blitz' UnitVecDeriv gives different nu than Mat_t function. error = %g",error);
            for (size_t j=0; j<3; ++j)
            {
                double err = fabs(tmp_dnudn(i,j)-dnudn(i,j));
                if (err>1.0e-15) throw new Fatal("Problem::CalcState: blitz' UnitVecDeriv gives different dnudn than Mat_t function. err = %g",err);
            }
        }
    }

    // Functions
    double nu0Fun (double t) { CalcState(t); return nu(0); }
    double nu1Fun (double t) { CalcState(t); return nu(1); }
    double nu2Fun (double t) { CalcState(t); return nu(2); }

    // Data
    int   test;                    // test number
    Vec_t n0, n, nu, dndt, dnudt;  // normal and unit normal vectors
    Mat_t dnudn;                   // derivative of unit vector
    Mat_t m, dmdt;                 // multiplier matrix
    Mat_t mI;                      // I matrix
};

typedef double (Problem::*pFun) (double t);

int main(int argc, char **argv) try
{
    // initialize problem
    Problem       prob;
    Diff<Problem> nd(&prob);
    bool   verbose = false;
    size_t ndiv    = 20;
    if (argc>1) prob.test = atoi(argv[1]);
    if (argc>2) verbose   = atoi(argv[2]);
    if (argc>3) ndiv      = atoi(argv[3]);

    // unit vector
    double max_err_dnudt[3] = {0.,0.,0.};
    pFun   nu_funcs     [3] = {&Problem::nu0Fun, &Problem::nu1Fun, &Problem::nu2Fun};
    if (verbose)
    {
        printf("\n --------------------- unit vector ----------------------\n\n");
        printf("\n%6s","t");
        char str0[32], str1[32], str2[32], str3[32], str4[32], str5[32];
        for (size_t k=0; k<3; ++k)
        {
            sprintf(str0,"n%zd",           k);
            sprintf(str1,"nu%zd",          k);
            sprintf(str2,"dn%zddt",        k);
            sprintf(str3,"dnu%zddt_num",   k);
            sprintf(str4,"dnu%zddt",       k);
            sprintf(str5,"error(dnu%zddt)",k);
            printf("%12s %12s %12s %12s %12s %16s  ",str0,str1,str2,str3,str4,str5);
        }
        printf("\n");
    }
    for (size_t i=0; i<ndiv+1; ++i)
    {
        double t = (double)i/(double)ndiv;
        prob.CalcState (t);
        if (verbose) printf("%6.3f",t);
        for (size_t k=0; k<3; ++k)
        {
            double dnudt_num = nd.DyDx (nu_funcs[k], t);
            double err      = fabs(dnudt_num - prob.dnudt(k));
            if (err > max_err_dnudt[k]) max_err_dnudt[k] = err;
            if (verbose) printf("%12.8f %12.8f %12.8f %12.8f %12.8f %16.8e  ", prob.n(k), prob.nu(k), prob.dndt(k), dnudt_num, prob.dnudt(k), err);
        }
        if (verbose) printf("\n");
    }

    // error
    double tol_dnudt[3] = {1.0e-7, 1.0e-6, 1.0e-6};
    printf("\n");
    for (size_t k=0; k<3; ++k) printf("  max_err_dnu%zddt = %s%16.8e%s\n",k,(max_err_dnudt[k]>tol_dnudt[k]?TERM_RED:TERM_GREEN),max_err_dnudt[k],TERM_RST);
    printf("\n");

    // end
    for (size_t k=0; k<3; ++k) if (max_err_dnudt[k] > tol_dnudt[k]) return 1;
    return 0;
}
MECHSYS_CATCH
