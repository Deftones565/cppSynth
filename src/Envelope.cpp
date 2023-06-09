#include "Envelope.h"

Envelope::Envelope()
    : attackTime(0.0), decayTime(0.0), sustainLevel(1.0), releaseTime(0.0),
      releaseStart(0.0), isReleased(false) {}

Envelope::~Envelope() {}

void Envelope::setAttack(double time) {
    attackTime = time;
}

void Envelope::setDecay(double time) {
    decayTime = time;
}

void Envelope::setSustain(double level) {
    sustainLevel = level;
}

void Envelope::setRelease(double time) {
    releaseTime = time;
}

double Envelope::process(double elapsedTime) {
    if (elapsedTime < attackTime) {
        return elapsedTime / attackTime;
    } else if (elapsedTime < attackTime + decayTime) {
        return 1.0 - (1.0 - sustainLevel) * ((elapsedTime - attackTime) / decayTime);
    } else if (!isReleased) {
        releaseStart = elapsedTime;
        isReleased = true;
    }

    double releaseElapsedTime = elapsedTime - (attackTime + decayTime);
    double releaseValue = sustainLevel;

    if (releaseElapsedTime < releaseTime) {
        double releaseFactor = releaseElapsedTime / releaseTime;
        releaseValue = sustainLevel - sustainLevel * releaseFactor;
    }

    return releaseValue;
}

