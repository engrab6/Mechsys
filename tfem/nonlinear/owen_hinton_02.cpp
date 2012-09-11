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

/*  Owen & Hinton (1980): Example 7.9, p262  *
 *  Finite Elements in Plasticity            *
 *  ======================================== */

// STL
#include <iostream>

// MechSys
#include <mechsys/mesh/structured.h>
#include <mechsys/fem/elems/quad8.h>
#include <mechsys/fem/equilibelem.h>
#include <mechsys/models/linelastic.h>
#include <mechsys/models/elastoplastic.h>
#include <mechsys/fem/domain.h>
#include <mechsys/fem/solvers/stdsolver.h>
#include <mechsys/util/maps.h>
#include <mechsys/util/fatal.h>

using std::cout;
using std::endl;
using FEM::PROB;
using FEM::GEOM;
using Util::_4;
using Util::_6_3;
using Util::_8s;
const double TRUE = 1.0;

const double DelP = 19.0;
const size_t NInc = 19;

const double nip = 9.0;

struct OutDat
{
    std::ofstream of;
    ~OutDat () { of.close (); }
    OutDat (String const & FKey) : fk(FKey), nstg(1)
    {
        String fn;   fn.Printf ("%s_n41.res", FKey.CStr());
        of.open (fn.CStr(),std::ios::out);
        of<<_6_3<<"Time" << _8s<< "P" <<_8s<<"ur"<< _8s<<"fr_int"<<_8s<<"fr_ext\n"; // radial displacement and forces
    }
    String fk;
    int    nstg;
};

void OutFun (FEM::Solver * Sol, void * Dat)
{
    FEM::STDSolver * sol = static_cast<FEM::STDSolver*>(Sol);
    OutDat * dat = static_cast<OutDat*>(Dat);

    // current P
    double P = sol->Dom.Time*(DelP/dat->nstg);

    //////////////////////////////////////////////////////////////////////////////////// Control Node /////
    
    {
        size_t inod = 41;
        FEM::Node const & nod = (*sol->Dom.Nods[inod]);
        int    eqx    = nod.Eq("ux");
        int    eqy    = nod.Eq("uy");
        double ux     = sol->U    (eqx),   uy     = sol->U    (eqy);
        double fx     = sol->F    (eqx),   fy     = sol->F    (eqy);
        double fx_int = sol->F_int(eqx),   fy_int = sol->F_int(eqy);
        double ur     = sqrt(ux*ux + uy*uy);
        double fr     = sqrt(fx*fx + fy*fy);
        double fr_int = sqrt(fx_int*fx_int + fy_int*fy_int);
        dat->of << _6_3 << sol->Dom.Time << _8s << P << _8s << ur << _8s << fr_int << _8s << fr << endl;
    }

    /////////////////////////////////////////////////////////////////////////////////////////// Elems /////

    {
        // header
        String fn;  fn.Printf("%s_P%g.res", dat->fk.CStr(), P);
        std::ofstream of(fn.CStr(), std::ios::out);
        of << _8s<<"P" << _8s<< "r" << _8s<< "sr" << _8s<< "st" << _8s<< "srt" << "\n";

        // results
        Array<int> eles(4);
        eles = 4, 5, 6, 7;
        for (size_t i=0; i<eles.Size(); ++i)
        {
            FEM::Element const & ele = (*sol->Dom.Eles[i]);
            Array<SDPair> res;
            ele.StateAtIPs (res);
            for (size_t j=0; j<ele.GE->NIP; ++j)
            {
                bool found = false;
                if ((int)nip == 4) {
                    found = fabs(ele.GE->IPs[j].s - (-sqrt(3.0/5.0)))<1.0e-5;
                } else {
                    found = fabs(ele.GE->IPs[j].s)<1.0e-5;
                }
                if (found) // mid point
                {
                    // coordinates of IP
                    Vec_t X;
                    ele.CoordsOfIP (j, X);
                    double x  = X(0);
                    double y  = X(1);
                    double r  = sqrt(x*x+y*y);
                    double c  = x/r;
                    double s  = y/r;
                    double cc = c*c;
                    double ss = s*s;
                    double cs = c*s;

                    // rotation to r-t coordinates
                    double sx  = res[j]("sx");
                    double sy  = res[j]("sy");
                    double sxy = res[j]("sxy");
                    double sr  =  cc*sx + ss*sy +  2.0*cs*sxy;
                    double st  =  ss*sx + cc*sy -  2.0*cs*sxy;
                    double srt = -cs*sx + cs*sy + (cc-ss)*sxy;

                    // output
                    of << _8s<< P << _8s<< r << _8s<< sr << _8s<< st << _8s<< srt << "\n";
                }
            }
        }
        of.close();
    }
}

