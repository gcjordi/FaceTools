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

#ifndef FACE_TOOLS_METRIC_METRIC_H
#define FACE_TOOLS_METRIC_METRIC_H

/**
 * Understands how to perform and record a generic metric calculation.
 */

#include "MetricType.h"
#include "GrowthDataRanker.h"
#include "MetricSet.h"
#include <FaceTools/LndMrk/Landmark.h>

namespace FaceTools { namespace Metric {

class FaceTools_EXPORT Metric
{
public:
    using Ptr = std::shared_ptr<Metric>;
    using CPtr = std::shared_ptr<const Metric>;

    // Load from file. Reads in all statistics and also makes new
    // growth data pairs for single sex / same ethnicity datasets,
    // and then same sex / two different ethnicity groups.
    // On error, return null.
    static Ptr load( const QString& filepath);

    inline int id() const { return _mct->id();}
    inline const QString &name() const { return _name;}
    inline const QString &description() const { return _desc;}
    inline const QString &region() const { return _regn;}

    inline size_t numDecimals() const { return _ndps;}

    inline QString category() const { return _mct->category();}
    inline QString units() const { return _mct->units();}
    inline size_t dims() const { return _mct->dimensions();}
    inline const QString remarks() const { return _mct->remarks();}
    inline const QString typeRemarks() const { return _mct->typeRemarks();}
    inline bool isBilateral() const { return _mct->bilateral();}

    inline Vis::MetricVisualiser *visualiser() const { return _mct->visualiser();}

    // Whether or not this metric should be visible (true by default).
    inline void setVisible( bool v) { _visible = v && _mct->visualiser() != nullptr;}
    inline bool isVisible() const { return _mct && _mct->visualiser() && _visible;}

    // Whether or not measurement is natively in-plane (i.e. without being determined
    // by GrowthData in-plane setting). Note that some metrics are always measured
    // in-plane and can't be set otherwise.
    inline void setInPlane( bool v) { _mct->setInPlane(v);}
    inline bool fixedInPlane() const { return _mct->fixedInPlane();}
    inline bool inPlane() const { return _mct->inPlane() || (_gdRanker.current() && _gdRanker.current()->inPlane());}

    // Returns the ids of the landmarks that this metric uses.
    const IntSet& landmarkIds() const { return _mct->landmarkIds();}

    GrowthDataRanker& growthData() { return _gdRanker;}
    const GrowthDataRanker& growthData() const { return _gdRanker;}

    // Record the measurement for this metric and store in the given model's
    // metric values against its current assessment.
    bool measure( FM*) const;

    // Returns true iff this metric can be measured for the given model's current assessment.
    bool canMeasure( const FM*) const;

    // Purge this metric of any data cached for the given model.
    void purge( const FM*);

private:
    mutable MetricType::Ptr _mct;
    bool _visible;
    size_t _ndps;
    QString _name, _desc, _regn;

    GrowthDataRanker _gdRanker;
    MetricValue _measure( const FM*, bool) const;
    bool _setIfMetricValueChanged( MetricSet&, const MetricValue&) const;

    Metric();
    Metric( const Metric&) = delete;
    void operator=( const Metric&) = delete;
};  // end class

}}   // end namespaces

#endif
