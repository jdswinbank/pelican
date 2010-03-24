#include <string>
#include "comms/Data.h"
#include "WritableData.h"
#include "AbstractLockableData.h"

#include "utility/memCheck.h"

namespace pelican {

// class WritableData 
WritableData::WritableData(AbstractLockableData* data)
    : _data(data)
{
    if( _data )
        _data->writeLock();
}

WritableData::~WritableData()
{
    if( _data )
        _data->writeUnlock();
}

void WritableData::write(const void* buf, size_t size, size_t offset )
{
    Q_ASSERT( size + offset <= _data->data()->size() );
    void* mem = (char*)(_data->data()->operator*()) + offset;
    std::memcpy( mem, buf , size);
}

} // namespace pelican
