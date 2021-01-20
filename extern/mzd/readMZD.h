/* ____________________________________________________________________________
	Header file for reading ".mzd" mesh files.
	Version 1.0, September 2014.

	Copyright (c) 2014 Eric Mootz

	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely.

	info@mootzoid.com
  ____________________________________________________________________________
*/

#ifndef READMZD_H
#define READMZD_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define READMZD_VERSION 1.0

#define MZD_HEAD	"    MZD-File-Format    "
#define MZD_TAIL	"   >> END OF FILE <<   "

int readMZD(const char		 *in_fname,
			int				 &out_numVertices,
			int				 &out_numPolygons,
			int				 &out_numNodes,
			float			**out_vertPositions,
			unsigned char	**out_polyVIndicesNum,
			int				**out_polyVIndices,
			float			**out_vertNormals	= NULL,
			float			**out_vertMotions	= NULL,
			float			**out_vertColors	= NULL,
			float			**out_vertUVWs		= NULL,
			float			**out_nodeNormals	= NULL,
			float			**out_nodeColors	= NULL,
			float			**out_nodeUVWs		= NULL);


#endif // READMZD_H

