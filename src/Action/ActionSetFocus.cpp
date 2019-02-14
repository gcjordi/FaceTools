/************************************************************************
 * Copyright (C) 2018 Spatial Information Systems Research Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/

#include <ActionSetFocus.h>
#include <FaceModelViewer.h>
#include <FaceView.h>
#include <algorithm>
#include <cassert>
using FaceTools::Action::ActionSetFocus;
using FaceTools::Action::EventSet;
using FaceTools::Action::FaceAction;
using FaceTools::FVS;
using FaceTools::Vis::FV;


// public
ActionSetFocus::ActionSetFocus( const QString& dn, const QIcon& icon)
    : FaceAction( dn, icon)
{
}   // end ctor


bool ActionSetFocus::testEnabled( const QPoint* p) const
{
    bool enabled = false;
    if ( p && ready1())
        enabled = ready1()->isPointOnFace(*p);
    return enabled;
}   // end testEnabled


bool ActionSetFocus::doAction( FVS& fvs, const QPoint& p)
{
    assert(fvs.size() == 1);
    FV* fv = fvs.first();
    assert(fv);
    cv::Vec3f nf;
    fv->projectToSurface( p, nf);
    fv->viewer()->setFocus(nf);
    return true;
}   // end doAction
