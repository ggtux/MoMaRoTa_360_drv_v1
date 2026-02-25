# MoMaRoTo_360_drv_v1 (English)

## Changes compared to MoMaRota_drv_v1

This version enables **full 360° rotation** of the rotator.

### Technical Details

- **Gear Ratio**: 1:2 (unchanged)
- **Rotator Range**: 0-360° (previously: 0-180°)
- **Motor Rotation**: 720° (2 full turns)
- **Formula**: `motorSteps = (gearDegrees * 2.0 / 360.0) * 4096`

### Changed Files

#### src/servo_control.cpp
- **Removed**: The limitation to 180° rotator rotation
- **Effect**: All values from 0-360° are directly mapped to motor steps
- Previously: Angles > 180° were mapped back to 0-180°
- Now: Full 0-360° range available
- **Position Tracking**: Absolute position is tracked by movement commands
- **Live Display**: During movement, the current position is calculated from `absolutePosition - posRead`
- **Motor-Mode 3**: Uses relative movements with accumulated absolute position

### Functionality

#### Position Tracking System

In Motor-Mode 3, the motor works with relative movement commands:
- `absolutePosition`: Accumulated absolute target position in steps
- `posRead`: Remaining distance to the target (reduced to 0 during movement)
- **Current Position**: `currentPos = absolutePosition - posRead`

This enables:
- Live display of the position during movement
- Correct position display without resetting to zero
- Working reverse mode in both directions
- Shortest-path calculation for optimal movements

- **0° Rotator** = 0° Motor (Step 0)
- **90° Rotator** = 180° Motor (Step 2048)
- **180° Rotator** = 360° Motor (Step 4096) = 1 full motor turn
- **270° Rotator** = 540° Motor (Step 6144)
- **360° Rotator** = 720° Motor (Step 8192) = 2 full motor turns

All intermediate steps are possible!

#### Reverse Mode

The reverse mode only inverts the physical movement direction of the motor:
- `logicalDelta`: Logical movement for position tracking (remains the same)
- `motorDelta`: Physical motor movement (is inverted in reverse mode)
- The display remains consistent regardless of reverse status

### Compatibility

- All other functions remain unchanged
- Web interface works with the full 0-360° range
- ASCOM Alpaca API supports full 360° rotation
- Calibration and zero-point functions unchanged
