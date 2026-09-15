#pragma once
#include <cmath>
// Geometry only; no hardware access. Values are output degrees.
namespace RotatorMotion {
inline double wrap(double angle) {
    double value = std::fmod(angle, 360.0);
    return value < 0.0 ? value + 360.0 : value;
}
inline bool absoluteTarget(double current, double wrapped, double minimum, double maximum, double& target,
                           double physicalCurrent, int direction) {
    if(!std::isfinite(current) || !std::isfinite(wrapped) || !std::isfinite(physicalCurrent)) return false;
    wrapped = wrap(wrapped);
    bool found = false;
    double best = 0.0;
    for(int turn = -1; turn <= 1; ++turn) {
        double candidate = wrapped + 360.0 * turn;
        if(candidate < minimum || candidate > maximum) continue;
        double physicalTarget = physicalCurrent + (candidate - current) * direction;
        if(physicalTarget < minimum || physicalTarget > maximum) continue;
        double distance = std::abs(candidate - current);
        if(!found || distance < best) { found = true; best = distance; target = candidate; }
    }
    return found;
}
inline bool relativeTarget(double current, double delta, double minimum, double maximum, double& target,
                           double physicalCurrent, int direction) {
    if(!std::isfinite(current) || !std::isfinite(delta) || !std::isfinite(physicalCurrent)) return false;
    target = current + delta;
    double physicalTarget = physicalCurrent + delta * direction;
    return target >= minimum && target <= maximum &&
           physicalTarget >= minimum && physicalTarget <= maximum;
}
}
