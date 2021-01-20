#include <math.h>
#include <algorithm>
#include <stdlib.h>

#include "MeshLoader.h"
#include "FileSystem.h"
#include "extern/happly/happly.h"
#include "OBJLoader.h"

#include <maya/MVectorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MArrayDataBuilder.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnMatrixData.h>

#include <maya/MFnStringData.h>
#include <maya/MPlugArray.h>
#include <maya/MFnTypedAttribute.h>

#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <maya/MGlobal.h>
#include <maya/MNodeMessage.h>
#include <maya/MAnimControl.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MFnTransform.h>
#include <maya/MCommandResult.h>


MTypeId MeshLoader::m_id( 0x83333 );
MObject MeshLoader::m_activeAttr;
MObject MeshLoader::m_meshFileAttr;
MObject MeshLoader::m_frameIndex;
MObject MeshLoader::m_outMeshAttr;

MeshLoader::MeshLoader()
{
	m_currentFrame = -1;
	m_lastFileName = "";
	m_meshFile = "c:/example/mesh_data_###.ply";
}


MeshLoader::~MeshLoader()
{
}

void *MeshLoader::creator()
{
    return new MeshLoader;
}

MStatus MeshLoader::initialize()
{
	MFnTypedAttribute tAttr;
	MFnNumericAttribute nAttr;

	MFnStringData fnStringData;
	MObject defaultString;

	// output
	m_outMeshAttr = tAttr.create("outMesh", "oM", MFnData::kMesh, MObject::kNullObj);
	nAttr.setReadable(true);
	nAttr.setKeyable(false);
	nAttr.setConnectable(true);
	nAttr.setStorable(false);
	tAttr.setArray(true);
	addAttribute(m_outMeshAttr);

	// Create the default string.
	defaultString = fnStringData.create("c:/example/mesh_data_###.ply");

	m_activeAttr = nAttr.create("active", "act", MFnNumericData::kBoolean, 1.0);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(false);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_activeAttr);

	m_meshFileAttr = tAttr.create("meshFile", "mFile", MFnStringData::kString, defaultString);
	tAttr.setReadable(true);
	tAttr.setWritable(true);
	tAttr.setKeyable(false);
	tAttr.setConnectable(true);
	tAttr.setStorable(true);
	tAttr.setInternal(true);
	addAttribute(m_meshFileAttr);

	m_frameIndex = nAttr.create("frameIndex", "fIndex", MFnNumericData::kInt, 1);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(true);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_frameIndex);

	attributeAffects(m_meshFileAttr, m_outMeshAttr);
	attributeAffects(m_frameIndex, m_outMeshAttr);
	attributeAffects(m_activeAttr, m_outMeshAttr);

	return( MS::kSuccess );
}

void MeshLoader::postConstructor()
{
	// create empty surface (for OpenGL only output)
	MFnMeshData meshData;
	m_emptyMeshObject = meshData.create();
	MFnMesh fnMesh;
	fnMesh.create(0, 0, MPointArray(), MIntArray(), MIntArray(), m_emptyMeshObject);
}

void  MeshLoader::setEmptyMesh(MArrayDataHandle &arrayData)
{
	const unsigned int count = arrayData.elementCount();
	for (unsigned int i = 0; i < count; i++)
	{
		MDataHandle outMeshHandle = arrayData.outputValue();
		outMeshHandle.set(m_emptyMeshObject);
	}
}


