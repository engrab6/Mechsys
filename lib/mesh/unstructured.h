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

#ifndef MECHSYS_MESH_UNSTRUCTURED_H
#define MECHSYS_MESH_UNSTRUCTURED_H

/* LOCAL indexes of Vertices, Edges, and Faces

  2D:
             Nodes                 Edges

   y           2
   |           @                     @
   +--x       / \                   / \
           5 /   \ 4               /   \
            @     @             2 /     \ 1
           /       \             /       \
          /         \           /         \
         @-----@-----@         @-----------@
        0      3      1              0
*/

// STL
#include <iostream> // for cout, endl, ostream
#include <sstream>  // for ostringstream
#include <fstream>  // for ofstream
#include <cfloat>   // for DBL_EPSILON
#include <ctime>    // for clock
#include <map>

// Jonathan R Shewchuk' Triangle
extern "C"
{
    #define REAL double
    #define ANSI_DECLARATORS
    #define VOID int
      #include "triangle.h"
    #undef REAL
    #undef ANSI_DECLARATORS
    #undef VOID
}

// Hang Si' Tetgen
#define TETLIBRARY
#include "tetgen.h"
#undef TETLIBRARY

// MechSys
#include "util/array.h"
#include "util/fatal.h"
#include "mesh/mesh.h"
#include "draw.h"

namespace Mesh
{


/////////////////////////////////////////////////////////////////////////////////////////// TriIO /////


/** JRS' Triangle Input/Output structure. */
typedef triangulateio TriIO;

/** HSI' Tetgen Input/Output structure. */
typedef tetgenio TetIO;

inline void TriAllocate (int NPoints, int NSegments, int NRegions, int NHoles, TriIO & Tio)
{
    // check
    if (NPoints<3)   throw new Fatal("Mesh::TriAllocate: At least 3 points are required. (%d is invalid)",NPoints);
    if (NSegments<3) throw new Fatal("Mesh::TriAllocate: At least 3 segments are required. (%d is invalid)",NSegments);

    // points
    Tio.pointlist       = (double*)malloc(NPoints*2*sizeof(double));
    Tio.numberofpoints  = NPoints;
    Tio.pointmarkerlist = (int*)malloc(NPoints*sizeof(int));

    // segments
    Tio.segmentlist       = (int*)malloc(NSegments*2*sizeof(int));
    Tio.segmentmarkerlist = (int*)malloc(NSegments * sizeof(int));
    Tio.numberofsegments  = NSegments;
    for (int i=0; i<NSegments; ++i) Tio.segmentmarkerlist[i]=0;

    // regions
    if (NRegions>0)
    {
        Tio.regionlist      = (double*)malloc(NRegions*4*sizeof(double));
        Tio.numberofregions = NRegions;
    }

    // holes
    if (NHoles>0)
    {
        Tio.holelist      = (double*)malloc(NHoles*2*sizeof(double));
        Tio.numberofholes = NHoles;
    }
}

inline void TriSetAllToNull (TriIO & Tio)
{
    // points
    Tio.pointlist               = NULL;
    Tio.pointattributelist      = NULL;
    Tio.pointmarkerlist         = NULL;
    Tio.numberofpoints          = 0;
    Tio.numberofpointattributes = 0;

    // triangles
    Tio.trianglelist               = NULL;
    Tio.triangleattributelist      = NULL;
    Tio.trianglearealist           = NULL;
    Tio.neighborlist               = NULL;
    Tio.numberoftriangles          = 0;
    Tio.numberofcorners            = 0;
    Tio.numberoftriangleattributes = 0;
    Tio.triedgemarks               = NULL;

    // segments
    Tio.segmentlist       = NULL;
    Tio.segmentmarkerlist = NULL;
    Tio.numberofsegments  = 0;

    // holes
    Tio.holelist      = NULL;
    Tio.numberofholes = 0;

    // regions
    Tio.regionlist      = NULL;
    Tio.numberofregions = 0;

    // edges
    Tio.edgelist       = NULL;
    Tio.edgemarkerlist = NULL;
    Tio.normlist       = NULL;
    Tio.numberofedges  = 0;
}

inline void TriDeallocateAll (TriIO & Tio)
{
    // Points
    if (Tio.pointlist          != NULL) free(Tio.pointlist);
    if (Tio.pointattributelist != NULL) free(Tio.pointattributelist);
    if (Tio.pointmarkerlist    != NULL) free(Tio.pointmarkerlist);

    // Triangles
    if (Tio.trianglelist          != NULL) free(Tio.trianglelist);
    if (Tio.triangleattributelist != NULL) free(Tio.triangleattributelist);
    if (Tio.trianglearealist      != NULL) free(Tio.trianglearealist);
    if (Tio.neighborlist          != NULL) free(Tio.neighborlist);
    if (Tio.triedgemarks          != NULL) free(Tio.triedgemarks);

    // Segments
    if (Tio.segmentlist       != NULL) free(Tio.segmentlist);
    if (Tio.segmentmarkerlist != NULL) free(Tio.segmentmarkerlist);

    // Holes
    if (Tio.holelist != NULL) free(Tio.holelist);

    // Regions
    if (Tio.regionlist != NULL) free(Tio.regionlist);

    // Edges
    if (Tio.edgelist       != NULL) free(Tio.edgelist);
    if (Tio.edgemarkerlist != NULL) free(Tio.edgemarkerlist);
    if (Tio.normlist       != NULL) free(Tio.normlist);

    // Clear all
    TriSetAllToNull (Tio);
}


/////////////////////////////////////////////////////////////////////////////////////////// Unstructured /////


class Unstructured : public virtual Mesh::Generic
{
public:
    // Constants
    static size_t FEM2TriPoint[]; ///< Map MechSys/FEM nodes to JRS-Triangle points
    static size_t FEM2TriEdge []; ///< Map MechSys/FEM nodes to JRS-Triangle edges
    static size_t FEM2TetPoint[]; ///< Map MechSys/FEM nodes to HSI-Tetgen points
    static size_t FEM2TetFace []; ///< Map MechSys/FEM nodes to HSI-Tetgen edges

