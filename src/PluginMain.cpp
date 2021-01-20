
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MApiNamespace.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MDrawRegistry.h>
#include <stdio.h>

// nodes
#include "MeshLoader.h"

bool RegisterPluginUI()
{
	// Create menu
	MGlobal::executeCommand(MString("string $mpt_root_menu = `menu -l \"Mesh Tools\" -p MayaWindow -allowOptionBoxes true`"));
	MGlobal::executeCommand(MString("menuItem -label \"Create mesh loader\" -command \"createMeshLoader()\""));

	return true;
}

bool DeregisterPluginUI()
{
	MGlobal::executeCommand(MString("deleteUI -m $mpt_root_menu"));

	// delete template editors
	MGlobal::executeCommand(MString("MPT_RemoveEditorTemplate(\"MeshLoaderNode\")"));

	return true;
}


MStatus initializePlugin(MObject obj) 
{
	MStatus status;
	MFnPlugin plugin(obj, "Jan Bender", "1.0", "Any");

	MString pluginPath = plugin.loadPath();
	MString scriptPath = pluginPath + "/scripts";

	// add path of plugin to script path
#ifdef WIN32
	MGlobal::executeCommand(MString("$s=`getenv \"MAYA_SCRIPT_PATH\"`; putenv \"MAYA_SCRIPT_PATH\" ($s + \";") + scriptPath + "\")");
#else
	MGlobal::executeCommand(MString("$s=`getenv \"MAYA_SCRIPT_PATH\"`; putenv \"MAYA_SCRIPT_PATH\" ($s + \":") + scriptPath + "\")");
#endif
	
	// execute the specified mel script
	status = MGlobal::sourceFile("MayaMeshTools.mel");

	status = plugin.registerNode("MeshLoader", MeshLoader::m_id,
		&MeshLoader::creator, &MeshLoader::initialize,
		MPxNode::kEmitterNode);
	if (!status)
	{
		status.perror("register MeshLoader");
		return status;
	}

	if (!RegisterPluginUI())
	{
		status.perror("Failed to create UI");
		return status;
	}

	return status;
}


MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterNode(MeshLoader::m_id);
	if (!status)
	{
		status.perror("deregister MeshLoader");
		return status;
	}

	if (!DeregisterPluginUI())
	{
		status.perror("Failed to deregister UI");
		return status;
	}

	return status;
}