MStatus MeshLoader::compute(const MPlug& plug, MDataBlock& block)
{
	MStatus status;

	if (plug != m_outMeshAttr)
        return( MS::kUnknownParameter );

	MArrayDataHandle arrayData = block.outputArrayValue(m_outMeshAttr);
	const unsigned int count = arrayData.elementCount();

	MString meshFile = block.inputValue(m_meshFileAttr).asString();

	if (meshFile == "")
	{
		setEmptyMesh(arrayData);
		return (MS::kFailure);
	}

	bool active = block.inputValue(m_activeAttr).asBool();
	if (!active)
	{
		block.setClean(plug);
		return MS::kSuccess;
	}


	// remove all maya transforms
	MFnDagNode me(thisMObject());
	MFnTransform myTransform(me.parent(0));
	MMatrix trans = myTransform.transformation().asMatrixInverse();

	int frameIndex = block.inputValue(m_frameIndex).asInt();

	std::string currentFile = convertFileName(meshFile.asChar(), frameIndex);
	if (currentFile == "")
	{
		setEmptyMesh(arrayData);
		return (MS::kFailure);
	}
	if (currentFile == m_lastFileName)
		return MS::kSuccess;
	m_lastFileName = currentFile;
	std::cout << "Current file: " << currentFile << "\n";

	MFnMeshData dataCreator;
	MObject newOutputData = dataCreator.create();

	std::string fileExt = Utilities::FileSystem::getFileExt(currentFile);
	transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);
	m_fileType = FileType::Unknown;
	if (fileExt == "MZD")
	{
		m_fileType = FileType::MZD;
		if (!readMZDFile(currentFile, newOutputData))
		{
			setEmptyMesh(arrayData);
			return (MS::kFailure);
		}
	}
	else if (fileExt == "PLY")
	{
		m_fileType = FileType::PLY;
		if (!readPLYFile(currentFile, newOutputData))
		{
			setEmptyMesh(arrayData);
			return (MS::kFailure);
		}
	}
	else if (fileExt == "OBJ")
	{
		m_fileType = FileType::OBJ;
		if (!readOBJFile(currentFile, newOutputData))
		{
			setEmptyMesh(arrayData);
			return (MS::kFailure);
		}
	}
		
	for (unsigned int i = 0; i < count; i++)
	{
		// compute the outgoing mesh
		MDataHandle outMeshHandle = arrayData.outputValue();
		outMeshHandle.set(newOutputData);

		if (!arrayData.next())
			break;
	}

	block.setClean( plug );

	return( MS::kSuccess );
}

bool MeshLoader::isBounded() const
{
	return false;
}

