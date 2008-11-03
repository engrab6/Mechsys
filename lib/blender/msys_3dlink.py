#!BPY

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
from   Blender import BGL, Draw, Window
import bpy
import msys_dict as di


# Transformation matrix
dict = di.load_dict()
if dict['show_props'] or dict['res_show']:
    # Buffer
    view_matrix = Window.GetPerspMatrix()
    view_buffer = [view_matrix[i][j] for i in xrange(4) for j in xrange(4)]
    view_buffer = BGL.Buffer(BGL.GL_FLOAT, 16, view_buffer)

    # Initialize
    BGL.glPushMatrix   ()
    BGL.glEnable       (Blender.BGL.GL_DEPTH_TEST)
    BGL.glLoadIdentity ()
    BGL.glMatrixMode   (Blender.BGL.GL_PROJECTION)
    BGL.glLoadMatrixf  (view_buffer)


# Mesh properties
if dict['show_props']:
    # Draw
    scn = bpy.data.scenes.active
    obs = scn.objects.selected
    edm = Blender.Window.EditMode()
    for obj in obs:
        if obj!=None and obj.type=='Mesh':

            # draw only if active layer corresponds to this object.Layer
            if Blender.Window.GetActiveLayer()==obj.Layer:

                # get mesh and transform to global coordinates
                msh = obj.getData(mesh=1)
                ori = msh.verts[:] # create a copy before transforming to global coordinates
                msh.transform(obj.matrix)

                # draw vertices IDs
                if dict['show_v_ids']:
                    for v in msh.verts:
                        BGL.glColor3f     (1.0, 1.0, 0.0)
                        BGL.glRasterPos3f (v.co[0], v.co[1], v.co[2])
                        Draw.Text         (str(v.index))

                # draw edges IDs
                if dict['show_e_ids']:
                    for e in msh.edges:
                        mid = 0.5*(e.v1.co+e.v2.co)
                        BGL.glColor3f     (1.0, 1.0, 1.0)
                        BGL.glRasterPos3f (mid[0], mid[1], mid[2])
                        Draw.Text         (str(e.index))

                # draw edge tags
                if dict['show_etags']:
                    if obj.properties.has_key('etags'):
                        for k, v in obj.properties['etags'].iteritems():
                            eid = int(k)
                            pos = msh.edges[eid].v1.co + 0.60*(msh.edges[eid].v2.co-msh.edges[eid].v1.co)
                            BGL.glColor3f     (0.0, 0.0, 0.0)
                            BGL.glRasterPos3f (pos[0], pos[1], pos[2])
                            Draw.Text         (str(v[0]))

                # draw face tags
                if dict['show_ftags']:
                    if obj.properties.has_key('ftags'):
                        BGL.glBlendFunc (BGL.GL_SRC_ALPHA, BGL.GL_ONE)
                        for k, v in obj.properties['ftags'].iteritems():
                            clr      = di.hex2rgb(v[1])
                            ids      = [int(id) for id in k.split('_')]
                            eds, vds = di.sort_edges_and_verts (msh, ids, msh.edges[ids[0]].v1.index) # will erase ids
                            cen      = msh.verts[vds[0]].co/len(eds)
                            for i in range(1,len(eds)):
                                cen += msh.verts[vds[i]].co/len(eds)
                            BGL.glColor4f     (clr[0], clr[1], clr[2], dict['show_opac'])
                            if dict['show_opac']<0.9:
                                BGL.glEnable  (BGL.GL_BLEND)
                                BGL.glDisable (BGL.GL_DEPTH_TEST)
                            BGL.glBegin       (BGL.GL_TRIANGLE_FAN)
                            BGL.glVertex3f    (cen[0], cen[1], cen[2])
                            for i in range(len(eds)):
                                BGL.glVertex3f(msh.verts[vds[i]].co[0], msh.verts[vds[i]].co[1], msh.verts[vds[i]].co[2])
                            BGL.glVertex3f    (msh.verts[vds[0]].co[0], msh.verts[vds[0]].co[1], msh.verts[vds[0]].co[2])
                            BGL.glEnd         ()
                            if dict['show_opac']<0.9:
                                BGL.glDisable (BGL.GL_BLEND)
                                BGL.glEnable  (BGL.GL_DEPTH_TEST)
                            BGL.glColor3f     (0.0, 0.0, 0.0)
                            BGL.glRasterPos3f (cen[0], cen[1], cen[2])
                            Draw.Text         (str(v[0]))

                # draw block IDs
                if dict['show_blks']:
                    if obj.properties.has_key('blks'):
                        for k, v in obj.properties['blks'].iteritems():
                            neds = int(v[17])
                            cen  = 0.5*(msh.edges[int(v[18])].v1.co+msh.edges[int(v[18])].v2.co)/neds
                            for i in range(19,19+neds-1):
                                cen += 0.5*(msh.edges[int(v[i])].v1.co+msh.edges[int(v[i])].v2.co)/neds
                            BGL.glColor3f     (0.2, 1.0, 0.2)
                            BGL.glRasterPos3f (cen[0], cen[1], cen[2])
                            Draw.Text         ('blk:%d:%d'%(int(k),int(v[0])))

                # draw local axes
                if dict['show_axes']:
                    if obj.properties.has_key('blks'):
                        for k, v in obj.properties['blks'].iteritems():
                            eids = [int(v[1]), int(v[2]), int(v[3])]
                            clrs = [(1.0,0.1,0.1), (0.1,1.0,0.1), (0.1,0.1,1.0)]
                            for i, eid in enumerate(eids):
                                if eid>=0:
                                    ed = msh.edges[eid]
                                    BGL.glColor3f  (clrs[i][0], clrs[i][1], clrs[i][2])
                                    BGL.glBegin    (BGL.GL_LINES)
                                    BGL.glVertex3f (ed.v1.co[0], ed.v1.co[1], ed.v1.co[2])
                                    BGL.glVertex3f (ed.v2.co[0], ed.v2.co[1], ed.v2.co[2])
                                    BGL.glEnd      ()
                            origin, xp, yp, zp = int(v[4]), int(v[5]), int(v[6]), int(v[7])
                            if origin>=0:
                                BGL.glColor3f     (0.0, 0.0, 0.0)
                                BGL.glRasterPos3f (msh.verts[origin].co[0], msh.verts[origin].co[1], msh.verts[origin].co[2])
                                Draw.Text         ('o')
                            if xp>=0:
                                BGL.glColor3f     (0.0, 0.0, 0.0)
                                BGL.glRasterPos3f (msh.verts[xp].co[0], msh.verts[xp].co[1], msh.verts[xp].co[2])
                                Draw.Text         ('+')
                            if yp>=0:
                                BGL.glColor3f     (0.0, 0.0, 0.0)
                                BGL.glRasterPos3f (msh.verts[yp].co[0], msh.verts[yp].co[1], msh.verts[yp].co[2])
                                Draw.Text         ('+')
                            if zp>=0:
                                BGL.glColor3f     (0.0, 0.0, 0.0)
                                BGL.glRasterPos3f (msh.verts[zp].co[0], msh.verts[zp].co[1], msh.verts[zp].co[2])
                                Draw.Text         ('+')

                # draw regions and holes
                if dict['show_regs']:
                    if obj.properties.has_key('regs'):
                        for k, v in obj.properties['regs'].iteritems():
                            BGL.glColor3f     (0.0, 0.0, 0.0)
                            BGL.glRasterPos3f (v[2],v[3],v[4])
                            Draw.Text         ('reg:%d:%d'%(int(k),int(v[0])))
                    if obj.properties.has_key('hols'):
                        for k, v in obj.properties['hols'].iteritems():
                            BGL.glColor3f     (0.0, 0.0, 0.0)
                            BGL.glRasterPos3f (v[0],v[1],v[2])
                            Draw.Text         ('hol:%d'%int(k))

                # draw elements information
                if dict['show_elems']:
                    try:    nelems = obj.properties['nelems']
                    except: nelems = 0
                    if nelems>0:
                        for id in obj.properties['elems']['cons']:
                            x, y, z = di.get_cg (msh, obj.properties['elems']['cons'][id], obj.properties['elems']['vtks'][int(id)])
                            BGL.glColor3f     (0.0, 1.0, 0.0)
                            BGL.glRasterPos3f (x, y, z)
                            Draw.Text         (str(id)+'('+str(obj.properties['elems']['tags'][int(id)])+')')

                # Resore mesh to local coordinates
                msh.verts = ori


