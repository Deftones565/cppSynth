#pragma once

class Envelope {
public:
    Envelope();
    ~Envelope();

    void setAttack(double time);
    void setDecay(double time);
    void setSustain(double level);
    void setRelease(double time);

    double process(double elapsedTime);

private:
    double attackTime;
    double decayTime;
    double sustainLevel;
    double releaseTime;
    double releaseStart;
    bool isReleased;
};