int main(int argc, char **argv) try
{
    bool two_stages = false;
    if (argc>1) two_stages = atoi(argv[1]);

    ///////////////////////////////////////////////////////////////////////////////////////// Mesh /////
    
    String extra("\
from msys_fig import *\n\
dat = read_table('owen_hinton_02_mesh.dat')\n\
plot(dat['x'],dat['y'],'ro',lw=3)\n");
    Mesh::Structured mesh(/*NDim*/2);
    //mesh.GenQRing (/*O2*/true,/*Nx*/4,/*Ny*/1,/*r*/100.,/*R*/200.,/*Nb*/3,/*Ax*/1.0); // w = 1 + Ax*i
    mesh.GenQRing (/*O2*/true,/*Nx*/0,/*Ny*/1,/*r*/100.,/*R*/200.,/*Nb*/3,/*Ax*/0.0,/*NonLin*/false,/*Wx*/"1.661998255 2.1643892556 3.0339121415 3.0918803339");
    mesh.WriteMPY ("owen_hinton_02", /*WithTags*/true, /*WithIDs*/true, /*WithShares*/false, extra.CStr());
    mesh.WriteVTU ("owen_hinton_02", /*VolSurfOrBoth: Vol=0, Surf=1, Both=2*/0);

    ////////////////////////////////////////////////////////////////////////////////////////// FEM /////

    // elements properties
    Dict prps;
    prps.Set(-1, "prob geom psa rho nip", PROB("Equilib"), GEOM("Quad8"), TRUE, 1.0, nip);

    // models
    Dict mdls;
    //mdls.Set(-1, "name E nu psa", MODEL("LinElastic"), 2.1e+4, 0.3, TRUE);
    mdls.Set(-1, "name E nu VM sY psa rho", MODEL("ElastoPlastic"), 2.1e+4, 0.3, TRUE, 24.0, TRUE, 1.0);

    // initial values
    Dict inis;
    inis.Set(-1, "sx sy sz sxy", 0.0,0.0,0.0,0.0);

    // domain
    Array<int> out_verts(41, /*JustOne*/true);
    FEM::Domain dom(mesh, prps, mdls, inis, "owen_hinton_02", &out_verts);

    // solver
    OutDat dat_stg1("owen_hinton_02_stg1");  dat_stg1.nstg = (two_stages ? 2 : 1);
    SDPair flags;
    flags.Set("nr", 1.);
    FEM::STDSolver sol(dom, flags, &OutFun, &dat_stg1);

    // stage # 1 -----------------------------------------------------------
    double dp = (two_stages ? DelP/2.0 : DelP);
    Dict bcs;
    bcs.Set(-10, "uy", 0.0);
    bcs.Set(-30, "ux", 0.0);
    bcs.Set(-40, "qn", -dp);
    dom.SetBCs (bcs);
    sol.Solve (NInc);

    // stage # 2 -----------------------------------------------------------
    if (two_stages)
    {
        //dom.SaveState ("owen_hinton_02");
        //dom.LoadState ("owen_hinton_02");
        OutDat dat_stg2("owen_hinton_02_stg2");  dat_stg2.nstg = (two_stages ? 2 : 1);
        sol.OutDat = &dat_stg2;
        dom.SetBCs (bcs);
        sol.Solve  (NInc);
    }

    //////////////////////////////////////////////////////////////////////////////////////// Output ////

    // draw elements with IPs
    String ext("\
from msys_fig import *\n\
A = linspace(0.0,pi/2.0,200)\n\
X = 100.0*cos(A)\n\
Y = 100.0*sin(A)\n\
plot(X,Y,'r-',lw=2)\n\
X = 200.0*cos(A)\n\
Y = 200.0*sin(A)\n\
plot(X,Y,'r-',lw=2)\n");
    FEM::MPyPrms mpy_prms;
    mpy_prms.Extra = ext.CStr();
    dom.WriteMPY ("owen_hinton_02_elems", mpy_prms);

    return 0;
}
MECHSYS_CATCH
