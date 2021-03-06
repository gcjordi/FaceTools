/************************************************************************
 * Copyright (C) 2018 SIS Research Ltd & Richard Palmer
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

#include <Action/ActionToggleStereoRendering.h>
#include <FaceModelViewer.h>
#include <vtkRenderWindow.h>
using FaceTools::Action::ActionToggleStereoRendering;
using FaceTools::Action::FaceAction;
using FaceTools::Action::Event;
using FaceTools::FMV;


namespace {
void setStereoRendering( FMV* v, bool enable)
{
    vtkRenderWindow* rwin = v->getRenderWindow();
    rwin->SetStereoRender(enable);
}   // end setStereoRendering
}   // end namespace


ActionToggleStereoRendering::ActionToggleStereoRendering( const QString& dn, const QIcon& ico)
    : FaceAction( dn, ico)
{
    setCheckable( true, false);
    doAction( Event::NONE);
}   // end ctor


void ActionToggleStereoRendering::doAction( Event)
{
    for ( FMV* v : ModelSelector::viewers())
        setStereoRendering( v, isChecked());
}   // end doAction
