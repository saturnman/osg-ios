#pragma once

#include <osgViewer/Viewer>
#include <osgViewer/StatsHandler>
#include <osgViewer/GraphicsWindowWin32>
#include <osgGA/TrackballManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgDB/DatabasePager>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <string>

class cOSG
{
public:
    cOSG(HWND hWnd);
    ~cOSG(){};

    void InitOSG(std::string filename);
    void InitManipulators(void);
    void InitSceneGraph(void);
    void InitCameraConfig(void);
    void SetupWindow(void);
    void SetupCamera(void);
    static void Render(void* ptr);

    osgViewer::Viewer* getViewer() { return mViewer.get(); }

private:
    std::string m_ModelName;
    HWND m_hWnd;
    osg::ref_ptr<osgViewer::Viewer> mViewer;
    osg::ref_ptr<osg::Group> mRoot;
    osg::ref_ptr<osg::Node> mModel;
    osg::ref_ptr<osgGA::TrackballManipulator> trackball;
    osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator;
};