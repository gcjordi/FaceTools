/************************************************************************
 * Copyright (C) 2020 SIS Research Ltd & Richard Palmer
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

#include <Action/ActionBackfaceCulling.h>
#include <LndMrk/LandmarksManager.h>
#include <Vis/FaceView.h>
#include <FaceModel.h>
using FaceTools::Action::ActionBackfaceCulling;
using FaceTools::Action::FaceAction;
using FaceTools::Action::Event;
using FaceTools::Vis::FV;
using MS = FaceTools::Action::ModelSelector;


ActionBackfaceCulling::ActionBackfaceCulling( const QString& dn, const QIcon& ico, const QKeySequence& ks)
    : FaceAction( dn, ico, ks)
{
    setCheckable( true, true);
    addTriggerEvent( Event::LOADED_MODEL);
}   // end ctor


bool ActionBackfaceCulling::update( Event e)
{
    bool rval = false;
    const FV* fv = MS::selectedView();
    if ( fv)
    {
        rval = true;
        if ( !fv->backfaceCulling())
            rval = false;
    }   // end if
    return rval;
}   // end update


bool ActionBackfaceCulling::isAllowed( Event) { return MS::isViewSelected();}   // end isAllowed


void ActionBackfaceCulling::doAction( Event)
{
    const bool ischecked = isChecked();
    const FV *sfv = MS::selectedView();
    for ( FV* fv : sfv->data()->fvs()) // All views of this model
        fv->setBackfaceCulling( ischecked);
}   // end doAction


Event ActionBackfaceCulling::doAfterAction( Event)
{
    return Event::VIEW_CHANGE;
}   // end doAfterAction
