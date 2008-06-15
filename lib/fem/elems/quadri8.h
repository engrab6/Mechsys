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

#ifndef MECHSYS_FEM_QUADRI8_H
#define MECHSYS_FEM_QUADRI8_H

// MechSys
#include "fem/element.h"
#include "linalg/vector.h"
#include "linalg/matrix.h"
#include "linalg/lawrap.h"

namespace FEM
{

// Quadri8 Constants
const int QUADRI8_NNODES      = 8;
const int QUADRI8_NINTPTS     = 4;
const int QUADRI8_NFACENODES  = 3;
const int QUADRI8_NFACEINTPTS = 2;
const Element::IntegPoint QUADRI8_INTPTS[]=
{{ -0.577350269189625764509149, -0.577350269189625764509149, 0.0, 1.0 },
 {  0.577350269189625764509149, -0.577350269189625764509149, 0.0, 1.0 },
 {  0.577350269189625764509149,  0.577350269189625764509149, 0.0, 1.0 },
 { -0.577350269189625764509149,  0.577350269189625764509149, 0.0, 1.0 }};
const Element::IntegPoint QUADRI8_FACEINTPTS[]=
{{ -0.577350269189625764509149, 0.0, 0.0, 1.0 },
 {  0.577350269189625764509149, 0.0, 0.0, 1.0 }};

class Quadri8: public virtual Element
{
public:
	// Constructor
	Quadri8();

	// Destructor
	virtual ~Quadri8() {}