bool MeshLoader::readMZDFile(const std::string &fileName, MObject &outputData)
{
	// declare and init the pointers.
	int              numVertices = 0;    // amount of vertices.
	int              numPolygons = 0;    // amount of polygons.
	int              numNodes = 0;		   // amount of polygon nodes.
	float           *vertPositions = nullptr;			// array of vertex positions.               Array size is 3 * numVertices.
	unsigned char   *polyVIndicesNum = nullptr;		// array of polygon vertex indices count.   Array size is numPolygons.
	int             *polyVIndices = nullptr;			// array of polygon vertex indices.         Array size is numNodes.
	float           *vertNormals = nullptr;			// array of vertex normal vectors.          Array size is 3 * numVertices.
	float           *vertMotions = nullptr;			// array of vertex motion vectors.          Array size is 3 * numVertices.
	float           *vertColors = nullptr;				// array of vertex colors.                  array size is 4 * numVertices.
	float           *vertUVWs = nullptr;				// array of vertex UVW coordinates.         Array size is 3 * numVertices.
	float           *nodeNormals = nullptr;			// array of polygon node normal vectors.    Array size is 3 * numNodes.
	float           *nodeColors = nullptr;				// array of polygon node colors.            Array size is 4 * numNodes.
	float           *nodeUVWs = nullptr;				// array of polygon node UVW coordinates.   Array size is 3 * numNodes.

	// read the .mzd file.
	int ret = readMZD(fileName.c_str(), numVertices, numPolygons, numNodes, &vertPositions, &polyVIndicesNum, &polyVIndices, &vertNormals,
					&vertMotions, &vertColors, &vertUVWs, &nodeNormals, &nodeColors, &nodeUVWs);
	switch (ret)
	{
		case 0:     break;  // success      
		case -1:    MGlobal::displayError("Error: unable to open file."); return false;
		case -2:    MGlobal::displayError("Error: read error."); return false;
		case -3:    MGlobal::displayError("Error: failed to allocate memory."); return false;
		case -4:    MGlobal::displayError("Error: wrong file format."); return false;
		case -5:    MGlobal::displayError("Error: illegal parameter value."); return false;
		default:    MGlobal::displayError("Error: unkown error."); return false;
	}

	// Read points
	MPointArray points;
	MVectorArray vNormals;
	MColorArray vColors;
	MIntArray vertexList;

	points.setLength(numVertices);
	vertexList.setLength(numVertices);

	if (vertNormals)
		vNormals.setLength(numVertices);
	if (vertColors)
		vColors.setLength(numVertices);

	for (int j = 0; j < numVertices; j++)
	{
		vertexList[j] = j;
		MPoint p(vertPositions[3 * j], vertPositions[3 * j + 1], vertPositions[3 * j + 2]);
		p.cartesianize();
		points[j] = p;

		if (vertNormals)
			vNormals[j] = MVector(vertNormals[3 * j], vertNormals[3 * j + 1], vertNormals[3 * j + 2]);
		if (vertColors)
			vColors[j] = MColor(vertColors[4 * j], vertColors[4 * j + 1], vertColors[4 * j + 2], vertColors[4 * j + 3]);
	}

	// Read faces	
	//MIntArray uvIndices;
	MIntArray polyConnects;
	MIntArray polyCounts;
	//uvIndices.setLength((nFaces) * 3);
	polyCounts.setLength(numPolygons);
	polyConnects.setLength(numNodes);
	unsigned int currentIndex = 0;
	for (int j = 0; j < numPolygons; j++)
		polyCounts[j] = polyVIndicesNum[j];

	for (int j = 0; j < numNodes; j++)
		polyConnects[j] = polyVIndices[j];

	MFnMesh outputMesh;
	MObject outputMeshObj = outputMesh.create(numVertices, numPolygons, points, polyCounts, polyConnects, outputData);

	if (vertNormals)
		outputMesh.setVertexNormals(vNormals, vertexList);

	if (vertColors)
		outputMesh.setVertexColors(vColors, vertexList);

	// set the updates
	outputMesh.updateSurface();

	delete[] vertPositions;
	delete[] polyVIndicesNum;
	delete[] polyVIndices;
	delete[] vertNormals;
	delete[] vertMotions;
	delete[] vertColors;
	delete[] vertUVWs;
	delete[] nodeNormals;
	delete[] nodeColors;
	delete[] nodeUVWs;

	MGlobal::displayInfo(MString("# vertices: ") + numVertices);
	MGlobal::displayInfo(MString("# faces: ") + numPolygons);

	return true;
}

