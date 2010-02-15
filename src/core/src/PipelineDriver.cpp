#include <QString>
#include <QtGlobal>
#include <QtDebug>
#include "core/AbstractDataClient.h"
#include "core/AbstractPipeline.h"
#include "core/ModuleFactory.h"
#include "core/PipelineDriver.h"
#include "data/DataBlob.h"
#include "utility/memCheck.h"

namespace pelican {

/**
 * @details
 * PipelineDriver constructor, which takes the command line arguments
 * for initialisation.
 */
PipelineDriver::PipelineDriver(DataBlobFactory* blobFactory)
{
    /* Initialise member variables */
    _run = false;
    _dataClient = NULL;
    _moduleFactory = NULL;
    _blobFactory = blobFactory;
}

/**
 * @details
 * PipelineDriver destructor. This deletes all the registered pipelines and
 * the data blobs.
 */
PipelineDriver::~PipelineDriver()
{
    foreach (AbstractPipeline* pipeline, _registeredPipelines) {
        delete pipeline;
    }

    foreach (DataBlob* dataBlob, _dataHash) {
        delete dataBlob;
    }
    _dataHash.clear();
}

/**
 * @details
 * Public interface to register the pipeline with the driver.
 * Registered pipelines will be deleted when the class is destroyed.
 */
void PipelineDriver::registerPipeline(AbstractPipeline *pipeline)
{
    _registeredPipelines.append(pipeline);
}

/**
 * @details
 * Sets the data client.
 */
void PipelineDriver::setDataClient(AbstractDataClient *client)
{
    _dataClient = client;
}

/**
 * @details
 * Sets the module factory.
 */
void PipelineDriver::setModuleFactory(ModuleFactory *moduleFactory)
{
    _moduleFactory = moduleFactory;
}

/**
 * @details
 * Iterates over all registered pipelines to determine the required data and
 * starts the data flow through the pipelines.
 */
void PipelineDriver::start()
{
    /* Initialise the pipelines */
    _initialisePipelines();

    /* Start the pipeline driver */
    if (_dataClient == NULL)
        throw QString("No data client set.");

    /* Enter main program loop */
    _run = true;
    while (_run) {
        /* Get the data from the client */
        QHash<QString, DataBlob*> data = _dataClient->getData(_requiredData);

        /* Check for empty data */
        if (data.isEmpty())
            throw QString("No data returned from client.");

        /* Run all the pipelines compatible with this data hash */
        QMultiHash<DataRequirements, AbstractPipeline*>::iterator i = _pipelines.begin();
        while (i != _pipelines.end()) {
            if (i.key().isCompatible(data))
                i.value()->run(data);
            ++i;
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
 * This method allocates all the required data blobs and inserts pointers to
 * them into the data hash.
 *
 * @param[in] req All data required by the pipeline.
 */
void PipelineDriver::_createDataBlobs(const DataRequirements& req)
{
    /* Create a union of all data requirements */
    QSet<QString> allData = req.serviceData() & req.streamData();

    /* Iterate over the data requirements to create blobs if they don't exist */
    foreach (QString type, allData) {
        if (  ! _dataHash.contains(type) )
            _dataHash.insert(type, _blobFactory->create(type));
    }
}

/**
 * @details
 * Private method to find the data requirements of the given pipeline.
 * This is called by _initialisePipelines().
 */
void PipelineDriver::_determineDataRequirements(AbstractPipeline* pipeline)
{
    /* Check for an empty pipeline */
    if (pipeline->dataRequired() == DataRequirements())
        throw QString("Empty pipelines are not supported.");

    /* Check that the set of stream data required for this pipeline does not
     * intersect the set of stream data required by another.
     * Data is not currently copied, so this ensures that two pipelines do not
     * try to modify the same data. */
    QMultiHash<DataRequirements, AbstractPipeline*>::iterator i = _pipelines.begin();
    while (i != _pipelines.end()) {
        if ((i.key().streamData() & pipeline->dataRequired().streamData()).size() > 0) {
            throw QString("Multiple pipelines requiring the same stream data are not currently supported.");
        }
        ++i;
    }

    _pipelines.insert(pipeline->dataRequired(), pipeline);
    _requiredData += pipeline->dataRequired();
}

/**
 * @details
 * Private method to initialises the pipelines by iterating over them,
 * creating their modules and finding out their data requirements.
 * This is called by start().
 */
void PipelineDriver::_initialisePipelines()
{
    /* Check for at least one registered pipeline */
    if (_registeredPipelines.isEmpty())
        throw QString("No pipelines.");

    foreach (AbstractPipeline* pipeline, _registeredPipelines) {
        /* Set the module factory and pipeline driver */
        pipeline->setModuleFactory(_moduleFactory);
        pipeline->setPipelineDriver(this);

        /* Initialise the pipeline (create modules) */
        pipeline->init();

        /* Add the required data to the data requirements */
        _determineDataRequirements(pipeline);
    }
}

} // namespace pelican
