#include "garageDoor.h"

static void enter(GarageDoor_ts *const ctx, DoorMode_te mode)
{
    ctx->mode = mode;
    ctx->modeTime_ms = 0;
    if (mode == DOOR_OPENING)
        ctx->lastDirection = 1;
    else if (mode == DOOR_CLOSING)
        ctx->lastDirection = -1;
}

void garageDoorInit(GarageDoor_ts *const ctx, uint16_t reversePause_ms, uint16_t maxRunTime_ms)
{
    ctx->reversePause_ms = reversePause_ms;
    ctx->maxRunTime_ms = maxRunTime_ms;
    ctx->button = 0;
    ctx->obstruction = 0;
    ctx->openLimit = 0;
    ctx->closedLimit = 1;
    ctx->motor = 0;
    ctx->lastButton = 0;
    ctx->lastDirection = -1;
    enter(ctx, DOOR_CLOSED);
}

void garageDoorCyclic(GarageDoor_ts *const ctx, uint16_t cycle_ms)
{
    uint8_t pressed = ctx->button && !ctx->lastButton;
    uint8_t timeout;

    ctx->lastButton = ctx->button;
    if (ctx->modeTime_ms < 60000)
        ctx->modeTime_ms += cycle_ms;
    timeout = ctx->modeTime_ms >= ctx->maxRunTime_ms;

    switch (ctx->mode)
    {
    case DOOR_CLOSED:
        if (pressed)
            enter(ctx, DOOR_OPENING);
        break;

    case DOOR_OPENING:
        if (ctx->openLimit)
            enter(ctx, DOOR_OPEN);
        else if (pressed || timeout)
            enter(ctx, DOOR_STOPPED);
        break;

    case DOOR_OPEN:
        if (pressed && !ctx->obstruction)
            enter(ctx, DOOR_CLOSING);
        break;

    case DOOR_CLOSING:
        if (ctx->obstruction)
            enter(ctx, DOOR_OPENING); /* something in the way: go back up */
        else if (ctx->closedLimit)
            enter(ctx, DOOR_CLOSED);
        else if (pressed || timeout)
            enter(ctx, DOOR_STOPPED);
        break;

    case DOOR_REVERSING:
        if (ctx->modeTime_ms >= ctx->reversePause_ms)
            enter(ctx, DOOR_OPENING);
        break;

    case DOOR_STOPPED:
        /* The next press runs the door the other way, once the motor has been off long enough */
        if (pressed && ctx->modeTime_ms >= ctx->reversePause_ms)
        {
            if (ctx->lastDirection > 0 && !ctx->obstruction)
                enter(ctx, DOOR_CLOSING);
            else if (ctx->lastDirection < 0)
                enter(ctx, DOOR_OPENING);
        }
        break;
    }

    ctx->motor = ctx->mode == DOOR_OPENING ? 1 : ctx->mode == DOOR_CLOSING ? -1 : 0;
}
