/* ____________________________________________________________________________
	Header file for reading ".mzd" mesh files.
	Version 1.0, September 2014.

	Copyright (c) 2014 Eric Mootz, Inc. All rights reserved

	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely.

	info@mootzoid.com

	PS: this code is best viewed with a tab width equal 4.

  ____________________________________________________________________________
*/

#include "readMZD.h"

// conversion from half float to float.
union  mzd_h2f { unsigned int ui; float f; };
static mzd_h2f lookupH2F[65536] =
{
	#include "readMZD_half2float.h"
};


// Reads a .mzd file and stores the result in the output arrays. Latter are
// allocated by this function and must be freed manually by the caller once
// they are no longer needed.
// Parameters: see parameter descriptions.
// Return values:	 0: success.
//					-1: unable to open file.
//					-2: read error.
//					-3: failed to allocate memory.
//					-4: wrong file format.
//					-5: illegal parameter value.
int readMZD(const char		 *in_fname,				// input file name.
			int				 &out_numVertices,		// amount of vertices.
			int				 &out_numPolygons,		// amount of polygons.
			int				 &out_numNodes,			// amount of polygon nodes.
			float			**out_vertPositions,	// array of vertex positions.
			unsigned char	**out_polyVIndicesNum,	// array of polygon vertex indices count.
			int				**out_polyVIndices,		// array of polygon vertex indices.
			float			**out_vertNormals,		// array of vertex normal vectors.
			float			**out_vertMotions,		// array of vertex motion vectors.
			float			**out_vertColors,		// array of vertex colors.
			float			**out_vertUVWs,			// array of vertex UVW coordinates.
			float			**out_nodeNormals,		// array of polygon node normal vectors.
			float			**out_nodeColors,		// array of polygon node colors.
			float			**out_nodeUVWs)			// array of polygon node UVW coordinates.
{
	// __________________________________
	// declare and init the return value.
	int ret = 0;

	// ____________________________________________________
	// init outputs and check the mandatory array pointers.
	out_numVertices	= 0;
	out_numPolygons	= 0;
	out_numNodes	= 0;
	if (out_vertPositions)		*out_vertPositions		= NULL;
	if (out_polyVIndicesNum)	*out_polyVIndicesNum	= NULL;
	if (out_polyVIndices)		*out_polyVIndices		= NULL;
	if (out_vertNormals)		*out_vertNormals		= NULL;
	if (out_vertMotions)		*out_vertMotions		= NULL;
	if (out_vertColors)			*out_vertColors			= NULL;
	if (out_vertUVWs)			*out_vertUVWs			= NULL;
	if (out_nodeNormals)		*out_nodeNormals		= NULL;
	if (out_nodeColors)			*out_nodeColors			= NULL;
	if (out_nodeUVWs)			*out_nodeUVWs			= NULL;
	if (!out_vertPositions || !out_polyVIndicesNum || !out_polyVIndices)
		ret = -5;

	// __________
	// open file.
	FILE *stream = fopen(in_fname, "rb");
	if (!stream)
		ret = -1;

	// _____________
	// read content.
	if (ret == 0)
	{
		// read and check the file header.
		char head[24];
		if (fread(head, 1, 24, stream) != 24)	ret = -4;
		else if (memcmp(head, MZD_HEAD, 24))	ret = -4;

		// read the chunks.
		while (ret == 0)
		{
			// read chunk header.
			unsigned int	chunkID;
			char			chunkName[24];
			unsigned int	chunkSize;
			if (fread(&chunkID,   4,  1, stream) !=  1)	{	ret = -2;	break;	}
			if (fread(chunkName,  1, 24, stream) != 24)	{	ret = -2;	break;	}
			if (fread(&chunkSize, 4,  1, stream) !=  1)	{	ret = -2;	break;	}

			// load/skip chunk.
			switch(chunkID)
			{
				case 0x0ABC0001:		// vertices and polygons.
					{
						// read num vertices.
						if (fread(&out_numVertices, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (out_numVertices  < 0)							{	ret = -127;	break;	}
						if (out_numVertices == 0)							{				break;	}

						// allocate memory for vertex positions and fill it.
						*out_vertPositions = (float *)malloc(3 * out_numVertices * sizeof(float));
						if (*out_vertPositions)
						{
							float *v = *out_vertPositions;
							for (int i=0;i<out_numVertices;i++,v+=3)
								if (fread(v, 4, 3, stream) != 3)	{	ret = -2;	break;	}
							if (ret != 0)
								break;
						}
						else
						{
							ret = -3;
							break;
						}

						// read num polygons.
						if (fread(&out_numPolygons, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (out_numPolygons < 0)							{	ret = -127;	break;	}

						// allocate memory for polygon vertex indices count and fill it.
						*out_polyVIndicesNum = (unsigned char *)malloc(out_numPolygons * sizeof(unsigned char));
						if (*out_polyVIndicesNum)
						{
							if (fread(*out_polyVIndicesNum, 1, out_numPolygons, stream) != out_numPolygons)
							{
								ret = -2;
								break;
							}
						}
						else
						{
							ret = -3;
							break;
						}

						// calculate the amount of polygon nodes.
						for (int i=0;i<out_numPolygons;i++)
							out_numNodes += (*out_polyVIndicesNum)[i];

						// read numBytesPerPolyVInd.
						int numBytesPerPolyVInd;
						if (fread(&numBytesPerPolyVInd, 4, 1, stream) != 1)		{	ret = -2;	break;	}

						// allocate memory for polygon vertex indices and fill it.
						*out_polyVIndices = (int *)malloc(out_numNodes * sizeof(int));
						if (*out_polyVIndices)
						{
							// int.
							if (numBytesPerPolyVInd == 4)
							{
								unsigned char	*pvn = *out_polyVIndicesNum;
								int				*pvi = *out_polyVIndices;
								for (int i=0;i<out_numPolygons;i++,pvn++)
								{
									if (fread(pvi, 4, *pvn, stream) != *pvn)
									{
										ret = -2;
										break;
									}
									pvi += *pvn;
								}	
								if (ret != 0)
									break;
							}

							// unsigned short.
							else if (numBytesPerPolyVInd == 2)
							{
								unsigned short	 us256[256];
								unsigned char	*pvn = *out_polyVIndicesNum;
								int				*pvi = *out_polyVIndices;
								for (int i=0;i<out_numPolygons;i++,pvn++)
								{
									if (fread(us256, 2, *pvn, stream) != *pvn)
									{
										ret = -2;
										break;
									}
									for (unsigned char j=0;j<*pvn;j++,pvi++)
										*pvi = us256[j];
								}	
								if (ret != 0)
									break;
							}

							// unsupported.
							else
							{
								ret = -127;
								break;
							}
						}
						else
						{
							ret = -3;
							break;
						}
					}
					break;
				case 0xDA7A0001:		// vertex normals.
					{
						// skip this chunk?
						if (!out_vertNormals)
						{
							if (fseek(stream, chunkSize, SEEK_CUR) != 0)
								ret = -2;
							break;
						}

						// read num.
						int num = 0;
						if (fread(&num, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (num != out_numVertices)				{	ret = -127;	break;	}

						// allocate memory and fill it.
						*out_vertNormals = (float *)malloc(3 * num * sizeof(float));
						if (*out_vertNormals)
						{
							unsigned short	 h[3];
							float			*v = *out_vertNormals;
							for (int i=0;i<num;i++,v+=3)
							{
								if (fread(h, 2, 3, stream) != 3)	{	ret = -2;	break;	}
								v[0] = lookupH2F[h[0]].f;
								v[1] = lookupH2F[h[1]].f;
								v[2] = lookupH2F[h[2]].f;
							}
							if (ret != 0)
								break;
						}
						else
						{
							ret = -3;
							break;
						}
					}
					break;
				case 0xDA7A0002:		// vertex motions.
					{
						// skip this chunk?
						if (!out_vertMotions)
						{
							if (fseek(stream, chunkSize, SEEK_CUR) != 0)
								ret = -2;
							break;
						}

						// read num.
						int num = 0;
						if (fread(&num, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (num != out_numVertices)				{	ret = -127;	break;	}

						// allocate memory and fill it.
						*out_vertMotions = (float *)malloc(3 * num * sizeof(float));
						if (*out_vertMotions)
						{
							unsigned short	 h[3];
							float			*v = *out_vertMotions;
							for (int i=0;i<num;i++,v+=3)
							{
								if (fread(h, 2, 3, stream) != 3)	{	ret = -2;	break;	}
								v[0] = lookupH2F[h[0]].f;
								v[1] = lookupH2F[h[1]].f;
								v[2] = lookupH2F[h[2]].f;
							}
							if (ret != 0)
								break;
						}
						else
						{
							ret = -3;
							break;
						}
					}
					break;
				case 0xDA7A0003:		// vertex colors.
					{
						// skip this chunk?
						if (!out_vertColors)
						{
							if (fseek(stream, chunkSize, SEEK_CUR) != 0)
								ret = -2;
							break;
						}

						// read num.
						int num = 0;
						if (fread(&num, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (num != out_numVertices)				{	ret = -127;	break;	}

						// allocate memory and fill it.
						*out_vertColors = (float *)malloc(4 * num * sizeof(float));
						if (*out_vertColors)
						{
							unsigned short	 h[4];
							float			*v = *out_vertColors;
							for (int i=0;i<num;i++,v+=4)
							{
								if (fread(h, 2, 4, stream) != 4)	{	ret = -2;	break;	}
								v[0] = lookupH2F[h[0]].f;
								v[1] = lookupH2F[h[1]].f;
								v[2] = lookupH2F[h[2]].f;
								v[3] = lookupH2F[h[3]].f;
							}
							if (ret != 0)
								break;
						}
						else
						{
							ret = -3;
							break;
						}
					}
					break;
				case 0xDA7A0004:		// vertex UVWs.
					{
						// skip this chunk?
						if (!out_vertUVWs)
						{
							if (fseek(stream, chunkSize, SEEK_CUR) != 0)
								ret = -2;
							break;
						}

						// read num.
						int num = 0;
						if (fread(&num, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (num != out_numVertices)				{	ret = -127;	break;	}

						// allocate memory and fill it.
						*out_vertUVWs = (float *)malloc(3 * num * sizeof(float));
						if (*out_vertUVWs)
						{
							float *v = *out_vertUVWs;
							for (int i=0;i<num;i++,v+=3)
								if (fread(v, 4, 3, stream) != 3)	{	ret = -2;	break;	}
							if (ret != 0)
								break;
						}
						else
						{
							ret = -3;
							break;
						}
					}
					break;
				case 0xDA7A0011:		// node normals.
					{
						// skip this chunk?
						if (!out_nodeNormals)
						{
							if (fseek(stream, chunkSize, SEEK_CUR) != 0)
								ret = -2;
							break;
						}

						// read num.
						int num = 0;
						if (fread(&num, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (num != out_numNodes)				{	ret = -127;	break;	}

						// allocate memory and fill it.
						*out_nodeNormals = (float *)malloc(3 * num * sizeof(float));
						if (*out_nodeNormals)
						{
							unsigned short	 h[3];
							float			*v = *out_nodeNormals;
							for (int i=0;i<num;i++,v+=3)
							{
								if (fread(h, 2, 3, stream) != 3)	{	ret = -2;	break;	}
								v[0] = lookupH2F[h[0]].f;
								v[1] = lookupH2F[h[1]].f;
								v[2] = lookupH2F[h[2]].f;
							}
							if (ret != 0)
								break;
						}
						else
						{
							ret = -3;
							break;
						}
					}
					break;
				case 0xDA7A0013:		// node colors.
					{
						// skip this chunk?
						if (!out_nodeColors)
						{
							if (fseek(stream, chunkSize, SEEK_CUR) != 0)
								ret = -2;
							break;
						}

						// read num.
						int num = 0;
						if (fread(&num, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (num != out_numNodes)				{	ret = -127;	break;	}

						// allocate memory and fill it.
						*out_nodeColors = (float *)malloc(4 * num * sizeof(float));
						if (*out_nodeColors)
						{
							unsigned short	 h[4];
							float			*v = *out_nodeColors;
							for (int i=0;i<num;i++,v+=4)
							{
								if (fread(h, 2, 4, stream) != 4)	{	ret = -2;	break;	}
								v[0] = lookupH2F[h[0]].f;
								v[1] = lookupH2F[h[1]].f;
								v[2] = lookupH2F[h[2]].f;
								v[3] = lookupH2F[h[3]].f;
							}
							if (ret != 0)
								break;
						}
						else
						{
							ret = -3;
							break;
						}
					}
					break;
				case 0xDA7A0014:		// node UVWs.
					{
						// skip this chunk?
						if (!out_nodeUVWs)
						{
							if (fseek(stream, chunkSize, SEEK_CUR) != 0)
								ret = -2;
							break;
						}

						// read num.
						int num = 0;
						if (fread(&num, 4, 1, stream) != 1)		{	ret = -2;	break;	}
						if (num != out_numNodes)				{	ret = -127;	break;	}

						// allocate memory and fill it.
						*out_nodeUVWs = (float *)malloc(3 * num * sizeof(float));
						if (*out_nodeUVWs)
						{
							float *v = *out_nodeUVWs;
							for (int i=0;i<num;i++,v+=3)
								if (fread(v, 4, 3, stream) != 3)	{	ret = -2;	break;	}
							if (ret != 0)
								break;
						}
						else
						{
							ret = -3;
							break;
						}
					}
					break;
				default:				// skip this chunk.
					{
						if (fseek(stream, chunkSize, SEEK_CUR) != 0)
							ret = -2;
					}
					break;
			}

			// error?
			if (ret != 0)
				break;

			// have we reached the end?
			char tail[24];
			if (fread(tail, 1, 24, stream) != 24)	{	ret = -2;	break;	}
			if (memcmp(tail, MZD_TAIL, 24) == 0)	{				break;	}
			else
			{
				// still some chunks left.
				if (fseek(stream, -24, SEEK_CUR) != 0)
				{
					ret = -2;
					break;
				}
			}
		}
	}

	// _______________
	// close the file.
	if (stream)
		fclose(stream);

	// _________
	// clean up?
	if (ret != 0)
	{
		out_numVertices	= 0;
		out_numPolygons	= 0;
		out_numNodes	= 0;
		if (out_vertPositions	&&	*out_vertPositions)		{	free(*out_vertPositions);	*out_vertPositions		= NULL;	}
		if (out_polyVIndicesNum	&&	*out_polyVIndicesNum)	{	free(*out_polyVIndicesNum);	*out_polyVIndicesNum	= NULL;	}
		if (out_polyVIndices	&&	*out_polyVIndices)		{	free(*out_polyVIndices);	*out_polyVIndices		= NULL;	}
		if (out_vertNormals		&&	*out_vertNormals)		{	free(*out_vertNormals);		*out_vertNormals		= NULL;	}
		if (out_vertMotions		&&	*out_vertMotions)		{	free(*out_vertMotions);		*out_vertMotions		= NULL;	}
		if (out_vertColors		&&	*out_vertColors)		{	free(*out_vertColors);		*out_vertColors			= NULL;	}
		if (out_vertUVWs		&&	*out_vertUVWs)			{	free(*out_vertUVWs);		*out_vertUVWs			= NULL;	}
		if (out_nodeNormals		&&	*out_nodeNormals)		{	free(*out_nodeNormals);		*out_nodeNormals		= NULL;	}
		if (out_nodeColors		&&	*out_nodeColors)		{	free(*out_nodeColors);		*out_nodeColors			= NULL;	}
		if (out_nodeUVWs		&&	*out_nodeUVWs)			{	free(*out_nodeUVWs);		*out_nodeUVWs			= NULL;	}
	}

	// _____
	// done.
	return ret;
}


