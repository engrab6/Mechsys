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

/*            12
     o|\ +-----------+ /|o
     o|/ |  embank 2 | \|o
         |-----------|
         |  embank 1 |  12
         |-----------|
         |           |
         +-----------+
        /_\         /_\
        o o         o o
*/

// STL
#include <iostream>

// MechSys
#include "fem/geometry.h"
#include "fem/functions.h"
#include "fem/elems/quad4pstrain.h"
#include "fem/elems/quad8pstrain.h"
#include "models/equilibs/linelastic.h"
#include "fem/solvers/forwardeuler.h"
#include "fem/solvers/autome.h"
#include "fem/output.h"
#include "util/exception.h"
#include "linalg/matrix.h"
#include "mesh/structured.h"

using std::cout;
using std::endl;
using LinAlg::Matrix;
using Util::_4;
using Util::_8s;
using boost::make_tuple;

int main(int argc, char **argv) try
{
	// Constants
	double E     = 5000.0; // Young
	double nu    = 0.3;    // Poisson
	int    ndivx = 48;     // number of divisions along x and y (for each block)
	int    ndivy = 16;     // number of divisions along x and y (for each block)
	bool   is_o2 = false;  // use high order elements?

	// Input
	cout << "Input: " << argv[0] << "  is_o2  ndivx  ndivy\n";
	if (argc>=2) is_o2 = (atoi(argv[1])>0 ? true : false);
	if (argc>=3) ndivx =  atof(argv[2]);
	if (argc>=4) ndivy =  atof(argv[3]);

	///////////////////////////////////////////////////////////////////////////////////////// Mesh /////

	// Block # 0
	Mesh::Block b0;
	b0.SetTag    (-1); // tag to be replicated to all generated elements inside this block
	b0.SetCoords (false, 4,               // Is3D, NNodes
	              0.0, 12.0, 12.0, 0.0,   // x coordinates
	              0.0,  0.0,  4.0, 4.0);  // y coordinates
	b0.SetNx     (ndivx);                 // x weights and num of divisions along x
	b0.SetNy     (ndivy);                 // y weights and num of divisions along y
	b0.SetETags  (4, -10, -10, -11,  0);  // edge tags

	// Block # 1
	Mesh::Block b1;
	b1.SetTag    (-2); // tag to be replicated to all generated elements inside this block
	b1.SetCoords (false, 4,               // Is3D, NNodes
	              0.0, 12.0, 12.0, 0.0,   // x coordinates
	              4.0,  4.0,  8.0, 8.0);  // y coordinates
	b1.SetNx     (ndivx);                 // x weights and num of divisions along x
	b1.SetNy     (ndivy);                 // y weights and num of divisions along y
	b1.SetETags  (4, -10, -10,   0,   0); // edge tags

	// Block # 2
	Mesh::Block b2;
	b2.SetTag    (-3); // tag to be replicated to all generated elements inside this block
	b2.SetCoords (false, 4,               // Is3D, NNodes
	              0.0, 12.0, 12.0,  0.0,  // x coordinates
	              8.0,  8.0, 12.0, 12.0); // y coordinates
	b2.SetNx     (ndivx);                 // x weights and num of divisions along x
	b2.SetNy     (ndivy);                 // y weights and num of divisions along y
	b2.SetETags  (4, -10, -10,   0, -12); // edge tags

	// Blocks
	Array<Mesh::Block*> blocks;
	blocks.Push (&b0);
	blocks.Push (&b1);
	blocks.Push (&b2);

	// Generate
	Mesh::Structured mesh(/*Is3D*/false);
	if (is_o2) mesh.SetO2();                // Non-linear elements
	clock_t start = std::clock();           // Initial time
	size_t  ne    = mesh.Generate (blocks); // Discretize domain
	clock_t total = std::clock() - start;   // Time elapsed
	if (is_o2) cout << "\nNum of quadrangles (o2) = " << ne << endl;
	else       cout << "\nNumber of quadrangles   = " << ne << endl;
	cout << "Time elapsed (mesh)     = "<<static_cast<double>(total)/CLOCKS_PER_SEC<<" seconds\n";

	////////////////////////////////////////////////////////////////////////////////////////// FEM /////

	// Geometry
	FEM::Geom g(2); // 2D

	// Edges brys
	FEM::EBrys_T ebrys;
	ebrys.Push (make_tuple(-10, "ux", 0.0));
	ebrys.Push (make_tuple(-11, "uy", 0.0));

	// Elements attributes
	String prms; prms.Printf("E=%f nu=%f",E,nu);
	FEM::EAtts_T eatts;
	if (is_o2)
	{
		eatts.Push (make_tuple(-1, "Quad8PStrain", "LinElastic", prms.CStr(), "ZERO", "gam=20", true ));
		eatts.Push (make_tuple(-2, "Quad8PStrain", "LinElastic", prms.CStr(), "ZERO", "gam=20", false));
		eatts.Push (make_tuple(-3, "Quad8PStrain", "LinElastic", prms.CStr(), "ZERO", "gam=20", false));
	}
	else
	{
		eatts.Push (make_tuple(-1, "Quad4PStrain", "LinElastic", prms.CStr(), "ZERO", "gam=20", true ));
		eatts.Push (make_tuple(-2, "Quad4PStrain", "LinElastic", prms.CStr(), "ZERO", "gam=20", false));
		eatts.Push (make_tuple(-3, "Quad4PStrain", "LinElastic", prms.CStr(), "ZERO", "gam=20", false));
	}

	// Set geometry: nodes, elements, attributes, and boundaries
	FEM::SetNodesElems (&mesh, &eatts, &g);
	FEM::SetBrys       (&mesh, NULL, &ebrys, NULL, &g);

	// Solver
	FEM::Solver * sol = FEM::AllocSolver("ForwardEuler");
	sol->SetGeom (&g);

	// Open collection for output
	Output out; out.OpenCollection ("tembank01");

	// Stage # -1 --------------------------------------------------------------
	g.ApplyBodyForces    ();
	sol->SolveWithInfo   (/*NDiv*/1, /*DTime*/0.0, /*iStage*/-1, "  Initial stress state due to self weight (zero displacements)\n");
	g.ClearDisplacements ();
	out.VTU              (&g, sol->Time());

	// Stage # 0 ---------------------------------------------------------------
	g.Activate         (/*Tag*/-2);
	sol->SolveWithInfo (1, 0.0, 0, "  Construction of first layer\n");
	out.VTU            (&g, sol->Time());

	// Stage # 1 ---------------------------------------------------------------
	g.Activate         (/*Tag*/-3);
	sol->SolveWithInfo (1, 0.0, 0, "  Construction of second layer\n");
	out.VTU            (&g, sol->Time());

	// Close collection
	out.CloseCollection();

	// Delete solver
	delete sol;

	//////////////////////////////////////////////////////////////////////////////////////// Check /////

	return 1;
}
catch (Exception * e)
{
	e->Cout();
	if (e->IsFatal()) {delete e; exit(1);}
	delete e;
}
catch (char const * m)
{
	std::cout << "Fatal: " << m << std::endl;
	exit (1);
}
catch (...)
{
	std::cout << "Some exception (...) ocurred\n";
}
