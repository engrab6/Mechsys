########################################################################
# MechSys - Open Library for Mechanical Systems                        #
# Copyright (C) 2005 Dorival M. Pedroso, Raul D. D. Farfan             #
#                                                                      #
# This program is free software: you can redistribute it and/or modify #
# it under the terms of the GNU General Public License as published by #
# the Free Software Foundation, either version 3 of the License, or    #
# any later version.                                                   #
#                                                                      #
# This program is distributed in the hope that it will be useful,      #
# but WITHOUT ANY WARRANTY; without even the implied warranty of       #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         #
# GNU General Public License for more details.                         #
#                                                                      #
# You should have received a copy of the GNU General Public License    #
# along with this program. If not, see <http://www.gnu.org/licenses/>  #
########################################################################

import Blender
from mechsys import *
from Blender import Mesh
from bpy     import data
from Blender.Mathutils import Vector
import msys_dict as di
import math

def gen_pkg(is_ttt):
    edm = Blender.Window.EditMode()
    d   = di.load_dict()
    dom = DEM_TTTDomain() if is_ttt else DEM_Domain()
    if d['dem_pkg']==0: # Spheres
        dom.GenSpheres (-1,d['dem_Lx'],d['dem_Nx'],d['dem_rho'],'Normal',d['dem_seed'],d['dem_prob'])
    elif d['dem_pkg']==1: # Spheres HCP
        dom.GenSpheres (-1,d['dem_Lx'],d['dem_Nx'],d['dem_rho'],'HCP',d['dem_seed'],d['dem_prob'])
    elif d['dem_pkg']==2: # Voronoi
        dom.AddVoroPack (-1,d['dem_R'], d['dem_Lx'], d['dem_Ly'], d['dem_Lz'], d['dem_Nx'], d['dem_Ny'], d['dem_Nz'], d['dem_rho'], True, d['dem_seed'], d['dem_prob'])
    P = []
    dom.GetParticles (P)
    add_particles    (P,d['dem_res'],d['dem_draw_verts'],d['dem_draw_edges'])
    if edm: Blender.Window.EditMode(1)
    Blender.Window.QRedrawAll()

def gen_py(is_ttt):
    # load dictionary
    d = di.load_dict()

    # domain
    dom = DEM_TTTDomain() if is_ttt else DEM_Domain()

    # generate particles
    if d['dem_pkg']==0: # Spheres
        dom.GenSpheres (-1,d['dem_Lx'],d['dem_Nx'],d['dem_rho'],'Normal',d['dem_seed'],d['dem_prob'])
    elif d['dem_pkg']==1: # Spheres HCP
        dom.GenSpheres (-1,d['dem_Lx'],d['dem_Nx'],d['dem_rho'],'HCP',d['dem_seed'],d['dem_prob'])
    elif d['dem_pkg']==2: # Voronoi
        dom.AddVoroPack (-1,d['dem_R'], d['dem_Lx'], d['dem_Ly'], d['dem_Lz'], d['dem_Nx'], d['dem_Ny'], d['dem_Nz'], d['dem_rho'], True, d['dem_seed'], d['dem_prob'])

    dom.GenBoundingBox(-2,d['dem_R'],1.2)

    # stage 1: isotropic compresssion
    p0         = #5.0
    TfIso      = #50.0
    dt         = #0.001
    dtout      = #1.0
    do_render  = #False

    sigf   = (-p0, -p0, -p0)       # final stress state
    peps   = (False, False, False) # prescribed strain rates ?
    depsdt = (0.0, 0.0, 0.0)       # strain rate

    dom.ResetEps  ()
    dom.SetTxTest (sigf, peps, depsdt);
    dom.Solve     (TfIso/2.0, dt, dtout, "ttt_isocomp_a", do_render)

    dom.SetTxTest (sigf, peps, depsdt);
    dom.Solve     (TfIso, dt, dtout, "ttt_isocomp_b", do_render)

    # stage 2: triaxial test

    pf    = d['pf']    #5.0
    qf    = d['qf']    #0.0
    thf   = d['thf']   #30.0
    TfShe = d['TfShe'] #200.0
    pepSx = d['pepSx'] #False
    pepSy = d['pepSy'] #False
    pepSz = d['pepSz'] #True
    ex    = d['ex']    #0.0
    ey    = d['ey']    #0.0
    ez    = d['ez']    #-0.2
    tf    = math.sin(3.0*thf*math.pi/180.0) # final t = sin(3theta)

    sigf   = pqt2L (pf, qf, tf, "cam")
    peps   = (pepSx, pepSy, pepSz)
    depsdt = (ex/(TfShe-TfIso), ey/(TfShe-TfIso), ez/(TfShe-TfIso))

    dom.ResetEps         ()
    dom.SetTxTest        (sigf, peps, depsdt)
    dom.ResetInteractons ()
    dom.Solve            (TfShe, dt, dtout, "ttt_shearing", do_render)

def gen_cpp():
    pass

def add_particles(P,res,draw_verts,draw_edges):
    scn = data.scenes.active
    for p in P:
        # features
        R, V, E, F = p[0], p[1], p[2], p[3]
        objs = []

        # vertices
        if draw_verts:
            for v in V:
                m = Mesh.Primitives.UVsphere(res,res,R*2)
                objs.append (scn.objects.new (m,'vert'))
                objs[-1].setLocation(v[0],v[1],v[2])

        # edges
        if draw_edges:
            for e in E:
                x0  = Vector(V[e[0]])
                x1  = Vector(V[e[1]])
                dx  = x1-x0
                mid = (x0+x1)/2.0
                m   = Mesh.Primitives.Tube(res,R*2,dx.length)
                qua = dx.toTrackQuat ('z','x')
                objs.append (scn.objects.new (m,'edge'))
                objs[-1].setMatrix   (qua.toMatrix())
                objs[-1].setLocation (mid)

        # faces
        for f in F:
            ed0 = Vector(V[f[1]]) - Vector(V[f[0]])
            ed1 = Vector(V[f[2]]) - Vector(V[f[1]])
            n   = ed0.cross(ed1).normalize()

            # list of vertices
            vi = [Vector(V[v])-R*n for v in f]
            vo = [Vector(V[v])+R*n for v in f]

            # list of triangular faces
            fa = []
            nv = len(f) # number of vertices
            nf = nv-2   # number of faces
            for i in range(nf): fa.append ([   0,    i+1,    i+2])
            for i in range(nf): fa.append ([nv+0, nv+i+1, nv+i+2])

            # draw
            m = data.meshes.new ('face')
            m.verts.extend (vi+vo)
            m.faces.extend (fa)
            objs.append (scn.objects.new (m,'face'))

        # join objects
        msh = data.meshes.new ('particle')
        obj = scn.objects.new (msh,'particle')
        obj.join (objs)
        obj.getData(mesh=True).remDoubles (0.001)
        for o in objs: scn.objects.unlink(o)
