#include "pelican/comms/StreamData.h"
#include "pelican/comms/Data.h"
#include <iostream>

#include "pelican/utility/memCheck.h"

namespace pelican {

// class StreamData
StreamData::StreamData(const QString& name, void* data, size_t size)
    : Data(name, data, size)
{
}

StreamData::StreamData(const QString& name, QString& id, QByteArray& d )
    : Data(name, id, d)
{
}

StreamData::StreamData(const QString& name, QString& id, size_t size)
    : Data(name, id, size)
{
}

StreamData::~StreamData()
{
}

const StreamData::DataList_t& StreamData::associateData() const
{
    return _associateData;
}

const QSet<QString>& StreamData::associateDataTypes() const
{
    return _associateDataTypes;
}

void StreamData::addAssociatedData(boost::shared_ptr<Data> data)
{
    Q_ASSERT( data.get() );
    _associateData.append(data);
    _associateDataTypes.insert(data->name());
}

bool StreamData::isValid() const
{
    bool rv = Data::isValid();
    if( rv ) {
        foreach(const boost::shared_ptr<Data>& d, _associateData) {
            rv &= d->isValid();
        }
    }
    return rv;
}

bool StreamData::operator==(const StreamData& sd) const
{
    if( _associateData.size() != sd._associateData.size() )
        return false;
    bool rv = Data::operator==(sd);
    for( int i = 0; i < _associateData.size(); ++i )
    {
        rv &= _associateData[i]->operator==(*(sd._associateData[i]));
    }
    return rv;
}


void StreamData::reset()
{
    _associateData.clear();
}

} // namespace pelican