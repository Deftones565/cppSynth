// WaveformWriter.cpp

#include "WaveformWriter.h"

WaveformWriter::WaveformWriter(const std::string& filename) {
    fileStream.open(filename);
    if (!fileStream.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
}

WaveformWriter::~WaveformWriter() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

void WaveformWriter::writeWaveform(const std::vector<double>& waveform) {
    for (const double& value : waveform) {
        fileStream << value << '\n';
    }
}