	// Derived methods
	int  VTKCellType    () const { return 23; } // VTK_QUADRATIC_QUAD
	void Shape          (double r, double s, double t, LinAlg::Vector<double> & Shape)  const;
	void Derivs         (double r, double s, double t, LinAlg::Matrix<double> & Derivs) const;
	void FaceShape      (double r, double s, LinAlg::Vector<double> & FaceShape)  const;
	void FaceDerivs     (double r, double s, LinAlg::Matrix<double> & FaceDerivs) const;
	void Dist2FaceNodes (Array<Node*> const & FaceConnects, double const FaceValue, LinAlg::Vector<double> & NodalValues) const;

}; // class Quadri8


/////////////////////////////////////////////////////////////////////////////////////////// Implementation /////


inline Quadri8::Quadri8()
{
	// Setup nodes number
	_n_dim          = 2;
	_n_nodes        = QUADRI8_NNODES;
	_n_int_pts      = QUADRI8_NINTPTS;
	_n_face_nodes   = QUADRI8_NFACENODES;
	_n_face_int_pts = QUADRI8_NFACEINTPTS;

	// Allocate nodes (connectivity)
	_connects.Resize(_n_nodes);

	// Setup pointer to the array of Integration Points
	_a_int_pts = QUADRI8_INTPTS;
}

inline void Quadri8::Shape(double r, double s, double t, LinAlg::Vector<double> & Shape) const
{

	/*      3           6            2
	 *        @---------@----------@
	 *        |               (1,1)|
	 *        |       s ^          |
	 *        |         |          |
	 *        |         |          |
	 *      7 @         +----> r   @ 5
	 *        |       (0,0)        |
	 *        |                    |
	 *        |                    |
	 *        |(-1,-1)             |
	 *        @---------@----------@
	 *      0           4            1
	 */
	Shape.Resize (QUADRI8_NNODEr);

	double rp1=r+1.0; double rm1=r-1.0;
	double sp1=s+1.0; double sm1=s-1.0;

	Shape(0) = 0.25*rm1*sm1*(rm1+sm1-3.0);
	Shape(1) = 0.25*rp1*sm1*(rp1+sm1-3.0);
	Shape(2) = 0.25*rp1*sp1*(rp1+sp1-3.0);
	Shape(3) = 0.25*rm1*sp1*(rm1+sp1-3.0);
	Shape(4) = 0.50*sm1*(1.0-r*r);
	Shape(5) = 0.50*rp1*(1.0-s*s);
	Shape(6) = 0.50*sp1*(1.0-r*r);
	Shape(7) = 0.50*rm1*(1.0-s*s);
}

inline void Quadri8::Derivs(double r, double s, double t, LinAlg::Matrix<double> & Derivs) const
{
	/*           _     _ T
	 *          |  dNi  |
	 * Derivs = |  ---  |   , where cj = r, s
	 *          |_ dcj _|
	 *
	 * Derivs(j,i), j=>local coordinate and i=>shape function
	 */
	Derivs.Resize (2, QUADRI8_NNODES);

	double rp1=r+1.0; double rm1=r-1.0;
	double sp1=s+1.0; double sm1=s-1.0;

	Derivs(0,0) = - 0.25 * sm1 * (rm1 + rm1 + sm1 - 3.0);
	Derivs(0,1) =   0.25 * sm1 * (rp1 + rp1 + sm1 - 3.0);
	Derivs(0,2) =   0.25 * sp1 * (rp1 + rp1 + sp1 - 3.0);
	Derivs(0,3) = - 0.25 * sp1 * (rm1 + rm1 + sp1 - 3.0);
	Derivs(0,4) = - r * sm1;
	Derivs(0,5) =   0.50 * (1.0 - s * s);
	Derivs(0,6) = - r * sp1;
	Derivs(0,7) = - 0.5 * (1.0 - s * s);

	Derivs(1,0) = - 0.25 * rm1 * (sm1 + rm1 + sm1 - 3.0);
	Derivs(1,1) = - 0.25 * rp1 * (sm1 + rp1 + sm1 - 3.0);
	Derivs(1,2) =   0.25 * rp1 * (sp1 + rp1 + sp1 - 3.0);
	Derivs(1,3) =   0.25 * rm1 * (sp1 + rm1 + sp1 - 3.0);
	Derivs(1,4) = - 0.50 * (1.0 - r * r);
	Derivs(1,5) = - s * rp1;
	Derivs(1,6) =   0.50 * (1.0 - r * r);
	Derivs(1,7) = - s * rm1;
}

inline void Quadri8::FaceShape(double r, double s, LinAlg::Vector<double> & FaceShape) const
{
	/*  
	 *  
	 *       @-----------@-----------@-> r
	 *       0           2           1
	 */
	FaceShape.Resize(QUADRI8_NFACENODES);
	FaceShape(0) = 0.5 * (r*r-r);
	FaceShape(1) = 0.5 * (r*r+r);
	FaceShape(2) = 1.0 -  r*r;
}

inline void Quadri8::FaceDerivs(double r, double s, LinAlg::Matrix<double> & FaceDerivs) const
{
	/*           _     _ T
	 *          |  dNi  |
	 * Derivs = |  ---  |   , where cj = r, s
	 *          |_ dcj _|
	 *
	 * Derivs(j,i), j=>local coordinate and i=>shape function
	 */
	FaceDerivs.Resize(1,QUADRI8_NFACENODES);
	FaceDerivs(0,0) =  r  - 0.5;
	FaceDerivs(0,1) =  r  + 0.5;
	FaceDerivs(0,2) = -2.0* r;
}

inline void Quadri8::Dist2FaceNodes(Array<Node*> const & FaceConnects, double const FaceValue, LinAlg::Vector<double> & NodalValues) const
{
	// Dimensioning NodalValues
	NodalValues.Resize(_n_face_nodes);
	NodalValues.SetValues(0.0);
	LinAlg::Matrix<double> J;                         // Jacobian matrix. size = 1 x 2
	LinAlg::Vector<double> face_shape(_n_face_nodes); // Shape functions of a face. size = _n_face_nodes
	// Integration along the face
	for (int i=0; i<_n_face_int_pts; i++)
	{
		double r = QUADRI8_FACEINTPTS[i].r;
		double w = QUADRI8_FACEINTPTS[i].w;
		FaceShape    (r, 0.0, face_shape);
		FaceJacobian (FaceConnects, r, J);
		NodalValues += FaceValue*face_shape*det(J)*w;
	}
}

}; // namespace FEM

#endif // MECHSYS_FEM_QUADRI8_H
