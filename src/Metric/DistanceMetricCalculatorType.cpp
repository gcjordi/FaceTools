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

#include <DistanceMetricCalculatorType.h>
#include <FaceModel.h>
#include <algorithm>
#include <sstream>
using FaceTools::Metric::DistanceMetricCalculatorType;
using FaceTools::Metric::MCT;
using FaceTools::Landmark::LandmarkSet;
using FaceTools::Landmark::LmkList;
using FaceTools::FM;


DistanceMetricCalculatorType::DistanceMetricCalculatorType( int id, const LmkList* l0, const LmkList* l1)
    : _vis(id, l0, l1) {}


MCT* DistanceMetricCalculatorType::make( int id, const LmkList* l0, const LmkList* l1) const
{
    return new DistanceMetricCalculatorType(id, l0, l1);
}   // end make


bool DistanceMetricCalculatorType::canCalculate( const FM* fm, const LmkList* ll) const
{
    using SLmk = FaceTools::Landmark::SpecificLandmark;
    LandmarkSet::Ptr lmks = fm->landmarks();
    if ( ll->size() != 2)
        return false;
    return std::all_of( std::begin(*ll), std::end(*ll), [lmks]( const SLmk& p){ return lmks->has(p);});
}   // end canCalculate


void DistanceMetricCalculatorType::measure( std::vector<double>& dvals, const FM* fm, const LmkList* ll) const
{
    assert( canCalculate(fm, ll));
    LandmarkSet::Ptr lmks = fm->landmarks();
    dvals.resize(1);
    dvals[0] = cv::norm( *lmks->pos(ll->front()) - *lmks->pos(ll->back()));
}   // end measure