# Results (visualisation)
if dict['res_show']:
    # Draw
    scn = bpy.data.scenes.active
    obs = scn.objects.selected
    edm = Blender.Window.EditMode()
    for obj in obs:
        if obj!=None and obj.type=='Mesh':

            # draw only if active layer corresponds to this object.Layer
            if Blender.Window.GetActiveLayer()==obj.Layer:

                # get mesh and transform to global coordinates
                msh = obj.getData(mesh=1)
                ori = msh.verts[:] # create a copy before transforming to global coordinates
                msh.transform(obj.matrix)

                # Post-processing
                if dict['res_show_scalar']:
                    key = dict['res_scalar']
                    try:
                        vals = obj.properties['scalars'][key]
                        BGL.glColor3f (0.0, 0.0, 0.0)
                        for v in msh.verts:
                            BGL.glRasterPos3f (v.co[0], v.co[1], v.co[2])
                            Draw.Text         (str(vals[v.index]))
                    except: pass

                if dict['res_show_warp']:
                    ux = []
                    uy = []
                    uz = []
                    try:
                        ux = obj.properties['scalars']['ux']
                        uy = obj.properties['scalars']['uy']
                        uz = obj.properties['scalars']['uz']
                    except: pass
                    if len(ux)>0 or len(uy)>0 or len(uz)>0:
                        if len(ux)<1: ux = [0 for i in range(len(msh.verts))]
                        if len(uy)<1: uy = [0 for i in range(len(msh.verts))]
                        if len(uz)<1: uz = [0 for i in range(len(msh.verts))]
                        m = float(dict['res_warp_scale'])
                        BGL.glColor3f (0.85, 0.85, 0.85)
                        for e in msh.edges:
                            BGL.glBegin    (BGL.GL_LINES)
                            BGL.glVertex3f (e.v1.co[0]+m*ux[e.v1.index], e.v1.co[1]+m*uy[e.v1.index], e.v1.co[2]+m*uz[e.v1.index])
                            BGL.glVertex3f (e.v2.co[0]+m*ux[e.v2.index], e.v2.co[1]+m*uy[e.v2.index], e.v2.co[2]+m*uz[e.v2.index])
                            BGL.glEnd      ()

                # Resore mesh to local coordinates
                msh.verts = ori


# Restore transformation matrix
if dict['show_props'] or dict['res_show']:
    # Restore
    BGL.glPopMatrix ()
    BGL.glDisable   (Blender.BGL.GL_DEPTH_TEST)