    // Constructor
    Unstructured (int NDim) : Mesh::Generic(NDim) { TriSetAllToNull(Tin); Pin.deinitialize(); }

    // Destructor
    ~Unstructured () { TriDeallocateAll(Tin); }

    /** 2D: Set Planar Straight Line Graph (PSLG)
     *  3D: Set Piecewise Linear Complex (PLC)
     *
     * see tst/mesh01 for example
     *
     *  Note:
     *     After NHoles, all data must be (double)   */
    void Set    (size_t NPoints, size_t NSegmentsOrFacets, size_t NRegions, size_t NHoles, ...);
    void SetSeg (size_t iSegment, int ETag, int L, int R);
    void SetFac (size_t iFacet,   int FTag, size_t NPolygons, ...);

    // Methods
    void Generate (bool O2=false, double GlobalMaxArea=-1, bool WithInfo=true); ///< Generate
    void WritePLY (char const * FileKey, bool Blender=true);                    ///< (.ply)
    void GenBox   (bool O2=false, double MaxVolume=-1.0,
                   double Lx=1.0, double Ly=1.0, double Lz=1.0);                ///< Generate a cube with dimensions Lx,Ly,Lz and with tags on faces

    // Data
    TriIO Tin; ///< Triangle structure: input PSLG
    TetIO Pin; ///< Tetgen structure: input PLC
};

size_t Unstructured::FEM2TriPoint[] = {0,1,2,5,3,4};
size_t Unstructured::FEM2TriEdge [] = {0,1,2};
size_t Unstructured::FEM2TetPoint[] = {0,1,2,3,4,5,6,7,8,9};
size_t Unstructured::FEM2TetFace [] = {3,1,0,2};


/////////////////////////////////////////////////////////////////////////////////////////// PLC: Implementation /////


inline void Unstructured::Set (size_t NPoints, size_t NSegmentsOrFacets, size_t NRegions, size_t NHoles, ...)
{
    if (NDim==2)
    {
        // erase previous PSLG
        TriDeallocateAll (Tin);

        // allocate PSLG
        TriAllocate (NPoints, NSegmentsOrFacets, NRegions, NHoles, Tin);

        // read points
        va_list   arg_list;
        va_start (arg_list, NHoles);
        for (size_t i=0; i<NPoints; ++i)
        {
            int id  = static_cast<int>(va_arg(arg_list,double)); // point id
            int tag = static_cast<int>(va_arg(arg_list,double)); // point tag
            if (id!=(int)i) throw new Fatal("Unstructured::Set: Points must be numbered from 0 to %d in ascending order. Problem with point %d",NPoints-1,id);
            Tin.pointlist[i*2  ]   = va_arg(arg_list,double);
            Tin.pointlist[i*2+1]   = va_arg(arg_list,double);
            Tin.pointmarkerlist[i] = tag;
        }

        // set regions
        for (size_t i=0; i<NRegions; ++i)
        {
            int tag = static_cast<int>(va_arg(arg_list,double)); // region tag
            Tin.regionlist[i*4  ] = va_arg(arg_list,double);
            Tin.regionlist[i*4+1] = va_arg(arg_list,double);
            Tin.regionlist[i*4+2] = tag;
            Tin.regionlist[i*4+3] = va_arg(arg_list,double); // MaxArea;
        }

        // set holes
        for (size_t i=0; i<NHoles; ++i)
        {
            Tin.holelist[i*2  ] = va_arg(arg_list,double);
            Tin.holelist[i*2+1] = va_arg(arg_list,double);
        }
        va_end (arg_list);
    }
    else if (NDim==3)
    {
        // erase previous PLC
        Pin.deinitialize ();

        // allocate PLC
        Pin.initialize ();
        
        // points
        Pin.firstnumber     = 0;
        Pin.numberofpoints  = NPoints;
        Pin.pointlist       = new double [NPoints*3];
        Pin.pointmarkerlist = new int [NPoints];

        // facets
        Pin.numberoffacets  = NSegmentsOrFacets;
        Pin.facetlist       = new TetIO::facet [NSegmentsOrFacets];
        Pin.facetmarkerlist = new int [NSegmentsOrFacets];

        // regions
        Pin.numberofregions = NRegions;
        Pin.regionlist      = new double [NRegions*5];

        // holes
        Pin.numberofholes = NHoles;
        Pin.holelist      = new double [NHoles*3];

        // read points
        va_list   arg_list;
        va_start (arg_list, NHoles);
        for (size_t i=0; i<NPoints; ++i)
        {
            int id  = static_cast<int>(va_arg(arg_list,double)); // point id
            int tag = static_cast<int>(va_arg(arg_list,double)); // point tag
            if (id!=(int)i) throw new Fatal("Unstructured::Set: Points must be numbered from 0 to %d in ascending order. Problem with point %d",NPoints-1,id);
            Pin.pointlist[i*3  ]   = va_arg(arg_list,double);
            Pin.pointlist[i*3+1]   = va_arg(arg_list,double);
            Pin.pointlist[i*3+2]   = va_arg(arg_list,double);
            Pin.pointmarkerlist[i] = tag;
        }

        // set regions
        for (size_t i=0; i<NRegions; ++i)
        {
            int tag = static_cast<int>(va_arg(arg_list,double)); // region tag
            Pin.regionlist[i*4  ] = va_arg(arg_list,double);
            Pin.regionlist[i*4+1] = va_arg(arg_list,double);
            Pin.regionlist[i*4+2] = va_arg(arg_list,double);
            Pin.regionlist[i*4+3] = tag;
            Pin.regionlist[i*4+4] = va_arg(arg_list,double); // MaxVolume;
        }

        // set holes
        for (size_t i=0; i<NHoles; ++i)
        {
            Pin.holelist[i*2  ] = va_arg(arg_list,double);
            Pin.holelist[i*2+1] = va_arg(arg_list,double);
            Pin.holelist[i*2+2] = va_arg(arg_list,double);
        }
        va_end (arg_list);
    }
    else throw new Fatal("Unstructured::Set: NDim must be either 2 or 3. NDim==%d is invalid",NDim);
}

inline void Unstructured::SetSeg (size_t iSeg, int ETag, int L, int R)
{
    Tin.segmentlist[iSeg*2  ]   = L;
    Tin.segmentlist[iSeg*2+1]   = R;
    Tin.segmentmarkerlist[iSeg] = ETag;
}

inline void Unstructured::SetFac (size_t iFacet, int FTag, size_t NPolygons, ...)
{
    Pin.facetmarkerlist[iFacet] = FTag;
    TetIO::facet * f    = &Pin.facetlist[iFacet];
    f->numberofpolygons = NPolygons;
    f->polygonlist      = new TetIO::polygon [NPolygons];
    f->numberofholes    = 0;
    f->holelist         = NULL;

    // read polygons
    va_list   arg_list;
    va_start (arg_list, NPolygons);
    for (size_t i=0; i<NPolygons; ++i)
    {
        TetIO::polygon * p  = &f->polygonlist[i];
        int npoints         = static_cast<int>(va_arg(arg_list,double));
        p->numberofvertices = npoints;
        p->vertexlist       = new int [npoints];
        for (int j=0; j<npoints; ++j)
        {
            int id = static_cast<int>(va_arg(arg_list,double));
            p->vertexlist[j] = id;
        }
    }
    va_end (arg_list);
}

inline void Unstructured::Generate (bool O2, double GlobalMaxArea, bool WithInfo)
{
    // info
    double start = std::clock();

    // parameters
    double min_angle = -1;
    String prms("QpzA"); // Q=quiet, p=poly, q=quality, z=zero
    if (GlobalMaxArea>0) prms.Printf("%sa%f", prms.CStr(), GlobalMaxArea);
    if (min_angle>0)     prms.Printf("%sq%f", prms.CStr(), min_angle);
    else                 prms.Printf("%sq",   prms.CStr());
    if (O2)              prms.Printf("%so2",  prms.CStr());
    prms.Printf("%sa", prms.CStr());

    if (NDim==2)
    {
        // generate
        TriIO tou;
        TriSetAllToNull (tou);
        triangulate (prms.CStr(), &Tin, &tou, NULL);

        // verts
        Verts.Resize (tou.numberofpoints);
        for (size_t i=0; i<Verts.Size(); ++i)
        {
            Verts[i]      = new Vertex;
            Verts[i]->ID  = i;
            Verts[i]->Tag = 0;
            Verts[i]->C   = tou.pointlist[i*2], tou.pointlist[i*2+1], 0.0;

            /* tou.pointmarkerlist[ipoint] will be equal to:
             * == edgeTag (<0) => on edge with tag <<<<<<<<<<<<<<<<<< REMOVED
             * == 0            => internal vertex (not on boundary)
             * == 1            => on boundary                   */
            int mark = tou.pointmarkerlist[i];
            if (mark<0)
            {
                Verts[i]->Tag = mark;
                TgdVerts.Push (Verts[i]);
            }
        }

        // cells
        Cells.Resize (tou.numberoftriangles);
        for (size_t i=0; i<Cells.Size(); ++i)
        {
            Cells[i]      = new Cell;
            Cells[i]->ID  = i;
            Cells[i]->Tag = tou.triangleattributelist[i*tou.numberoftriangleattributes];
            Cells[i]->V.Resize (tou.numberofcorners);
            for (size_t j=0; j<Cells[i]->V.Size(); ++j)
            {
                Cells[i]->V[j] = Verts[tou.trianglelist[i*tou.numberofcorners+FEM2TriPoint[j]]];
            }
            bool has_bry_tag = false;
            for (size_t j=0; j<3; ++j)
            {
                int edge_tag = tou.triedgemarks[i*3+FEM2TriEdge[j]];
                if (edge_tag<0)
                {
                    Cells[i]->BryTags[j] = edge_tag;
                    has_bry_tag          = true;
                }
            }
            if (has_bry_tag) TgdCells.Push (Cells[i]);
        }

        // clean up
        /* After triangulate (with -p switch), tou.regionlist gets the content of Tin.regionlist and
         * tou.holelist gets the content of Tin.holelist. Thus, these output variables must be set
         * to NULL in order to tell TriDeallocateAll to ignore them and do not double-free memory. */
        tou.regionlist      = NULL;
        tou.numberofregions = 0;
        tou.holelist        = NULL;
        tou.numberofholes   = 0;
        TriDeallocateAll (tou);
    }
    else
    {
        // generate
        prms.append("f");
        char sw[prms.size()+1];
        strcpy (sw, prms.CStr());
        TetIO pou;
        tetrahedralize (sw, &Pin, &pou);

        // verts
        Verts.Resize (pou.numberofpoints);
        for (size_t i=0; i<Verts.Size(); ++i)
        {
            Verts[i]      = new Vertex;
            Verts[i]->ID  = i;
            Verts[i]->Tag = 0;
            Verts[i]->C   = pou.pointlist[i*3], pou.pointlist[i*3+1], pou.pointlist[i*3+2];

            /* pou.pointmarkerlist[ipoint] will be equal to:
             * == faceTag (<0) => on face with tag <<<<<<<<<<<<<<<<<< REMOVED
             * == 0            => internal vertex (not on boundary)
             * == 1            => on boundary                   */
            int mark = pou.pointmarkerlist[i];
            if (mark<0)
            {
                Verts[i]->Tag = mark;
                TgdVerts.Push (Verts[i]);
            }
        }

        // cells
        Cells.Resize (pou.numberoftetrahedra);
        for (size_t i=0; i<Cells.Size(); ++i)
        {
            Cells[i]      = new Cell;
            Cells[i]->ID  = i;
            Cells[i]->Tag = pou.tetrahedronattributelist[i*pou.numberoftetrahedronattributes];
            Cells[i]->V.Resize (pou.numberofcorners);
            for (size_t j=0; j<Cells[i]->V.Size(); ++j)
            {
                Cells[i]->V[j] = Verts[pou.tetrahedronlist[i*pou.numberofcorners+FEM2TetPoint[j]]];
            }
        }

        // face tags
        for (std::map<int,tetgenio::facemarkers>::const_iterator p=pou.tetfacemarkers.begin(); p!=pou.tetfacemarkers.end(); ++p)
        {
            int  icell       = p->first;
            bool has_bry_tag = false;
            for (size_t j=0; j<4; ++j)
            {
                int face_tag = p->second.m[FEM2TetFace[j]];
                //std::cout << icell << " " << j << " " << face_tag << "\n";
                if (face_tag<0)
                {
                    Cells[icell]->BryTags[j] = face_tag;
                    has_bry_tag              = true;
                }
            }
            if (has_bry_tag) TgdCells.Push (Cells[icell]);
        }
    }

    // info
    if (WithInfo)
    {
        double total = std::clock() - start;
        if (NDim==2) std::cout << "[1;33m\n--- Unstructured Mesh Generation --- (2D) ----------------------[0m\n";
        else         std::cout << "[1;33m\n--- Unstructured Mesh Generation --- (3D) ----------------------[0m\n";
        if (O2) std::cout << "[1;36m    Time elapsed (o2)     = [1;31m" <<static_cast<double>(total)/CLOCKS_PER_SEC<<" seconds[0m\n";
        else    std::cout << "[1;36m    Time elapsed          = [1;31m" <<static_cast<double>(total)/CLOCKS_PER_SEC<<" seconds[0m\n";
        if (NDim==2) std::cout <<         "    JRS' triangle command = " << prms                    << std::endl;
        else         std::cout <<         "    HSI's tetgen command  = " << prms                    << std::endl;
        std::cout << "[1;32m    Number of cells       = " << Cells.Size() << "[0m" << std::endl;
        std::cout << "[1;32m    Number of vertices    = " << Verts.Size() << "[0m" << std::endl;
    }
}

inline void Unstructured::WritePLY (char const * FileKey, bool Blender)
{
    // output string
    String fn(FileKey); fn.append(".ply");
    std::ostringstream oss;

    if (Blender)
    {
        // header
        oss << "import Blender\n";
        oss << "import bpy\n";

        // scene, mesh, and object
        oss << "scn = bpy.data.scenes.active\n";
        oss << "msh = bpy.data.meshes.new('unstruct_poly')\n";
        oss << "obj = scn.objects.new(msh,'unstruct_poly')\n";

        if (NDim==2)
        {
            // points
            oss << "pts = [";
            for (int i=0; i<Tin.numberofpoints; ++i)
            {
                oss << "[" << Tin.pointlist[i*2];
                oss << "," << Tin.pointlist[i*2+1] << ", 0.0]";
                if (i==Tin.numberofpoints-1) oss << "]\n";
                else                         oss << ",\n       ";
            }
            oss << "\n";

            // edges
            oss << "edg = [";
            for (int i=0; i<Tin.numberofsegments; ++i)
            {
                oss << "[" << Tin.segmentlist[i*2] << "," << Tin.segmentlist[i*2+1] << "]";
                if (i==Tin.numberofsegments-1) oss << "]\n";
                else                           oss << ",\n       ";
            }
            oss << "\n";
        }
        else
        {
            // points
            oss << "pts = [";
            for (int i=0; i<Pin.numberofpoints; ++i)
            {
                oss << "[" << Pin.pointlist[i*3];
                oss << "," << Pin.pointlist[i*3+1];
                oss << "," << Pin.pointlist[i*3+2] << "]";
                if (i==Pin.numberofpoints-1) oss << "]\n";
                else                         oss << ",\n       ";
            }
            oss << "\n";

            // edges
            oss << "edg = [";
            for (int i=0; i<Pin.numberoffacets; ++i)
            {
                TetIO::facet * f = &Pin.facetlist[i];
                for (int j=0; j<f->numberofpolygons; ++j)
                {
                    TetIO::polygon * p  = &f->polygonlist[j];
                    for (int k=1; k<p->numberofvertices; ++k)
                    {
                        oss << "[" << p->vertexlist[k-1] << "," << p->vertexlist[k] << "]";
                        if (k==p->numberofvertices-1) oss << ",[" << p->vertexlist[k] << "," << p->vertexlist[0] << "]";
                        if ((i==Pin.numberoffacets-1) && (j==f->numberofpolygons-1) && (k==p->numberofvertices-1)) oss << "]\n";
                        else oss << ",\n       ";
                    }
                }
            }
            oss << "\n";
        }

        // extend mesh
        oss << "msh.verts.extend(pts)\n";
        oss << "msh.edges.extend(edg)\n";
    }
    else // matplotlib
    {
        if (NDim==3) throw new Fatal("Unstructured::WritePLY: Method not available for 3D and MatPlotLib");

        // header
        MPL::Header (oss);

        // vertices and commands
        oss << "# vertices and commands\n";
        oss << "dat = []\n";
        for (int i=0; i<Tin.numberofsegments; ++i)
        {
            int I = Tin.segmentlist[i*2];
            int J = Tin.segmentlist[i*2+1];
            oss << "dat.append((PH.MOVETO, (" << Tin.pointlist[I*2] << "," << Tin.pointlist[I*2+1] << ")))\n";
            oss << "dat.append((PH.LINETO, (" << Tin.pointlist[J*2] << "," << Tin.pointlist[J*2+1] << ")))\n";
        }
        oss << "\n";

        // draw edges
        MPL::AddPatch (oss);

        // draw tags
        oss << "# draw tags\n";
        for (int i=0; i<Tin.numberofpoints; ++i)
        {
            int pt_tag = Tin.pointmarkerlist[i];
            if (pt_tag<0) oss << "ax.text(" << Tin.pointlist[i*2] << "," << Tin.pointlist[i*2+1] << ", " << pt_tag << ", ha='center', va='center', fontsize=14, backgroundcolor=lyellow)\n";
        }
        for (int i=0; i<Tin.numberofsegments; ++i)
        {
            int edge_tag = Tin.segmentmarkerlist[i];
            if (edge_tag<0)
            {
                int    I  = Tin.segmentlist[i*2];
                int    J  = Tin.segmentlist[i*2+1];
                double x0 = Tin.pointlist[I*2];
                double y0 = Tin.pointlist[I*2+1];
                double x1 = Tin.pointlist[J*2];
                double y1 = Tin.pointlist[J*2+1];
                double xm = (x0+x1)/2.0;
                double ym = (y0+y1)/2.0;
                oss << "ax.text(" << xm << "," << ym << ", " << edge_tag << ", ha='center', va='center', fontsize=14, backgroundcolor=pink)\n";
            }
        }
        oss << "\n";

        // show
        oss << "# show\n";
        oss << "axis ('scaled')\n";
        oss << "show ()\n";
    }

    // create file
    std::ofstream of(fn.CStr(), std::ios::out);
    of << oss.str();
    of.close();
}

inline void Unstructured::GenBox (bool O2, double MaxVolume, double Lx, double Ly, double Lz)
{
    Set (8, 6, 1, 0,                 // nverts, nfaces, nregs, nholes
         0., -1.,   0.0,  0.0,  0.0, // id, vtag, x, y, z
         1., -2.,    Lx,  0.0,  0.0,
         2., -3.,    Lx,   Ly,  0.0,
         3., -4.,   0.0,   Ly,  0.0,
         4., -5.,   0.0,  0.0,   Lz,
         5., -6.,    Lx,  0.0,   Lz,
         6., -7.,    Lx,   Ly,   Lz,
         7., -8.,   0.0,   Ly,   Lz,
             -1.,  Lx/2., Ly/2., Lz/2., MaxVolume); // tag, reg_x, reg_y, reg_z, max_vol

    SetFac (0, -1, 1,  4., 0.,3.,7.,4.); // id, ftag, npolys, nverts, v0,v1,v2,v3
    SetFac (1, -2, 1,  4., 1.,2.,6.,5.);
    SetFac (2, -3, 1,  4., 0.,1.,5.,4.);
    SetFac (3, -4, 1,  4., 2.,3.,7.,6.);
    SetFac (4, -5, 1,  4., 0.,1.,2.,3.);
    SetFac (5, -6, 1,  4., 4.,5.,6.,7.);

    Generate (O2);
}

}; // namespace Mesh

#endif // MECHSYS_MESH_UNSTRUCTURED_H
