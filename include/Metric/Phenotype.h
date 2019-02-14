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

#ifndef FACE_TOOLS_METRIC_PHENOTYPE_H
#define FACE_TOOLS_METRIC_PHENOTYPE_H

#include <FaceTypes.h>
#include "MetricSet.h"
#include <sol.hpp>

namespace FaceTools { namespace Metric {

class FaceTools_EXPORT Phenotype
{
public:
    using Ptr = std::shared_ptr<Phenotype>;

    // Load from lua script returning null on error.
    static Ptr load( const QString&);

    // Create a new empty Phenotype object.
    static Ptr create();

    void setId( int id) { _id = id;}
    int id() const { return _id;}

    void setName( const QString& nm) { _name = nm;}
    const QString& name() const { return _name;}

    void setRegion( const QString& r) { _region = r;}
    const QString& region() const { return _region;}

    void setSynonyms( const QStringList& sl) { _synonyms = sl;}
    const QStringList& synonyms() const { return _synonyms;}

    void setCriteria( const QString& c) { _criteria = c;}
    const QString& criteria() const { return _criteria;}

    void setRemarks( const QString& r) { _remarks = r;}
    const QString& remarks() const { return _remarks;}

    // Returns the set of metrics (ids) associated with this HPO term.
    const IntSet& metrics() const { return _metrics;}
    void addMetric( int id) { _metrics.insert(id);}
    void removeMetric( int id) { _metrics.erase(id);}

    // Check if this HPO term is present. Pass in metric sets for the front (non-bilateral),
    // left and right laterals. Metrics that are not bilateral are in the f set while bilateral
    // metrics appear in the left and right sets (llat and rlat).
    bool isPresent( const MetricSet* f, const MetricSet* llat, const MetricSet* rlat) const;

    // Returns true iff the associated metrics have data about the provided model that
    // would allow the isPresent function to return true in principle if the associated
    // measurements meet the criteria for this phenotype.
    bool isDemographicMatch( const FM*) const;

    ~Phenotype();
private:
    int _id;
    QString _name;
    QString _region;
    QStringList _synonyms;
    QString _criteria;
    QString _remarks;
    IntSet _metrics;
    sol::state _lua;
    sol::function _determine;

    Phenotype();
    Phenotype( const Phenotype&) = delete;
    Phenotype& operator=( const Phenotype&) = delete;
};  // end class

}}   // end namespaces

#endif
