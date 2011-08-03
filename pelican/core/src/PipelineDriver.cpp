#include "pelican/core/PipelineDriver.h"

#include "pelican/core/DataClientFactory.h"
#include "pelican/core/AbstractDataClient.h"
#include "pelican/core/FileDataClient.h"
#include "pelican/core/AbstractPipeline.h"
#include "pelican/data/DataBlob.h"
#include "pelican/utility/Config.h"
#include "pelican/utility/ConfigNode.h"
#include "pelican/core/PipelineSwitcher.h"

#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtCore/QtDebug>
#include <iostream>


namespace pelican {

/**
 * @details
 * PipelineDriver constructor, which takes pointers to the allocated factories.
 */
PipelineDriver::PipelineDriver(FactoryGeneric<DataBlob>* blobFactory,
        FactoryConfig<AbstractModule>* moduleFactory,
        DataClientFactory* clientFactory, OutputStreamManager* osmanager)
    : _osmanager(osmanager)
{
    // Initialise member variables.
    _run = false;
    _dataClient = NULL;

    // Store pointers to factories.
    _blobFactory = blobFactory;
    _moduleFactory = moduleFactory;
    _clientFactory = clientFactory;
    Q_ASSERT(_blobFactory != 0 );
    Q_ASSERT(_moduleFactory != 0 );
    Q_ASSERT(_clientFactory != 0 );
}

/**
 * @details
 * PipelineDriver destructor. This deletes all the registered pipelines.
 */
PipelineDriver::~PipelineDriver()
{
    // Delete the pipelines.
    foreach (AbstractPipeline* pipeline, _registeredPipelines) {
        delete pipeline;
    }
    _registeredPipelines.clear();
}

/**
 * @details
 * Public interface to register the given \p pipeline with the driver.
 * The pipeline is initialised when this method is called.
 *
 * Registered pipelines will be deleted when the class is destroyed.
 *
 * @param[in] pipeline Pointer to the allocated pipeline.
 */
void PipelineDriver::registerPipeline(AbstractPipeline *pipeline)
{
    _registerPipeline(pipeline);
    _activatePipeline(pipeline);
}

void PipelineDriver::_registerPipeline(AbstractPipeline *pipeline)
{
    // Check the pipeline exists.
    if (!pipeline)
        throw QString("PipelineDriver::registerPipeline(): Null pipeline.");

    // Store the pointer to the registered pipeline.
    _registeredPipelines.append(pipeline);

    // Set up and initialise the pipeline.
    pipeline->setBlobFactory(_blobFactory);
    pipeline->setModuleFactory(_moduleFactory);
    pipeline->setPipelineDriver(this);
    pipeline->setOutputStreamManager(_osmanager);
    pipeline->init();

    // Store the remote data requirements.
    _allDataReq.append(pipeline->requiredDataRemote());
}

void PipelineDriver::_activatePipeline(AbstractPipeline *pipeline)
{
     if( pipeline ) {
        foreach ( const QString type, pipeline->requiredDataRemote().allData() ) {
            if ( ! _dataHash.contains(type) )
                _dataHash.insert(type, _blobFactory->create(type));
        }
        _pipelines.insert(pipeline->requiredDataRemote(), pipeline);
     }
}


void PipelineDriver::deactivatePipeline(AbstractPipeline *pipeline)
{
    if( pipeline ) {
        // queue the pipeline to be deactivated when it is safe to do so
        _deactivateQueue.append(pipeline);
    }
}

void PipelineDriver::_deactivatePipeline(AbstractPipeline *pipeline)
{
    if( pipeline ) {
        _pipelines.remove(pipeline->requiredDataRemote(),pipeline);
         // if the pipeline is in a switcher then get the next one
         if( _switcherMap.contains(pipeline) ) {
             AbstractPipeline* next = _switcherMap[pipeline]->next();
             // mark the next pipeline against the switcher
             if( next != pipeline ) {
                 _switcherMap[next] = _switcherMap[pipeline];
                 _switcherMap.remove(pipeline);
             }
             // now activate the new pipeline
             _activatePipeline(next);
         }
     }
}

void PipelineDriver::addPipelineSwitcher(const PipelineSwitcher& switcher)
{
    _switchers.push_back(switcher);
     PipelineSwitcher* sw = &_switchers.last(); // pointer to local switcher copy

     AbstractPipeline* next = sw->next();
     DataRequirements reqs = next->requiredDataRemote();

     // register all the pipelines in the switcher
     foreach( AbstractPipeline* pipe, switcher.pipelines() ) {
        if( pipe->requiredDataRemote() != reqs )
                throw( QString("PipelineDriver: Pipelines with different Data requirements in"
                               " the same switcher is not supported") );
        _registerPipeline(pipe);
     }

     // activate the first
     _switcherMap[next] = sw;
     _activatePipeline(next);
}

/**
 * @details
 * Sets the given data client based on the named argument.
 * The client is only created after the pipelines have been initialised,
 * when start() is called.
 *
 * @param[in] name The type of the data client to create.
 */
void PipelineDriver::setDataClient(QString name)
{
    _dataClientName = name;
}

/**
 * @details
 * Iterates over all registered pipelines to determine the required data and
 * starts the data flow through the pipelines.
 *
 * This public method is called by PipelineApplication start().
 */
void PipelineDriver::start()
{
    // Check for at least one registered pipeline.
    if (_registeredPipelines.isEmpty())
        throw QString("PipelineDriver::start(): No pipelines registered.");

    // Store the remote data requirements for each pipeline.
    //foreach (AbstractPipeline* pipeline, _registeredPipelines) {
    //    _pipelines.insert(pipeline->requiredDataRemote(), pipeline);
    //}

    // Create data blobs for all pipelines.
    //foreach ( DataRequirements req, _allDataReq ) {
    //    foreach ( QString type, req.allData() ) {
    //        if ( ! _dataHash.contains(type) )
    //            _dataHash.insert(type, _blobFactory->create(type));
    //    }
    //}

    // Create the data client.
    if (!_dataClientName.isEmpty() && _dataClient == NULL )
        _dataClient = _clientFactory->create(_dataClientName, _allDataReq);

    // Check the data requirements.
    _checkDataRequirements();

    // Enter main program loop.
    _run = true;
    QString lastError;
    while (_run) {
        // Get the data from the client.
        QHash<QString, DataBlob*> validData;
        try {
            if (_dataClient)
                validData = _dataClient->getData(_dataHash);
        }
        catch(QString& e)
        {
            // log the error and keep going
            if( e != lastError )
                std::cerr << e.toStdString() << std::endl;
            lastError = e;
            continue;
        }
        lastError = "";

        // Run all the pipelines compatible with this data hash.
        bool ranPipeline = false;
        QMultiHash<DataRequirements, AbstractPipeline*>::iterator pipe;
        for (pipe = _pipelines.begin(); pipe != _pipelines.end(); ++pipe) {
            if (pipe.key().isCompatible(validData)) {
                ranPipeline = true;
                pipe.value()->run(_dataHash);
            }
        }

        //deactivate any pipelines
        while( _deactivateQueue.size() > 0 ) {
             _deactivatePipeline(_deactivateQueue[0]);
             _deactivateQueue.pop_front();
        }

        // Check if no pipelines were run.
        if (!ranPipeline) {
            QString msg;
            foreach( const QString& d, validData.keys() ) {
                msg += " " + d;
            }
            throw QString("PipelineDriver::start(): received data incompatible with the pipelines:"
                                + msg );
        }
    }
}

/**
 * @details
 * Stops the pipeline driver.
 * This method should be called by a running pipeline.
 */
void PipelineDriver::stop()
{
    _run = false;
}

/**
 * @details
 * Private method to find the data requirements of the given pipeline.
 * This is called by start().
 */
void PipelineDriver::_checkDataRequirements()
{
    if (!_dataClient)
        return;

    /* Check that the set of stream data required for each pipeline does not
     * intersect the set of stream data required by another.
     * Data is not currently copied, so this ensures that two pipelines do not
     * try to modify the same data. */
    DataRequirements totalReq;
    foreach (DataRequirements req, _dataClient->dataRequirements()) {
#ifdef BROKEN_QT_SET_HEADER
        QSet<QString> temp = totalReq.streamData();
        if ((temp & req.streamData()).size() > 0) {
#else
        if ((totalReq.streamData() & req.streamData()).size() > 0) {
#endif
            throw QString("Multiple pipelines requiring the same remote stream data are not supported.");
        }
        totalReq += req;
    }
}

} // namespace pelican
