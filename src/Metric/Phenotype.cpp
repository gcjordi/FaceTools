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

#include <Metric/Phenotype.h>
#include <Metric/MetricManager.h>
#include <Ethnicities.h>
#include <FaceModel.h>
using FaceTools::Metric::Phenotype;
using FaceTools::Metric::MetricSet;
using FaceTools::Metric::MetricValue;
using FaceTools::Metric::GrowthData;
using MM = FaceTools::Metric::MetricManager;
using FaceTools::FM;


// private
Phenotype::Phenotype() : _id(-1)
{
    // Register MetricSet for Phenotype determination function:
    _lua.new_usertype<MetricSet>( "MetricSet",
                                  "metric", &MetricSet::metric);

    _lua.new_usertype<MetricValue>( "MetricValue",
                                    "ndims", &MetricValue::ndims,
                                    "mean", &MetricValue::mean,
                                    "value", &MetricValue::value,
                                    "zscore", &MetricValue::zscore);
}   // end ctor


// public static
Phenotype::Ptr Phenotype::create() { return Ptr( new Phenotype, [](Phenotype *d){ delete d;});}


// public static
Phenotype::Ptr Phenotype::load( const QString& fpath)
{
    bool loadedOk = false;
    Ptr hpo = create();
    hpo->_lua.open_libraries( sol::lib::base);
    try
    {
        hpo->_lua.script_file( fpath.toStdString());
        loadedOk = true;
    }   // end try
    catch ( const sol::error& e)
    {
        std::cerr << "[WARN] FaceTools::Metric::Phenotype::load: Unable to load and execute file '" << fpath.toStdString() << "'!" << std::endl;
        std::cerr << "\t" << e.what() << std::endl;
    }   // end catch

    if ( !loadedOk)
        return nullptr;

    auto table = hpo->_lua["hpo"];
    if ( !table.valid())
    {
        std::cerr << "[WARN] FaceTools::Metric::Phenotype::load: Missing table 'hpo'!" << std::endl;
        return nullptr;
    }   // end if

    hpo->_id = table["id"].get_or(-1);
    if ( hpo->_id < 0)
    {
        std::cerr << "[WARN] FaceTools::Metric::Phenotype::load: Invalid ID!" << std::endl;
        return nullptr;
    }   // end if

    if ( sol::optional<std::string> v = table["name"]) hpo->_name = v.value().c_str();
    if ( sol::optional<std::string> v = table["region"]) hpo->_region = v.value().c_str();
    if ( sol::optional<std::string> v = table["ocrit"]) hpo->_ocriteria = v.value().c_str();
    if ( sol::optional<std::string> v = table["scrit"]) hpo->_scriteria = v.value().c_str();
    if ( sol::optional<std::string> v = table["remarks"]) hpo->_remarks = v.value().c_str();

    if ( sol::optional<sol::table> v = table["synonyms"])
    {
        sol::table synonyms = v.value();
        for ( size_t i = 1; i <= synonyms.size(); ++i)
            hpo->_synonyms.append( synonyms[i].get_or_create<std::string>().c_str());
    }   // end if

    if ( sol::optional<sol::table> v = table["refs"])
    {
        sol::table refs = v.value();
        for ( size_t i = 1; i <= refs.size(); ++i)
            hpo->_refs.append( refs[i].get_or_create<std::string>().c_str());
    }   // end if

    if ( sol::optional<sol::table> v = table["metrics"])
    {
        sol::table metrics = v.value();
        for ( size_t i = 1; i <= metrics.size(); ++i)
            hpo->_metrics.insert( metrics[i].get_or(-1));
    }   // end if

    if ( sol::optional<sol::function> v = table["determine"])
        hpo->_determine = v.value();

    return hpo;
}   // end load


bool Phenotype::_hasMeasurements( const FM* fm, int aid) const
{
    FaceAssessment::CPtr ass = aid < 0 ? fm->currentAssessment() : fm->assessment(aid);
    const MetricSet& mlat = ass->cmetrics(MID);
    const MetricSet& llat = ass->cmetrics(LEFT);
    const MetricSet& rlat = ass->cmetrics(RIGHT);

    // Only evaluate if all the measurements are available for this indication
    for ( int mid : _metrics)
    {
        if ( !MM::metric(mid))
            return false;

        if ( MM::metric(mid)->isBilateral())
        {
            if ( !llat.has(mid) || !rlat.has(mid))
                return false;
        }   // end if
        else
        {
            if ( !mlat.has(mid))
                return false;
        }   // end else
    }   // end for

    return true;
}   // end _hasMeasurements


bool Phenotype::isPresent( const FM* fm, int aid) const
{
    if ( !_determine.valid())
        return false;

    if ( !_hasMeasurements(fm, aid))
        return false;

    bool present = false;
    try
    {
        FaceAssessment::CPtr ass = aid < 0 ? fm->currentAssessment() : fm->assessment(aid);
        const MetricSet& mlat = ass->cmetrics(MID);
        const MetricSet& llat = ass->cmetrics(LEFT);
        const MetricSet& rlat = ass->cmetrics(RIGHT);
        sol::function_result result = _determine( fm->age(), mlat, llat, rlat);
        if ( result.valid())
            present = result;
        else
            std::cerr << "[WARN] FaceTools::Metric::Phenotype::isPresent: Invalid result from function!" << std::endl;
    }   // end try
    catch (const sol::error& e)
    {
        std::cerr << "[WARN] FaceTools::Metric::Phenotype::isPresent: Error in determination script!" << std::endl;
        std::cerr << "\t" << e.what() << std::endl;
    }   // end catch

    /*
    if ( present)
        std::cerr << QString( "%1 (%2) is present").arg( name()).arg( id()).toStdString() << std::endl;
    */
    return present;
}   // end isPresent


QString Phenotype::metricsList() const
{
    const IntSet &mids = metrics();
    std::list<int> lst( mids.begin(), mids.end());
    lst.sort();
    QStringList smids;
    for ( int mid : lst)
        smids << QString("%1").arg(mid);
    return smids.join(",");
}   // end metricsList


bool Phenotype::isSexMatch( int8_t sex) const
{
    if ( sex == UNKNOWN_SEX)
        return false;
    for ( int mid : _metrics)
    {
        MC::Ptr mc = MM::metric(mid);
        const GrowthData *gd = mc->growthData().current();  // May be null
        if ( !gd || ( gd->sex() != sex && gd->sex() != (FEMALE_SEX | MALE_SEX)))
            return false;
    }   // end for
    return true;
}   // end isSexMatch


bool Phenotype::isAgeMatch( float age) const
{
    for ( int mid : _metrics)
    {
        MC::Ptr mc = MM::metric(mid);
        const GrowthData *gd = mc->growthData().current();  // May be null
        if ( !gd || !gd->isWithinAgeRange(age))
            return false;
    }   // end for
    return true;
}   // end isAgeMatch


bool Phenotype::isEthnicityMatch( int ethn) const
{
    for ( int mid : _metrics)
    {
        MC::Ptr mc = MM::metric(mid);
        const GrowthData *gd = mc->growthData().current();  // May be null
        if ( !gd || !Ethnicities::belongs( gd->ethnicity(), ethn))
            return false;
    }   // end for
    return true;
}   // end isEthnicityMatch