bool MeshLoader::readPLYFile(const std::string &fileName, MObject &outputData)
{
	try
	{
		// Construct a data object by reading from file
		happly::PLYData plyIn(fileName);

		MPointArray points;
		MVectorArray vNormals;
		MColorArray vColors;
		MIntArray vertexList;

		happly::Element &element = plyIn.getElement("vertex");

		int numVertices = 0;
		int numPolygons = 0;

		// vertices
		if ((element.hasPropertyType<float>("x")) &&
			(element.hasPropertyType<float>("y")) &&
			(element.hasPropertyType<float>("z")))
		{
			std::vector<float> x = element.getProperty<float>("x");
			std::vector<float> y = element.getProperty<float>("y");
			std::vector<float> z = element.getProperty<float>("z");

			numVertices = (int) x.size();
			points.setLength(numVertices);
			vertexList.setLength(numVertices);
			for (int i = 0; i < numVertices; i++)
			{
				vertexList[i] = i;
				MPoint p(x[i], y[i], z[i]);
				p.cartesianize();
				points[i] = p;
			}
		}
		else if ((element.hasPropertyType<double>("x")) &&
				(element.hasPropertyType<double>("y")) &&
				(element.hasPropertyType<double>("z")))
		{
			std::vector<double> x = element.getProperty<double>("x");
			std::vector<double> y = element.getProperty<double>("y");
			std::vector<double> z = element.getProperty<double>("z");

			numVertices = (int) x.size();
			points.setLength(numVertices);
			vertexList.setLength(numVertices);
			for (int i = 0; i < numVertices; i++)
			{
				vertexList[i] = i;
				MPoint p(x[i], y[i], z[i]);
				p.cartesianize();
				points[i] = p;
			}
		}
		else
			return false;

				
		// normals
		bool foundNormals = false;
		if ((element.hasPropertyType<float>("nx")) &&
			(element.hasPropertyType<float>("ny")) &&
			(element.hasPropertyType<float>("nz")))
		{
			std::vector<float> nx = element.getProperty<float>("nx");
			std::vector<float> ny = element.getProperty<float>("ny");
			std::vector<float> nz = element.getProperty<float>("nz");

			vNormals.setLength((int)nx.size());
			for (int i = 0; i < (int)nx.size(); i++)
				vNormals[i] = MVector(nx[i], ny[i], nz[i]);
			foundNormals = true;
		}
		else if ((element.hasPropertyType<double>("nx")) &&
				(element.hasPropertyType<double>("ny")) &&
				(element.hasPropertyType<double>("nz")))
		{
			std::vector<double> nx = element.getProperty<double>("nx");
			std::vector<double> ny = element.getProperty<double>("ny");
			std::vector<double> nz = element.getProperty<double>("nz");

			vNormals.setLength((int)nx.size());
			for (int i = 0; i < (int)nx.size(); i++)
				vNormals[i] = MVector(nx[i], ny[i], nz[i]);
			foundNormals = true;
		}

		// vertex colors
		bool foundVertColors = false;
		if ((element.hasPropertyType<unsigned char>("red")) &&
			(element.hasPropertyType<unsigned char>("green")) &&
			(element.hasPropertyType<unsigned char>("blue")))
		{
			std::vector<unsigned char> r = element.getProperty<unsigned char>("red");
			std::vector<unsigned char> g = element.getProperty<unsigned char>("green");
			std::vector<unsigned char> b = element.getProperty<unsigned char>("blue");

			vColors.setLength((int)r.size());
			for (int i = 0; i < (int)r.size(); i++)
				vColors[i] = MColor((float)r[i] / 255.0f, (float)g[i] / 255.0f, (float)b[i] / 255.0f, 1.0f);		
			foundVertColors = true;
		}
		
		// faces
		MIntArray polyConnects;
		MIntArray polyCounts;
		std::vector<std::vector<size_t>> fInd = plyIn.getFaceIndices<size_t>();
		numPolygons = (int)fInd.size();
		int numNodes = 0;
		for (int i = 0; i < numPolygons; i++)
			numNodes += (int)fInd[i].size();

		polyCounts.setLength(numPolygons);
		polyConnects.setLength(numNodes);
		
		int index = 0;
		for (int i = 0; i < numPolygons; i++)
		{
			for (int j = 0; j < (int)fInd[i].size(); j++)
			{
				polyConnects[index++] = (int)fInd[i][j];
			}
			polyCounts[i] = (int)fInd[i].size();
		}
		
		MFnMesh outputMesh;
		MObject outputMeshObj = outputMesh.create(numVertices, numPolygons, points, polyCounts, polyConnects, outputData);

		if (foundNormals)
			outputMesh.setVertexNormals(vNormals, vertexList);

		if (foundVertColors)
			outputMesh.setVertexColors(vColors, vertexList);

		// set the updates
		outputMesh.updateSurface();

		MGlobal::displayInfo(MString("# vertices: ") + numVertices);
		MGlobal::displayInfo(MString("# faces: ") + numPolygons);
	}
	catch (const std::exception & e)
	{
		MGlobal::displayError(MString("Exception: ") + e.what());
		return false;
	}
	
	return true;
}


