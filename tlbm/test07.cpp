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
// Sinking disks

//STD
#include<iostream>

// MechSys
#include <mechsys/lbm/Domain.h>

struct UserData
{
    double Kn;
    Vec3_t g;
    Vec3_t Xmin;
    Vec3_t Xmax;
};

void Setup(LBM::Domain & dom, void * UD)
{
    UserData & dat = (*static_cast<UserData *>(UD));
    for (size_t i=0;i<dom.Lat[0].Cells.Size();i++)
    {
        Cell * c = dom.Lat[0].Cells[i];
        c->BForcef = c->Density()*dat.g;
    }
    for (size_t i=0;i<dom.Particles.Size();i++)
    {
        dom.Particles[i]->Ff = dom.Particles[i]->M*dat.g;
        double delta;
        delta =   dat.Xmin(0) - dom.Particles[i]->X(0) + dom.Particles[i]->R;
        if (delta > 0.0)  dom.Particles[i]->Ff(0) += dat.Kn*delta;
        delta = - dat.Xmax(0) + dom.Particles[i]->X(0) + dom.Particles[i]->R;
        if (delta > 0.0)  dom.Particles[i]->Ff(0) -= dat.Kn*delta;
        delta =   dat.Xmin(1) - dom.Particles[i]->X(1) + dom.Particles[i]->R;
        if (delta > 0.0)  dom.Particles[i]->Ff(1) += dat.Kn*delta;
        delta = - dat.Xmax(1) + dom.Particles[i]->X(1) + dom.Particles[i]->R;
        if (delta > 0.0)  dom.Particles[i]->Ff(1) -= dat.Kn*delta;
        delta =   dat.Xmin(2) - dom.Particles[i]->X(2) + dom.Particles[i]->R;
        if (delta > 0.0)  dom.Particles[i]->Ff(2) += dat.Kn*delta;
        delta = - dat.Xmax(2) + dom.Particles[i]->X(2) + dom.Particles[i]->R;
        if (delta > 0.0)  dom.Particles[i]->Ff(2) -= dat.Kn*delta;
    }
}


int main(int argc, char **argv) try
{
    size_t nx = 100;
    size_t ny = 100;
    size_t nz = 100;
    double nu = 0.015;
    double dx = 1.0;
    double dt = 1.0;
    double rho= 3000.0;
    LBM::Domain Dom(D3Q15, nu, iVec3_t(nx,ny,nz), dx, dt);
    UserData dat;
    Dom.UserData = &dat;
    Dom.Lat[0].G    = -200.0;
    Dom.Lat[0].Gs   = 0.0;
    dat.g        = 0.0,-0.001,0.0;
    dat.Xmin     = 0.0,0.0,0.0;
    dat.Xmax     = nx*dx,ny*dx,nz*dx;
    dat.Kn       = 1.0e5*rho/500.0;

    //Set solid boundaries
    for (size_t i=0;i<nx;i++)
    for (size_t j=0;j<ny;j++)
    {
        Dom.Lat[0].GetCell(iVec3_t(i,0   ,j))->IsSolid = true;
        Dom.Lat[0].GetCell(iVec3_t(i,ny-1,j))->IsSolid = true;
        Dom.Lat[0].GetCell(iVec3_t(i,j,0   ))->IsSolid = true;
        Dom.Lat[0].GetCell(iVec3_t(i,j,ny-1))->IsSolid = true;
        Dom.Lat[0].GetCell(iVec3_t(0   ,i,j))->IsSolid = true;
        Dom.Lat[0].GetCell(iVec3_t(ny-1,i,j))->IsSolid = true;
    }

    for (int i=0;i<nx;i++)
    for (int j=0;j<ny;j++)
    for (int k=0;k<nz;k++)
    {
        Vec3_t v0(0.0,0.0,0.0);
        if (j<ny/2.0) Dom.Lat[0].GetCell(iVec3_t(i,j,k))->Initialize(2300.0 ,v0);
        else          Dom.Lat[0].GetCell(iVec3_t(i,j,k))->Initialize(    .01,v0);
    }

    Dom.AddSphere(0,Vec3_t(0.58*nx*dx,0.65*ny*dx,0.58*nz*dx),OrthoSys::O,OrthoSys::O,0.9*rho,0.1*ny,dt);
    Dom.AddSphere(0,Vec3_t(0.57*nx*dx,0.85*ny*dx,0.57*nz*dx),OrthoSys::O,OrthoSys::O,rho,0.1*ny,dt);
    Dom.AddSphere(0,Vec3_t(0.43*nx*dx,0.65*ny*dx,0.42*nz*dx),OrthoSys::O,OrthoSys::O,rho,0.1*ny,dt);
    Dom.AddSphere(0,Vec3_t(0.36*nx*dx,0.85*ny*dx,0.63*nz*dx),OrthoSys::O,OrthoSys::O,0.3*rho,0.1*ny,dt);
    Dom.AddSphere(0,Vec3_t(0.70*nx*dx,0.65*ny*dx,0.40*nz*dx),OrthoSys::O,OrthoSys::O,0.6*rho,0.1*ny,dt);
    for (size_t i=0;i<Dom.Particles.Size();i++)
    {
        Dom.Particles[i]->Kn = 1.0e5*rho/500.0;
    }

    //Solving
    Dom.Solve(4000.0,20.0,Setup,NULL,"test07");
    //Dom.Solve(10000.0,20.0,NULL,NULL,"test05");
}
MECHSYS_CATCH

