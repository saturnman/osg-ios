/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/

#include "DataSetLayer.h"

#include <osg/Notify>
#include <osg/io_utils>

#include <cpl_string.h>
#include <gdalwarper.h>
#include <ogr_spatialref.h>

using namespace GDALPlugin;

DataSetLayer::DataSetLayer():
    _dataset(0)
{
}

DataSetLayer::DataSetLayer(const std::string& fileName):
    _dataset(0)
{
    openFile(fileName);
}

DataSetLayer::DataSetLayer(const DataSetLayer& dataSetLayer,const osg::CopyOp& copyop):
    ProxyLayer(dataSetLayer)
{
    if (dataSetLayer._dataset) open();
}

DataSetLayer::~DataSetLayer()
{
    close();
}

void DataSetLayer::open()
{
    if (_dataset) return;

    if (getFileName().empty()) return;
    
    _dataset = static_cast<GDALDataset*>(GDALOpen(getFileName().c_str(),GA_ReadOnly));
    
    setUpLocator();
}

void DataSetLayer::close()
{
    if (_dataset)
    {
        GDALClose(static_cast<GDALDatasetH>(_dataset));
        
        _dataset = 0;
    }
}

unsigned int DataSetLayer::getNumColumns() const
{
    return _dataset!=0 ? _dataset->GetRasterXSize() : 0;
}

unsigned int DataSetLayer::getNumRows() const
{
    return _dataset!=0 ? _dataset->GetRasterYSize() : 0;
}

osgTerrain::ImageLayer* DataSetLayer::extractImageLayer(unsigned int sourceMinX, unsigned int sourceMinY, unsigned int sourceMaxX, unsigned int sourceMaxY, unsigned int targetWidth, unsigned int targetHeight)
{
    if (!_dataset || sourceMaxX<sourceMinX || sourceMaxY<sourceMinY) return 0;

    osg::notify(osg::NOTICE)<<"DataSetLayer::extractImageLayer("<<sourceMinX<<", "<<sourceMinY<<", "<<sourceMaxX<<", "<<sourceMaxY<<", target:"<<targetWidth<<", "<<targetHeight<<") not yet implemented"<<std::endl;

    return 0;
}

void DataSetLayer::setUpLocator()
{
    osg::notify(osg::NOTICE)<<"DataSetLayer::setUpLocator()"<<std::endl;
    if (!isOpen()) return;
    
    const char* pszSourceSRS = _dataset->GetProjectionRef();
    if (!pszSourceSRS || strlen(pszSourceSRS)==0) pszSourceSRS = _dataset->GetGCPProjection();


    osg::ref_ptr<osgTerrain::Locator> locator = new osgTerrain::Locator;

    if (pszSourceSRS)
    {
        locator->setFormat("WKT");
        locator->setCoordinateSystem(pszSourceSRS);
    }

    osg::Matrixd matrix;

    double geoTransform[6];
    if (_dataset->GetGeoTransform(geoTransform)==CE_None)
    {
#ifdef SHIFT_RASTER_BY_HALF_CELL
        // shift the transform to the middle of the cell if a raster interpreted as vector
        if (data->_dataType == VECTOR)
        {
            geoTransform[0] += 0.5 * geoTransform[1];
            geoTransform[3] += 0.5 * geoTransform[5];
        }
#endif

        matrix.set( geoTransform[1],    geoTransform[4],    0.0,    0.0,
                    geoTransform[2],    geoTransform[5],    0.0,    0.0,
                    0.0,                0.0,                1.0,    0.0,
                    geoTransform[0],    geoTransform[3],    0.0,    1.0);
                    
        locator->setTransform(matrix);
        locator->setDefinedInFile(true);

        setLocator(locator.get());    

    }
    else if (_dataset->GetGCPCount()>0 && _dataset->GetGCPProjection())
    {
        osg::notify(osg::NOTICE) << "    Using GCP's"<< std::endl;


        /* -------------------------------------------------------------------- */
        /*      Create a transformation object from the source to               */
        /*      destination coordinate system.                                  */
        /* -------------------------------------------------------------------- */
        void *hTransformArg = 
            GDALCreateGenImgProjTransformer( _dataset, pszSourceSRS, 
                                             NULL, pszSourceSRS, 
                                             TRUE, 0.0, 1 );

        if ( hTransformArg == NULL )
        {
            osg::notify(osg::NOTICE)<<" failed to create transformer"<<std::endl;
            return;
        }

        /* -------------------------------------------------------------------- */
        /*      Get approximate output definition.                              */
        /* -------------------------------------------------------------------- */
        double adfDstGeoTransform[6];
        int nPixels=0, nLines=0;
        if( GDALSuggestedWarpOutput( _dataset, 
                                     GDALGenImgProjTransform, hTransformArg, 
                                     adfDstGeoTransform, &nPixels, &nLines )
            != CE_None )
        {
            osg::notify(osg::NOTICE)<<" failed to create warp"<<std::endl;
            return;
        }

        GDALDestroyGenImgProjTransformer( hTransformArg );


        matrix.set( adfDstGeoTransform[1],    adfDstGeoTransform[4],    0.0,    0.0,
                    adfDstGeoTransform[2],    adfDstGeoTransform[5],    0.0,    0.0,
                    0.0,                0.0,                1.0,    0.0,
                    adfDstGeoTransform[0],    adfDstGeoTransform[3],    0.0,    1.0);

        locator->setTransform(matrix);
        locator->setDefinedInFile(true);

        setLocator(locator.get());    
    }
    else
    {
        osg::notify(osg::NOTICE) << "    No GeoTransform or GCP's - unable to compute position in space"<< std::endl;
    }

}