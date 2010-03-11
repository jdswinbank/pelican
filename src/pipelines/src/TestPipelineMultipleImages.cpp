#include "pipelines/TestPipelineMultipleImages.h"
#include "modules/ZenithImagerDft.h"
#include "modules/ImageWriterFits.h"
#include "modules/ZenithModelVisibilities.h"
#include "data/ImageData.h"
#include "data/ModelVisibilityData.h"
#include "data/AntennaPositions.h"
#include <iostream>

#include "utility/memCheck.h"

namespace pelican {


/**
 * @details
 */
TestPipelineMultipleImages::TestPipelineMultipleImages()
    : AbstractPipeline()
{
}


/**
 * @details
 */
TestPipelineMultipleImages::~TestPipelineMultipleImages()
{
    delete _imageA;
    delete _imageB;
    delete _modelVis;
}


/**
 * @details
 * Initialises the pipeline.
 */
void TestPipelineMultipleImages::init()
{
    setName("TestPipelineMultipleImages");
    _visModel = (ZenithModelVisibilities*) createModule("ZenithModelVisibilities");
    _imagerA = (ZenithImagerDft*) createModule("ZenithImagerDft", "a");
    _imagerB = (ZenithImagerDft*) createModule("ZenithImagerDft", "b");
    _fitsWriterA = (ImageWriterFits*) createModule("ImageWriterFits", "a");
    _fitsWriterB = (ImageWriterFits*) createModule("ImageWriterFits", "b");

    requestRemoteData("AntennaPositions");

    _modelVis = new ModelVisibilityData;
    _imageA = new ImageData;
    _imageB = new ImageData;
}


/**
 * @details
 * Runs the pipeline iteration.
 */
void TestPipelineMultipleImages::run(QHash<QString, DataBlob*>& remoteData)
{
    AntennaPositions* antPos = (AntennaPositions*) remoteData["AntennaPositions"];
    Q_ASSERT(antPos != NULL);

    _visModel->run(antPos, _modelVis);

    _imagerA->run(_imageA, antPos, _modelVis);
    _imagerB->run(_imageB, antPos);

    _fitsWriterA->run(_imageA);
    _fitsWriterB->run(_imageB);

    stop();
}

} // namespace pelican
