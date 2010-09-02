/****************************************************************************
* MeshLab                                                           o o     *
* A versatile mesh processing toolbox                             o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005                                                \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *   
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
/****************************************************************************
  History
$Log: samplefilter.cpp,v $
****************************************************************************/

#include <QtGui>

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "vsa.h"
#include "region_growing.h"

#include <vcg/complex/trimesh/clean.h>
#include <vcg/complex/trimesh/allocate.h>
#include <vcg/complex/trimesh/update/position.h>
#include <vcg/complex/trimesh/update/normal.h>
#include <vcg/complex/trimesh/update/bounding.h>
#include<vcg/complex/trimesh/append.h>


using namespace vcg;
using namespace std;
// Constructor usually performs only two simple tasks of filling the two lists 
//  - typeList: with all the possible id of the filtering actions
//  - actionList with the corresponding actions. If you want to add icons to your filtering actions you can do here by construction the QActions accordingly

VsaPlugin::VsaPlugin()
{ 
        typeList << FP_VSA;
  
  foreach(FilterIDType tt , types())
	  actionList << new QAction(filterName(tt), this);
}

VsaPlugin::~VsaPlugin(){
//    typedef RegionGrower<CMeshO>::Pov Pov;
//    CMeshO::PerMeshAttributeHandle<RegionGrower<CMeshO> * > rg_handle;
//    rg_handle = vcg::tri::Allocator<CMeshO>::GetPerMeshAttribute<RegionGrower<CMeshO> *> (md.mm()->cm,"regiongrower");
//    if (vcg::tri::Allocator<CMeshO>::IsValidHandle(md.mm()->cm,rg_handle) && (  rg_handle() != NULL))
//            delete rg_handle();
//
//    CMeshO::PerMeshAttributeHandle<std::vector<Pov> * > povs_handle;
//    povs_handle = vcg::tri::Allocator<CMeshO>::GetPerMeshAttribute<std::vector<Pov> *> (md.mm()->cm,"pointofviews");
//    if (vcg::tri::Allocator<CMeshO>::IsValidHandle(md.mm()->cm,povs_handle) && (  povs_handle() != NULL))
//            delete povs_handle();

}

// ST() must return the very short string describing each filtering action 
// (this string is used also to define the menu entry)
QString VsaPlugin::filterName(FilterIDType filterId) const
{
  switch(filterId) {
                case FP_VSA :  return QString("Compute Variational Shape Approximation");
		default : assert(0); 
	}
}

// Info() must return the longer string describing each filtering action 
// (this string is used in the About plugin dialog)
 QString VsaPlugin::filterInfo(FilterIDType filterId) const
{
  switch(filterId) {
                case FP_VSA :  return QString("Compute Variational Shape Approximation");
		default : assert(0); 
	}
}

// This function define the needed parameters for each filter. Return true if the filter has some parameters
// it is called every time, so you can set the default value of parameters according to the mesh
// For each parmeter you need to define, 
// - the name of the parameter, 
// - the string shown in the dialog 
// - the default value
// - a long string describing the meaning of that parameter (shown as a popup help in the dialog)
void VsaPlugin::initParameterSet(QAction *action,MeshDocument &  m , RichParameterSet & parlst)
{
	 switch(ID(action))	 {
                case FP_VSA :
                  parlst.addParam(new RichInt ("MaxPatches", 10,"maximum number of patches to use"));
                  parlst.addParam(new RichBool ("ComputeSamplingViews", false,"Also compute the view for sampling",""));
                  parlst.addParam(new RichInt ("Width", 1024,"width of the viewport"));
                  parlst.addParam(new RichInt ("Heigth", 512,"height of the viewport"));
                  parlst.addParam(new RichInt ("Resolution", 10000,"Linear resolution: number of samples along the diagonal of the bbox"));
                  break;

   default: break; // do not add any parameter for the other filters
  }
}
int VsaPlugin::getRequirements(QAction *){
    return MeshModel::MM_FACEFACETOPO | MeshModel::MM_FACECOLOR;
}

// The Real Core Function doing the actual mesh processing.
bool VsaPlugin::applyFilter(QAction *filter, MeshDocument &md, RichParameterSet & par, vcg::CallBackPos *cb)
{
    typedef RegionGrower<CMeshO>::Pov Pov;

	switch(ID(filter)) {
                case FP_VSA :
		{
			// to access to the parameters of the filter dialog simply use the getXXXX function of the FilterParameter Class
                        int maxpatches = par.getInt("MaxPatches");

                        CMeshO::PerMeshAttributeHandle<RegionGrower<CMeshO>  > rg_handle;
                        rg_handle = vcg::tri::Allocator<CMeshO>::GetPerMeshAttribute<RegionGrower<CMeshO> > (md.mm()->cm,"regiongrower");
                        if (!vcg::tri::Allocator<CMeshO>::IsValidHandle(md.mm()->cm,rg_handle) )
                            rg_handle = vcg::tri::Allocator<CMeshO>::AddPerMeshAttribute<RegionGrower<CMeshO> > (md.mm()->cm,"regiongrower");

                        rg_handle().Init(md.mm()->cm,maxpatches);
                        rg_handle().Refill();


                        for(int ns = 0; ns < 20; ++ns)
                            rg_handle().GrowStep();

                        for(  RegionGrower<CMeshO>::TriRegIterator ti = rg_handle().regions.begin(); ti != rg_handle().regions.end(); ++ti)
                            for( RegionGrower<CMeshO>::RegionType::FaceIterator fi = (*ti).face.begin(); fi != (*ti).face.end(); ++fi)
                                (*fi)->C() = (*ti).color;

                        Log("Mesh Partitioned in Almost Planar regions");
                        bool alsoviews = par.getBool("ComputeSamplingViews");

                        if(alsoviews){
                            int vpsize[2]; int pps;
                            vpsize[0] = par.getInt("Width");
                            vpsize[1] = par.getInt("Height");
                            pps = par.getInt("Resolution");
                            Log("Computing Views");
                            rg_handle().ComputeShots(vpsize,pps);

                            CMeshO::PerMeshAttributeHandle<std::vector<Pov>  > povs_handle;
                            povs_handle = vcg::tri::Allocator<CMeshO>::GetPerMeshAttribute<std::vector<Pov> > (md.mm()->cm,"pointofviews");
                            if (!vcg::tri::Allocator<CMeshO>::IsValidHandle(md.mm()->cm,povs_handle) )
                                 povs_handle = vcg::tri::Allocator<CMeshO>::AddPerMeshAttribute<std::vector<Pov> > (md.mm()->cm,"pointofviews");

                            rg_handle().GetAllShots(povs_handle());
                        }
				
                } break;
		default: assert (0);
	}
	return true;
}

 VsaPlugin::FilterClass VsaPlugin::getClass(QAction *a)
{
  switch(ID(a))
  {
    case FP_VSA :
      return MeshFilterInterface::FaceColoring ;
    default : 
			return MeshFilterInterface::Generic;
  }
}

Q_EXPORT_PLUGIN(VsaPlugin)