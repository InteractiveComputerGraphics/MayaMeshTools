#include <maya/MIOStream.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <vector>
#include "extern/mzd/readMZD.h"


#define CheckError(stat, msg)		\
	if ( MS::kSuccess != stat )		\
	{								\
		std::cerr << msg;			\
		return MS::kFailure;		\
	}

class MeshLoader: public MPxLocatorNode
{
public:
	enum class FileType { MZD = 0, PLY, OBJ, Unknown, NumFileTypes };

	MeshLoader();
	~MeshLoader() override;

	static void		*creator();
	static MStatus	initialize();
	MStatus	compute( const MPlug& plug, MDataBlock& block ) override;
	virtual bool isBounded() const;
	virtual void postConstructor();
	virtual bool setInternalValue(const MPlug &plug, const MDataHandle &handle);
	virtual bool getInternalValue(const MPlug &plug, MDataHandle &handle);

	static MTypeId m_id;
	static MObject m_outMeshAttr;
	static MObject m_activeAttr;
	static MObject m_meshFileAttr;
	static MObject m_frameIndex;


protected:	
	int m_currentFrame;
	/** Empty output mesh */
	MObject m_emptyMeshObject;
	FileType m_fileType;
	std::string m_lastFileName;
	std::string m_meshFile;

	bool readMZDFile(const std::string &fileName, MObject &outputData);
	bool readPLYFile(const std::string &fileName, MObject &outputData);
	bool readOBJFile(const std::string &fileName, MObject &outputData);

	std::string convertFileName(const std::string &inputFileName, const unsigned int currentFrame);
	std::string zeroPadding(const unsigned int number, const unsigned int length);

	void setEmptyMesh(MArrayDataHandle &arrayData);
};
