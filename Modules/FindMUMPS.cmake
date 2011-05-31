#####################################################################################
# MechSys - A C++ library to simulate (Continuum) Mechanical Systems                #
# Copyright (C) 2005 Dorival de Moraes Pedroso <dorival.pedroso at gmail.com>       #
# Copyright (C) 2005 Raul Dario Durand Farfan  <raul.durand at gmail.com>           #
#                                                                                   #
# This file is part of MechSys.                                                     #
#                                                                                   #
# MechSys is free software; you can redistribute it and/or modify it under the      #
# terms of the GNU General Public License as published by the Free Software         #
# Foundation; either version 2 of the License, or (at your option) any later        #
# version.                                                                          #
#                                                                                   #
# MechSys is distributed in the hope that it will be useful, but WITHOUT ANY        #
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A   #
# PARTICULAR PURPOSE. See the GNU General Public License for more details.          #
#                                                                                   #
# You should have received a copy of the GNU General Public License along with      #
# MechSys; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, #
# Fifth Floor, Boston, MA 02110-1301, USA                                           #
#####################################################################################

SET(MUMPS_INCLUDE_SEARCH_PATH
  $ENV{MECHSYS_ROOT}/pkg/MUMPS_4.9.2/include
  $ENV{HOME}/pkg/MUMPS_4.9.2/include
  $ENV{MECHSYS_ROOT}/pkg/MUMPS_4.10.0/include
  $ENV{HOME}/pkg/MUMPS_4.10.0/include)

SET(MUMPS_LIBRARY_SEARCH_PATH
  $ENV{MECHSYS_ROOT}/pkg/MUMPS_4.9.2/lib
  $ENV{HOME}/pkg/MUMPS_4.9.2/lib
  $ENV{MECHSYS_ROOT}/pkg/MUMPS_4.10.0/lib
  $ENV{HOME}/pkg/MUMPS_4.10.0/lib)

FIND_PATH(MUMPS_MUMPS_H dmumps_c.h ${MUMPS_INCLUDE_SEARCH_PATH})

FIND_LIBRARY(MUMPS_DMUMPS NAMES dmumps       PATHS ${MUMPS_LIBRARY_SEARCH_PATH})
FIND_LIBRARY(MUMPS_CMUMPS NAMES mumps_common PATHS ${MUMPS_LIBRARY_SEARCH_PATH})
FIND_LIBRARY(MUMPS_PMUMPS NAMES pord         PATHS ${MUMPS_LIBRARY_SEARCH_PATH})

SET(MUMPS_FOUND 1)
FOREACH(var MUMPS_MUMPS_H MUMPS_CMUMPS MUMPS_DMUMPS MUMPS_PMUMPS)
  IF(NOT ${var})
	SET(MUMPS_FOUND 0)
  ENDIF(NOT ${var})
ENDFOREACH(var)

IF(MUMPS_FOUND)
  SET(MUMPS_INCLUDE_DIRS ${MUMPS_MUMPS_H})
  SET(MUMPS_LIBRARIES    ${MUMPS_DMUMPS} ${MUMPS_CMUMPS} ${MUMPS_PMUMPS})
ENDIF(MUMPS_FOUND)
