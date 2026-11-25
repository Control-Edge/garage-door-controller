#include <math.h>
#include <stdio.h>
#include <cppmodel/CModel.h>
#include "garageDoor.h"

#define DOOR_SPEED_PCT_PER_S 10.0 /* full travel in 10 s */

static GarageDoor_ts door;
static double position; /* plant: 0 % closed .. 100 % open */
static uint8_t button, obstruction;
static CppModelRule_t neverIntoObstruction, reopens, modeSequence;

/* ---- Stimuli and checks: each runs once per cycle for its duration, and returns whether the cycle passed ---- */

static uint8_t PressButton(CModelSimulation_ts *self, unsigned long t)
{
    (void)self, (void)t;
    button = 1;
    return 1;
}

static uint8_t BlockPhotoEye(CModelSimulation_ts *self, unsigned long t)
{
    (void)self, (void)t;
    obstruction = 1;
    return 1;
}

static uint8_t PressWhileBlocked(CModelSimulation_ts *self, unsigned long t)
{
    (void)self, (void)t;
    button = 1;
    obstruction = 1;
    return 1;
}

static uint8_t IsOpen(CModelSimulation_ts *self, unsigned long t)
{
    (void)self, (void)t;
    return door.mode == DOOR_OPEN && position >= 100.0;
}

static uint8_t IsClosed(CModelSimulation_ts *self, unsigned long t)
{
    (void)self, (void)t;
    return door.mode == DOOR_CLOSED && position <= 0.0;
}

static uint8_t IsStopped(CModelSimulation_ts *self, unsigned long t)
{
    (void)self, (void)t;
    return door.mode == DOOR_STOPPED && door.motor == 0;
}

/* Press and release the wall button: written once, used throughout */
static const CppModelStep_ts pressSteps[] = {
    CMODEL_STEP(PressButton, 200),
    CMODEL_STEP(NULL, 300),
};
static const CppModelInputProfile_ts press = CMODEL_INPUT_PROFILE(pressSteps);

static const CppModelStep_ts testSteps[] = {
    /* Open the door */
    CMODEL_STEP_INPUT_PROFILE(press),
    CMODEL_STEP(NULL, 11000),
    CMODEL_STEP(IsOpen, 0),
    /* Close it, and block the photo-eye halfway down: it must go back up */
    CMODEL_STEP_INPUT_PROFILE(press),
    CMODEL_STEP(NULL, 4000),
    CMODEL_STEP(BlockPhotoEye, 1000),
    CMODEL_STEP(NULL, 6000),
    CMODEL_STEP(IsOpen, 0),
    /* With the photo-eye blocked, a press doesn't close the door */
    CMODEL_STEP(PressWhileBlocked, 200),
    CMODEL_STEP(BlockPhotoEye, 2000),
    CMODEL_STEP(IsOpen, 0),
    /* Stop it halfway down, then send it back up */
    CMODEL_STEP_INPUT_PROFILE(press),
    CMODEL_STEP(NULL, 4000),
    CMODEL_STEP_INPUT_PROFILE(press),
    CMODEL_STEP(NULL, 2000),
    CMODEL_STEP(IsStopped, 0),
    CMODEL_STEP_INPUT_PROFILE(press),
    CMODEL_STEP(NULL, 6000),
    CMODEL_STEP(IsOpen, 0),
    /* Close it all the way */
    CMODEL_STEP_INPUT_PROFILE(press),
    CMODEL_STEP(NULL, 11000),
    CMODEL_STEP(IsClosed, 0),
};
static const CppModelInputProfile_ts test = CMODEL_INPUT_PROFILE(testSteps);

