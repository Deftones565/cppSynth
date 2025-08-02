#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <vector>
#include <string>

#define PI 3.14159265358979323846

class Oscillator {
public:
    Oscillator(unsigned tableSize, double sampleRate);

    void setWaveform(const std::string& waveform);
    void setFrequency(double frequency);
    void setNote(int note);
    void setVolume(double volume);

    double getWaveformValue();

private:
    void updateSineWaveTable();
    void updateSquareWaveTable();
    void updateSawtoothWaveTable();
    void updateTriangleWaveTable();
    void updateNoiseWaveTable();

    double noteToFrequency(int note);

    void normalizeAmplitude(std::vector<double>& waveform);
    void applyWindow(std::vector<double>& waveform, const std::vector<double>& window);

    std::vector<double> sineWaveTable;
    std::vector<double> squareWaveTable;
    std::vector<double> sawtoothWaveTable;
    std::vector<double> triangleWaveTable;
    std::vector<double> noiseWaveTable;
    std::vector<double> silentWaveTable;

    std::vector<double>* activeWaveTable;

    double sampleRate;
    double frequency;
    double currentPosition;
    double volume;
};

#endif // OSCILLATOR_H
