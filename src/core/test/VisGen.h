#ifndef VISGEN_H
#define VISGEN_H

#include <QDataStream>
#include <vector>
#include <complex>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

/**
 * @file VisGen.h
 */

namespace pelican {

/**
 * @class VisGen
 *
 * @brief
 *
 * @details
 */

class VisGen
{
    public:
        VisGen(int argc, char** argv);
        ~VisGen();

        void generate() {
            _generate(_nAnt, _nChan, _nPol);
        }

        void write(const std::string& fileName);

        QDataStream& dataStream();

    private:
        void _generate(int nAnt, int nChan, int nPol);
        void _getCommandLineArgs(int argc, char** argv);

    private:
        typedef std::complex<float> complex_t;
        bool _binary;
        int _nAnt;
        int _nChan;
        int _nPol;
        std::vector<complex_t> _data;
        QDataStream *_stream;
};

} // namespace pelican
#endif // VISGEN_H