static void Init(CModelSimulation_ts *self, unsigned long stepTime_ms)
{
    (void)stepTime_ms;

    garageDoorInit(&door,
                   CppModel_getParameterU16(self, "ReversePause [ms]", 500),
                   CppModel_getParameterU16(self, "MaxRunTime [ms]", 15000));
    position = 0.0;

    /* GD-1: the door never runs down while the photo-eye is blocked */
    neverIntoObstruction = CppModel_ruleAlways(self, "never closes on an obstruction", 0.0);
    CppModel_ruleSignal(neverIntoObstruction, "Door.Motor [%]");

    /* GD-2: an obstruction while closing sends the door back up within 1 s */
    reopens = CppModel_ruleResponse(self, "reopens within 1 s", 1.0, 0.0);
    CppModel_ruleSignal(reopens, "Door.Motor [%]");

    /* GD-3: the motor rests 0.5 s before it changes direction; GD-4: no travel lasts longer than 15 s */
    modeSequence = CppModel_ruleMode(self, "mode sequence");
    CppModel_modeAllow(modeSequence, DOOR_CLOSED, DOOR_OPENING);
    CppModel_modeAllow(modeSequence, DOOR_OPENING, DOOR_OPEN);
    CppModel_modeAllow(modeSequence, DOOR_OPENING, DOOR_STOPPED);
    CppModel_modeAllow(modeSequence, DOOR_OPEN, DOOR_CLOSING);
    CppModel_modeAllow(modeSequence, DOOR_CLOSING, DOOR_CLOSED);
    CppModel_modeAllow(modeSequence, DOOR_CLOSING, DOOR_STOPPED);
    CppModel_modeAllow(modeSequence, DOOR_CLOSING, DOOR_REVERSING);
    CppModel_modeAllow(modeSequence, DOOR_REVERSING, DOOR_OPENING);
    CppModel_modeAllow(modeSequence, DOOR_STOPPED, DOOR_OPENING);
    CppModel_modeAllow(modeSequence, DOOR_STOPPED, DOOR_CLOSING);
    CppModel_modeDwell(modeSequence, DOOR_REVERSING, 0.5, NAN);
    CppModel_modeDwell(modeSequence, DOOR_STOPPED, 0.5, NAN);
    CppModel_modeDwell(modeSequence, DOOR_OPENING, NAN, 15.0);
    CppModel_modeDwell(modeSequence, DOOR_CLOSING, NAN, 15.0);
    CppModel_ruleSignal(modeSequence, "Door.Mode");

    CppModel_verifyRequirement(self, "GD-1");
    CppModel_verifyRequirement(self, "GD-2");
    CppModel_verifyRequirement(self, "GD-3");
    CppModel_verifyRequirement(self, "GD-4");
}

void RunCyclic(CModelSimulation_ts *self, unsigned long currentTime_ms)
{
    double dt_s = self->stepTime_ms / 1000.0;
    uint8_t passed, obstructedWhileClosing;

    /* The running step sets the stimuli and checks the door */
    button = 0;
    obstruction = 0;
    passed = CppModel_runInputProfile(self, &test, currentTime_ms);

    door.button = CppModel_getInputU8(self, "Button", button);
    door.obstruction = CppModel_getInputU8(self, "PhotoEye.Blocked", obstruction);
    door.openLimit = position >= 100.0;
    door.closedLimit = position <= 0.0;
    obstructedWhileClosing = door.obstruction && door.motor < 0;

    /* Controller under test */
    garageDoorCyclic(&door, (uint16_t)self->stepTime_ms);

    /* Plant model: the motor moves the door at constant speed between its end stops */
    position += door.motor * DOOR_SPEED_PCT_PER_S * dt_s;
    position = position < 0.0 ? 0.0 : position > 100.0 ? 100.0 : position;

    CppModel_setOutputF64(self, "Door.Position [%]", position);
    CppModel_setOutputI8(self, "Door.Motor [%]", (int8_t)(door.motor * 100));
    CppModel_setOutputI32(self, "Door.Mode", door.mode);

    CppModel_updateAlways(neverIntoObstruction,
                          CppModel_implies(CppModel_condition(door.obstruction), CppModel_condition(door.motor >= 0)));
    CppModel_updateResponse(reopens, obstructedWhileClosing, door.motor > 0);
    CppModel_updateMode(modeSequence, door.mode);

    CppModel_setOutputU8(self, "CppModel.StepResult", passed);
}

int main(void)
{
    CModelSimulation_ts *simulation = CppModel_create("GarageDoor", CppModel_inputProfileDuration(&test, 10), 10);
    if (simulation == NULL)
        return 1;

    CppModel_setInitFunction(simulation, Init);
    CppModel_setRunStepFunction(simulation, RunCyclic);
    CppModel_Simulate(simulation);

    int result = CppModel_getSimulationResult(simulation);
    CppModel_destroy(simulation);

    if (result != 0)
        printf("First failing step: %d\n", result);
    return result == 0 ? 0 : 1;
}
