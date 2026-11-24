#pragma once
#include <stdint.h>

typedef enum
{
    DOOR_CLOSED = 0,
    DOOR_OPENING = 1,
    DOOR_OPEN = 2,
    DOOR_CLOSING = 3,
    DOOR_STOPPED = 4,
    DOOR_REVERSING = 5,
} DoorMode_te;

typedef struct GarageDoor_s
{
    /* Configuration */
    uint16_t reversePause_ms; /* the motor is off this long before it runs the other way */
    uint16_t maxRunTime_ms;   /* the longest a single travel may take */

    /* Inputs */
    uint8_t button;      /* wall button, 1 while pressed */
    uint8_t obstruction; /* photo-eye beam interrupted */
    uint8_t openLimit;   /* limit switch: door fully open */
    uint8_t closedLimit; /* limit switch: door fully closed */

    /* Outputs */
    int8_t motor; /* +1 up, -1 down, 0 off */
    DoorMode_te mode;

    /* State */
    uint8_t lastButton;
    int8_t lastDirection;
    uint16_t modeTime_ms;
} GarageDoor_ts;

void garageDoorInit(GarageDoor_ts *const ctx, uint16_t reversePause_ms, uint16_t maxRunTime_ms);
void garageDoorCyclic(GarageDoor_ts *const ctx, uint16_t cycle_ms);
