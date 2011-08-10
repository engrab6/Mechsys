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

/*  Bhatti (2005): Example 4.2, p230  *
 *  ================================  */

// STL
#include <iostream>

// MechSys
#include <mechsys/mesh/mesh.h>
#include <mechsys/fem/rod.h>
#include <mechsys/fem/domain.h>
#include <mechsys/fem/solvers/stdsolver.h>
#include <mechsys/util/maps.h>
#include <mechsys/util/fatal.h>

using std::cout;
using std::endl;
using FEM::PROB;
using FEM::GEOM;

int main(int argc, char **argv) try
{
    ///////////////////////////////////////////////////////////////////////////////////////// Mesh /////

    Mesh::Generic mesh(/*NDim*/3);
    mesh.SetSize  (4/*verts*/, 3/*cells*/);
    mesh.SetVert  (0, -100,   960, 1920,  0.0);
    mesh.SetVert  (1, -100, -1440, 1440,  0.0);
    mesh.SetVert  (2, -100,   0.0,  0.0,  0.0);
    mesh.SetVert  (3, -200,   0.0,  0.0, 2000);
    mesh.SetCell  (0,   -1, Array<int>(0, 3));
    mesh.SetCell  (1,   -1, Array<int>(1, 3));
    mesh.SetCell  (2,   -2, Array<int>(2, 3));
    //mesh.WriteVTU ("ex42");
    //cout << mesh << endl;

    ////////////////////////////////////////////////////////////////////////////////////////// FEM /////

    // elements properties
    Dict prps;
    prps.Set(-1, "prob active E A fra", PROB("Rod"), 1.0, 210000.0, 200.0, 1.0);
    prps.Set(-2, "prob active E A fra", PROB("Rod"), 1.0, 210000.0, 600.0, 1.0);

    // domain
    FEM::Domain dom(mesh, prps, /*mdls*/Dict(), /*inis*/Dict());

    // check matrices
    {
        double tol   = 1.0e-9;
        double error = 0.0;
        Mat_t K0c(6,6),K1c(6,6),K2c(6,6);
        K0c =
          1.5326336939063351e+03,  3.0652673878126702e+03, -3.1929868623048642e+03, -1.5326336939063351e+03, -3.0652673878126702e+03,  3.1929868623048642e+03,
          3.0652673878126702e+03,  6.1305347756253404e+03, -6.3859737246097284e+03, -3.0652673878126702e+03, -6.1305347756253404e+03,  6.3859737246097284e+03,
         -3.1929868623048642e+03, -6.3859737246097284e+03,  6.6520559631351334e+03,  3.1929868623048642e+03,  6.3859737246097284e+03, -6.6520559631351334e+03,
         -1.5326336939063351e+03, -3.0652673878126702e+03,  3.1929868623048642e+03,  1.5326336939063351e+03,  3.0652673878126702e+03, -3.1929868623048642e+03,
         -3.0652673878126702e+03, -6.1305347756253404e+03,  6.3859737246097284e+03,  3.0652673878126702e+03,  6.1305347756253404e+03, -6.3859737246097284e+03,
          3.1929868623048642e+03,  6.3859737246097284e+03, -6.6520559631351334e+03, -3.1929868623048642e+03, -6.3859737246097284e+03,  6.6520559631351334e+03;
        K1c =
          3.7450852505723842e+03, -3.7450852505723842e+03,  5.2015072924616452e+03, -3.7450852505723842e+03,  3.7450852505723842e+03, -5.2015072924616452e+03,
         -3.7450852505723842e+03,  3.7450852505723842e+03, -5.2015072924616452e+03,  3.7450852505723842e+03, -3.7450852505723842e+03,  5.2015072924616452e+03,
          5.2015072924616452e+03, -5.2015072924616452e+03,  7.2243156839745079e+03, -5.2015072924616452e+03,  5.2015072924616452e+03, -7.2243156839745079e+03,
         -3.7450852505723842e+03,  3.7450852505723842e+03, -5.2015072924616452e+03,  3.7450852505723842e+03, -3.7450852505723842e+03,  5.2015072924616452e+03,
          3.7450852505723842e+03, -3.7450852505723842e+03,  5.2015072924616452e+03, -3.7450852505723842e+03,  3.7450852505723842e+03, -5.2015072924616452e+03,
         -5.2015072924616452e+03,  5.2015072924616452e+03, -7.2243156839745079e+03,  5.2015072924616452e+03, -5.2015072924616452e+03,  7.2243156839745079e+03;
        K2c =
          0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,
          0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,
          0.0000000000000000e+00,  0.0000000000000000e+00,  6.3000000000000000e+04,  0.0000000000000000e+00,  0.0000000000000000e+00, -6.3000000000000000e+04,
          0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,
          0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,  0.0000000000000000e+00,
          0.0000000000000000e+00,  0.0000000000000000e+00, -6.3000000000000000e+04,  0.0000000000000000e+00,  0.0000000000000000e+00,  6.3000000000000000e+04;
        Mat_t K0,K1,K2;
        dom.Eles[0]->CalcK(K0);
        dom.Eles[1]->CalcK(K1);
        dom.Eles[2]->CalcK(K2);
        error += CompareMatrices (K0,K0c);
        error += CompareMatrices (K1,K1c);
        error += CompareMatrices (K2,K2c);
        cout << "\n[1;37m--- Matrices: Error ----------------------------------------------------------[0m\n";
        cout << "error (K) = " << (error>tol ? "[1;31m" : "[1;32m") << error << "[0m" << endl;
    }

    // solver
    SDPair flags;
    FEM::STDSolver sol(dom, flags);

    // stage # 1 -----------------------------------------------------------
    Dict bcs;
    bcs.Set(-100, "ux uy uz", 0.0,0.0,0.0);
    bcs.Set(-200, "fy", -20000.0);
    dom.SetBCs (bcs);
    sol.Solve  ();

    //////////////////////////////////////////////////////////////////////////////////////// Output ////

    dom.PrintResults ("%11.6g");

    //////////////////////////////////////////////////////////////////////////////////////// Check /////

    // correct solution
    Table nod_sol;
    nod_sol.Set("                  ux                      uy                      uz", /*NRows*/4,
                0.000000000000000e+00,  0.000000000000000e+00,  0.000000000000000e+00,
                0.000000000000000e+00,  0.000000000000000e+00,  0.000000000000000e+00,
                0.000000000000000e+00,  0.000000000000000e+00,  0.000000000000000e+00,
               -1.781429675607220e-01, -2.468574484639203e+00, -3.674309229864785e-01);

    Table ele_sol;
    ele_sol.Set("                    N", /*NRows*/3,
                 2.037457868988028e+04,
                 1.321449094437872e+04,
                -2.314814814814814e+04);

    // error tolerance
    SDPair nod_tol, ele_tol;
    nod_tol.Set("ux uy uz", 1.0e-15,1.0e-15,1.0e-15);
    ele_tol.Set("N",        1.0e-10);

    // return error flag
    bool err1 = dom.CheckErrorNods(nod_sol, nod_tol);
    bool err2 = dom.CheckErrorEles(ele_sol, ele_tol);
    return (err1 || err2);
}
MECHSYS_CATCH
