/************************************************************************
 * MechSys - Open Library for Mechanical Systems                        *
 * Copyright (C) 2005 Dorival M. Pedroso, Raul Durand                   *
 * Copyright (C) 2009 Sergio Galindo                                    *
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

// MechSys
#include <mechsys/dem/domain.h>

int main(int argc, char **argv) try
{
    // domain
    DEM::Domain d;
    d.Xmax =  15.0;
    d.Xmin =  15.0;
    d.Alpha=   1.0;

    // add cube
	Vec3_t x(-10,0,0);    // position
    Vec3_t w(0,M_PI/5,0); //  rot veloc
    Vec3_t v(1.,0,0);     // veloc
	d.AddCube (-1, x,0.3,3.,1.);
    d.Particles[0]->v = v;
    d.Particles[0]->w = w;

    // add tetrahedron
    x =  10 , 0 , 0;
    w =   0 , 0 , M_PI/10.;
    v = -1. , 0 , 0;
    d.AddTetra (-2, x,0.5,5.,1.);
    d.Particles[1]->v = v;
    d.Particles[1]->w = w;
    
	//Vec3_t x(-10,0,0);    // position
    //Vec3_t v(1.,0,0);     // veloc
	//d.AddCube (-1, x,0.3,3.,1.,0.0,&OrthoSys::e1);
    //d.Particles[0]->v = v;
//
    //x =  10 , 0 , 0;
    //v = -1. , 0 , 0;
	//d.AddCube (-2, x,0.3,3.,1.,0.0,&OrthoSys::e1);
    //d.Particles[1]->v = v;
    
    // Particle parameters
    Dict B;
    B.Set(-1,"Gn Gt Mu",0.0,0.0,0.0);
    B.Set(-2,"Gn Gt Mu",0.0,0.0,0.0);
    d.SetProps(B);

    // initial constants
    Vec3_t l0(0,0,0);  // initial linear momentum
    Vec3_t p0(0,0,0);  // initial angular momentum
    double Ek0,Ep0,E0; // initial energy
    d.LinearMomentum  (p0);
    d.AngularMomentum (l0);
    E0 = d.CalcEnergy (Ek0,Ep0);


    // solve
    d.CamPos = 0.0,30.0,0.0;
    d.Solve(/*tf*/100.0, 1.0e-5, /*dtOut*/0.3, NULL, NULL, "test_dynamics", true, 1);

    // final constants
    Vec3_t l1(0,0,0);  // initial linear momentum
    Vec3_t p1(0,0,0);  // initial angular momentum
    double Ek1,Ep1,E1; // initial energy
    d.LinearMomentum  (p1);
    d.AngularMomentum (l1);
    E1 = d.CalcEnergy (Ek1,Ep1); 

    // check
    double tol   = 1.0e-3;
    double err_l = norm(l1-l0);
    double err_p = norm(p1-p0);
    double err_E = fabs(E1-E0);
    double error = err_l + err_p + err_E;
    cout << "Error in energy           = " << err_E <<endl;
    cout << "Error in angular momentum = " << err_l <<endl;
    cout << "Error in linear  momentum = " << err_p <<endl;


    // results
    if (error>tol) return 1;
    else           return 0;
}
MECHSYS_CATCH
