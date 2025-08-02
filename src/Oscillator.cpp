// Include necessary header files
#include "Oscillator.h"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <random>

// Define maximum number of harmonics for waveforms
#define MAX_HARMONICS 500

// Oscillator constructor
Oscillator::Oscillator(unsigned tableSize, double sampleRate)
    : sampleRate(sampleRate), frequency(0.0), currentPosition(0.0), volume(1.0) 
{
    // Resize wave tables based on input tableSize
    sineWaveTable.resize(tableSize);
    squareWaveTable.resize(tableSize);
    sawtoothWaveTable.resize(tableSize);
    triangleWaveTable.resize(tableSize);
    noiseWaveTable.resize(tableSize);
    silentWaveTable.resize(tableSize, 0.0); // Silent wave is just zeros

    frequency = 440.0;  // Set the default frequency 

    // Populate all the wave tables
    updateSineWaveTable();
    updateSquareWaveTable();
    updateSawtoothWaveTable();
    updateTriangleWaveTable();
    updateNoiseWaveTable();

    activeWaveTable = &sineWaveTable;  // Default wave form is sine
    currentPosition = 0.0;  // Reset current position
}

// Function to generate a sine waveform
void Oscillator::updateSineWaveTable() {
    for (unsigned i = 0; i < sineWaveTable.size(); ++i) {
        double angle = (double)i / (double)sineWaveTable.size();
        sineWaveTable[i] = sin(2.0 * PI * angle);
    }
}

// Function to generate a triangle waveform
void Oscillator::updateTriangleWaveTable() {
    for (unsigned i = 0; i < triangleWaveTable.size(); ++i) {
        // Harmonic synthesis
        double angle = (double)i / (double)triangleWaveTable.size();
        double value = 0.0;
        double sign = 1.0;
        for (unsigned h = 1; h <= MAX_HARMONICS; h += 2) {
            if (h * frequency > sampleRate / 2) {
                break;
            }
            value += sign * (1.0 / (h * h)) * sin(2.0 * PI * h * angle);
            sign *= -1.0; // Flip the sign
        }
        triangleWaveTable[i] = value * (8.0 / (PI * PI));
    }
}

// Function to generate white noise values
void Oscillator::updateNoiseWaveTable() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    for (double &sample : noiseWaveTable) {
        sample = dist(gen);
    }
}

// Function to set the waveform type
void Oscillator::setWaveform(const std::string& waveform) {
    // Switch the active waveform based on input string
    if (waveform == "sine") {
        activeWaveTable = &sineWaveTable;
    }
    else if (waveform == "square") {
        activeWaveTable = &squareWaveTable;
    }
    else if (waveform == "sawtooth") {
        activeWaveTable = &sawtoothWaveTable;
    }
    else if (waveform == "triangle") {
        activeWaveTable = &triangleWaveTable;
    }
    else if (waveform == "noise") {
        activeWaveTable = &noiseWaveTable;
    }
    else if (waveform == "none") {
        activeWaveTable = &silentWaveTable;
    }
    else {
        std::cout << "Invalid waveform. Defaulting to silent." << std::endl;
        activeWaveTable = &silentWaveTable;
    }
}

// Function to set the frequency and update all the wave tables
void Oscillator::setFrequency(double frequency) {
    this->frequency = frequency;
    updateSquareWaveTable();
    normalizeAmplitude(squareWaveTable);
    updateSawtoothWaveTable();
    normalizeAmplitude(sawtoothWaveTable);
    updateTriangleWaveTable();
    normalizeAmplitude(triangleWaveTable);
    updateSineWaveTable();
}

// Function to generate a square waveform
void Oscillator::updateSquareWaveTable() {
    for (unsigned i = 0; i < squareWaveTable.size(); ++i) {
        // Harmonic synthesis
        double angle = (double)i / (double)squareWaveTable.size();
        double value = 0.0;
        // Square waves are comprised of the odd harmonics only
        for (unsigned h = 1; h <= sampleRate / (2 * frequency); h += 2) {
            value += sin(2.0 * PI * h * angle) / h;
        }
        squareWaveTable[i] = value * (4.0 / PI);
    }
}

// Function to generate a sawtooth waveform
void Oscillator::updateSawtoothWaveTable() {
    for (unsigned i = 0; i < sawtoothWaveTable.size(); ++i) {
        // Harmonic synthesis
        double angle = (double)i / (double)sawtoothWaveTable.size();
        double value = 0.0;
        for (unsigned h = 1; h <= sampleRate / (2 * frequency); ++h) {
            value += (h % 2 == 0) ? -sin(2.0 * PI * h * angle) / h : sin(2.0 * PI * h * angle) / h;
        }
        sawtoothWaveTable[i] = value * (2.0 / PI);
    }
}

// Function to convert a MIDI note to frequency
double Oscillator::noteToFrequency(int note) {
    // Use the standard MIDI to frequency formula
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

// Function to set the frequency based on MIDI note
void Oscillator::setNote(int note) {
    setFrequency(noteToFrequency(note));
}

// Function to set the volume
void Oscillator::setVolume(double volume) {
	this->volume = volume;
}

// Function to generate the next value in the waveform
double Oscillator::getWaveformValue() {
    // Update position in the wave
    double advance = frequency / sampleRate;
    currentPosition = fmod(currentPosition + advance, 1.0);

    // Interpolate to get a more accurate waveform value
    double position = currentPosition * activeWaveTable->size();
    unsigned index = (unsigned)position;
    double fraction = position - index;

    unsigned nextIndex = (index + 1) % activeWaveTable->size();
    unsigned nextNextIndex = (index + 2) % activeWaveTable->size();
    unsigned prevIndex = (index - 1 + activeWaveTable->size()) % activeWaveTable->size();

    // Cubic interpolation
    double value0 = (*activeWaveTable)[prevIndex];
    double value1 = (*activeWaveTable)[index];
    double value2 = (*activeWaveTable)[nextIndex];
    double value3 = (*activeWaveTable)[nextNextIndex];

    double P = (value3 - value2) - (value0 - value1);
    double Q = (value0 - value1) - P;
    double R = value2 - value0;
    double S = value1;

    double interpolatedValue = P*fraction*fraction*fraction + Q*fraction*fraction + R*fraction + S;

    return interpolatedValue * volume;
}

// Function to normalize the amplitude of the waveform to 1
void Oscillator::normalizeAmplitude(std::vector<double>& waveform) {
    // Determine the largest magnitude sample
    double maxAmplitude = 0.0;
    for (const double& sample : waveform) {
        maxAmplitude = std::max(maxAmplitude, std::abs(sample));
    }

    // Avoid division by zero for silent waveforms
    if (maxAmplitude > 0.0) {
        double scalingFactor = 1.0 / maxAmplitude;
        for (double& sample : waveform) {
            sample *= scalingFactor;
        }
    }
}

// Function to apply a window to the waveform
void Oscillator::applyWindow(std::vector<double>& waveform, const std::vector<double>& window) {
    // Throw error if sizes don't match
    if (waveform.size() != window.size()) {
        throw std::runtime_error("Waveform and window size mismatch.");
    }

    // Apply window to waveform
    for (unsigned i = 0; i < waveform.size(); ++i) {
        waveform[i] *= window[i];
    }
}