bool MeshLoader::readOBJFile(const std::string &fileName, MObject &outputData)
{
	// Construct a data object by reading from file
	std::vector<Utilities::OBJLoader::Vec3f> x;
	std::vector<Utilities::OBJLoader::Vec3f> normals;
	std::vector<Utilities::MeshFaceIndices> faces;
	Utilities::OBJLoader::Vec3f s = { 1.0f, 1.0f, 1.0f };
	Utilities::OBJLoader::loadObj(fileName, &x, &faces, &normals, nullptr, s);

	MPointArray points;
	MVectorArray vNormals;
	MIntArray vertexList;

	int numVertices = (int)x.size();
	int numPolygons = (int)faces.size();

	points.setLength(numVertices);
	vertexList.setLength(numVertices);
	for (int i = 0; i < numVertices; i++)
	{
		vertexList[i] = i;
		MPoint p(x[i][0], x[i][1], x[i][2]);
		p.cartesianize();
		points[i] = p;
	}

	// normals
	bool foundNormals = normals.size() > 0;
	if (foundNormals)
	{
		vNormals.setLength((int)normals.size());
		for (int i = 0; i < (int)normals.size(); i++)
			vNormals[i] = MVector(normals[i][0], normals[i][1], normals[i][2]);
	}

	// faces
	MIntArray polyConnects;
	MIntArray polyCounts;

	polyCounts.setLength(numPolygons);
	polyConnects.setLength(3 * numPolygons);

	int index = 0;
	for (int i = 0; i < numPolygons; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			polyConnects[index++] = faces[i].posIndices[j]-1;
		}
		polyCounts[i] = 3;
	}

	MFnMesh outputMesh;
	MObject outputMeshObj = outputMesh.create(numVertices, numPolygons, points, polyCounts, polyConnects, outputData);

	if (foundNormals)
		outputMesh.setVertexNormals(vNormals, vertexList);

	// set the updates
		outputMesh.updateSurface();

	MGlobal::displayInfo(MString("# vertices: ") + numVertices);
	MGlobal::displayInfo(MString("# faces: ") + numPolygons);

	return true;
}


std::string MeshLoader::zeroPadding(const unsigned int number, const unsigned int length)
{
	std::ostringstream out;
	out << std::internal << std::setfill('0') << std::setw(length) << number;
	return out.str();
}

std::string MeshLoader::convertFileName(const std::string &inputFileName, const unsigned int currentFrame)
{
	std::string fileName = inputFileName;

	// remove 
	char ch = '\"';
	fileName.erase(std::remove(fileName.begin(), fileName.end(), ch), fileName.end());

	std::string::size_type pos1 = fileName.find_first_of("#", 0);
	if (pos1 != std::string::npos)
	{
		std::string::size_type pos2 = fileName.find_first_not_of("#", pos1);
		std::string::size_type length = pos2 - pos1;

		std::string numberStr = zeroPadding(currentFrame, (unsigned int)length);
		fileName.replace(pos1, length, numberStr);
	}

	if (Utilities::FileSystem::isRelativePath(fileName))
	{
		// Get path of the scene file
		MCommandResult result;
		MGlobal::executeCommand(MString("file -q -sn"), result);
		MString sceneFile;
		result.getResult(sceneFile);
		std::string scenePath = Utilities::FileSystem::getFilePath(sceneFile.asChar());
		fileName = Utilities::FileSystem::normalizePath(scenePath + "/" + fileName);
	}
	return fileName;
}

bool MeshLoader::setInternalValue(const MPlug &plug, const MDataHandle &handle)
{
	if (plug == m_meshFileAttr) 
	{
		MString meshFile = handle.asString();
		m_meshFile = meshFile.asChar();

		// remove "
		char ch = '\"';
		m_meshFile.erase(std::remove(m_meshFile.begin(), m_meshFile.end(), ch), m_meshFile.end());

		return true;
	}

	return MPxLocatorNode::setInternalValue(plug, handle);
}

bool MeshLoader::getInternalValue(const MPlug &plug, MDataHandle &handle)
{
	if (plug == m_meshFileAttr) 
	{
		handle.set(MString(m_meshFile.c_str()));
		return true;
	}
	return MPxLocatorNode::getInternalValue(plug, handle);
}
