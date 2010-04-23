#ifndef TIMESTREAMDATA_H_
#define TIMESTREAMDATA_H_

/**
 * @file TimeStreamData.h
 */

#include "pelican/data/DataBlob.h"
#include <vector>
#include <complex>

namespace pelican {

/**
 * @class TimeStreamData
 *
 * @brief
 * Container class to hold a buffer of time stream data.
 *
 * @details
 * Used for time domain processing application such as channeliser modules.
 *
 * Multidimensional data holding a time series for a number of sub-bands
 * and polarisations.
 *
 * Data is held in a vector ordered by
 *
 */

template <class T>
class T_TimeStreamData : public DataBlob
{
    public:
        /// Constructs an empty time stream data blob.
        T_TimeStreamData() : DataBlob() {
            _nSubbands = 0;
            _nPolarisations = 0;
            _nSamples = 0;
            _startTime = 0.0;
            _sampleDelta = 0.0;
        }

        /// Constructs and assigns memory for a time stream buffer data blob.
        T_TimeStreamData(const unsigned nSubbands, const unsigned nPolarisations,
                const unsigned nSamples) : DataBlob()
        {
            resize(_nSubbands, _nPolarisations, _nSamples);
        }

        /// Destroys the time stream data blob.
        virtual ~T_TimeStreamData() {}

    public:
        /// Clears the time stream data.
        void clear()
        {
            _data.clear();
            _nSubbands = 0;
            _nPolarisations = 0;
            _nSamples = 0;
            _startTime = 0.0;
            _sampleDelta = 0.0;
        }

        /// Assign memory for the time stream data blob.
        void resize(const unsigned nSubbands, const unsigned nPolarisations,
                const unsigned nSamples)
        {
            _nSubbands = nSubbands;
            _nPolarisations = nPolarisations;
            _nSamples = nSamples;
            _data.resize(_nSubbands * _nPolarisations * _nSamples);
        }

    public: // accessor methods
        /// Returns the number of entries in the data blob.
        unsigned size() { return _data.size(); }

        /// Returns a pointer to the time stream data.
        T* data() { return _data.size() > 0 ? &_data[0] : NULL; }

        /// Returns a pointer to the time stream data (const overload).
        const T* data() const  { return _data.size() > 0 ? &_data[0] : NULL; }

        /// Returns the number of sub-bands in the data.
        unsigned nSubbands() const { return _nSubbands; }

        /// Returns the number of polarisations in the data.
        unsigned nPolarisations() const { return _nPolarisations; }

        /// Returns the number of polarisations in the data.
        unsigned nSamples() const { return _nSamples; }

        /// Returns the start time of the data.
        double startTime() const { return _startTime; }

        /// Sets the start time of the data.
        void setStartTime(const double value) { _startTime = value; }

        /// Returns the sample delta.
        double sampleDelta() const { return _sampleDelta; }

        /// Sets the time interval between samples.
        void setSampleDelta(const double value) { _sampleDelta = value; }

    private:
        std::vector<T> _data;
        unsigned _nSubbands;
        unsigned _nPolarisations;
        unsigned _nSamples;
        double _startTime;
        double _sampleDelta;
};


/**
 * @class TimeStreaDataDbl
 *
 * @brief
 *
 *
 * @details
 */
class TimeStreamData : public T_TimeStreamData<std::complex<double> >
{
    public:
        /// Constructs an empty time stream data blob.
        TimeStreamData() : T_TimeStreamData<std::complex<double> >() {}

        /// Constructs and assigns memory for a time stream buffer data blob.
        TimeStreamData(const unsigned nTimeSamples) :
            T_TimeStreamData(nTimeSamples) {}

        /// Destroys the time stream data blob.
        ~TimeStreamData() {}
};


}// namespace pelican

#endif // TIMESTREAMDATA_H_
