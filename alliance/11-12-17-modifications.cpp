
// Terrain and Targeting Variables
#define MAX_TERRAIN_SPOTS_KNOWN 30
#define DISTANCE_TO_ZONE_FOR_DROP .19f
#define BOTTOM_WALL .2875f

float terrainHeights[MAX_TERRAIN_SPOTS_KNOWN][2];
float currentTarget[3];
int terrainPntr;

// Robot State
enum ROBOT_STATE {MOVING_TO_DRILL, STARTING_DRILL, AVOIDING_CAMPERS, DRILLING, DROPPING_OFF};
ROBOT_STATE state;
bool color; // sphere color
float myState[12]; // sphere state
float velocity; // sphere velocity
float rotV; // sphere rotation velocity

// Drill Movement Constants
float spinAlignQuat[4];
float drillTarget[3];
float farDisMat[3]; // Where we should be facing to drop off

float zero[3]; // not actually 0. location of base station drop off

// Function that gets the location of the terrain heights that are useful to us
void setTerrainHeights() {
    
    // Position of the current tile we are in
    float myPos[3] = {myState[0], myState[1], myState[2]};
    int startTile[2];
    game.pos2square(myPos, startTile);
    if (color) {
        startTile[0] -= 1;
        startTile[1] -= 1;
    }
    else {
        startTile[0] += 1;
        startTile[1] +=1;
    }
    int currentPntr = 0;
    // This algorithm finds the 5 closest drill spots by sweeping the grid in concentric circles with the sphere at the
    // center. The layer determines which circle we're looking at
    int layer = 0;
    while (currentPntr < 15 && layer < 10) {
        // current tile being checked
        int currentTile[3] = {startTile[0] - layer, startTile[1] - layer, myPos[2]};
        // Repeat for each tile in the layer
        int tiles = 8*layer;
        if (layer == 0) tiles = 1;
        for (int i = 0; i < tiles; i++) {
            // Skip tiles that are out of bounds or already drilled
            if (currentTile[0] >= -8 && currentTile[0] <= 8 &&
                currentTile[1] >= -10 && currentTile[1] <= 10 &&
                (fabsf(currentTile[0])+fabsf(currentTile[1])) > 4 &&
                game.getDrills(currentTile) == 0) {
                    // If the tile is at the height we want, add it to the list
                    if (game.getTerrainHeight(currentTile) < .41) {
                        DEBUG(("%d,%d,%d", currentTile[0], currentTile[1], currentTile[2]));
                        float newPos[3];
                        game.square2pos(currentTile, newPos);
                        terrainHeights[currentPntr][0] = newPos[0];
                        terrainHeights[currentPntr][1] = newPos[1];
                        currentPntr = currentPntr + 1;
                    }
                }
            // Determine which spot to search next. For each layer, we start at the top left corner and sweep clockwise
            if (currentTile[0] - startTile[0] == layer && currentTile[1] - startTile[1] < layer) {
                // Down
                currentTile[1] = currentTile[1] + 1;
            } else if (startTile[1] - currentTile[1] == layer) {
                // Right
                currentTile[0] = currentTile[0] + 1;
            } else if (startTile[0] - currentTile[0] == layer) {
                // Up
                currentTile[1] = currentTile[1] - 1;
            } else {
                // Left
                currentTile[0] = currentTile[0] - 1;
            }
        }
        layer = layer + 1;
    }
}

// INIT Function
void init(){
    api.getMyZRState(myState);
    state = MOVING_TO_DRILL;
    color = (myState[0] > 0); // TRUE FOR BLUE, FALSE FOR RED
    setTerrainHeights();

    terrainPntr = -1; // just the way it is written
    setTarget();
    
    for (int i = 0; i < MAX_TERRAIN_SPOTS_KNOWN; i++ ) {
        DEBUG(("X: %f, Y: %f", terrainHeights[i][0], terrainHeights[i][1]));
    }
    
    // Quarternion to align to zone
    for (int i = 0; i < 4; i++) {
        spinAlignQuat[i] = 0;
    }
    
    if (color) {
        spinAlignQuat[1] = 1;
    }
    else {
        spinAlignQuat[0] = -1;
    }
    
    drillTarget[0] = 0;
    drillTarget[1] = 0;
    drillTarget[2] = .4;
    
	farDisMat[0] = 0;
	farDisMat[1] = 0;
	farDisMat[2] = -0.5;
}

void setTarget() {
    terrainPntr += 1;
    DEBUG(("TARGET PNTR: %d", terrainPntr));
    currentTarget[0] = terrainHeights[terrainPntr][0];
    currentTarget[1] = terrainHeights[terrainPntr][1];
    currentTarget[2] = .40 - .10 - .020;  
    DEBUG(("CURRENT TARGET <%f,%f,%f>", currentTarget[0],currentTarget[1],currentTarget[2]));
}

void flattenMe() {
    // if (fabsf(myState[8]) > .075) {
    api.setQuatTarget(spinAlignQuat);
    //          DEBUG(("FLATTENING WITH QUAt"));

    // }
    // else {
    //     float real_zero[3] = {0,0,0};
    //     api.setAttRateTarget(real_zero);

    // }
}

// LOOP Function
void loop(){
    DEBUG(("LOOP"));
    api.getMyZRState(myState);
    velocity = sqrtf(myState[3] * myState[3]  + myState[4] * myState[4] + myState[5] * myState[5]);
    rotV = sqrtf(myState[9] * myState[9]  + myState[10] * myState[10] + myState[11] * myState[11]);
    
    // MOVING TO DRILL STATE
    if (state == MOVING_TO_DRILL) {
        DEBUG(("MOVING TO DRILL"));
        float distance = moveTo(currentTarget);
        
        if (distance < .05 && velocity < .0030 && fabsf(myState[2] - currentTarget[2]) < .022 && myState[2] < BOTTOM_WALL) {
             state = STARTING_DRILL;
             DEBUG(("SWITCHING STATE"));
        }
        flattenMe();
    }
    
    if (state == STARTING_DRILL) {
        DEBUG(("STARTING TO DRILL"));
        if (rotV > .025 || fabsf(myState[8]) > .075) {
            DEBUG(("Keep flattening"));
             flattenMe(); // we are not stopped or we are not flat
        }
        else {
            game.startDrill();
            state = DRILLING;
            if (api.getTime() > 160) drillTarget[2] = .9; // If this is the end of the game, go faster
             api.setAttRateTarget(drillTarget);
             currentTarget[2] -= .005; // move up the current target slightly to stop drifting
        }
    }
    
    if (state == DRILLING) {
        DEBUG(("DRILLING"));
        bool justPickedUpSample = false;
        if(game.checkSample()){
           game.pickupSample();
           justPickedUpSample = true;
       }
       
       if (justPickedUpSample && game.getNumSamplesHeld() % 2 == 1) {
           state = DRILLING;
           DEBUG(("STILL DRILLING"));
       }
       else if (justPickedUpSample && (game.getNumSamplesHeld() == 4 ||  (game.getNumSamplesHeld() > 0 && api.getTime() > 155 && 
                game.getFuelRemaining() > .18)) && !game.isGeyserHere(myState)) {
            state = DROPPING_OFF;
            game.stopDrill();
            
            for (int i = 0; i <3; i++) zero[i] = 0;
            float vecBet[3];
            mathVecSubtract(vecBet, myState, zero,3);
            mathVecScale(vecBet, DISTANCE_TO_ZONE_FOR_DROP/mathVecMagnitude(vecBet,3));
            memcpy(zero, vecBet, sizeof(float)*3);
            DEBUG(("HERE 1"));
            
        }
        else if (justPickedUpSample ) {
            DEBUG(("HERE 2"));
            state = MOVING_TO_DRILL;
            if (game.isGeyserHere(myState)) {
                DEBUG(("ADJUSTING FOR GEYSER"));
                
                if (terrainPntr < 9) {
                    terrainPntr += 8; // hope that this is far enough to correct for the geyser
                }
                else {
                    terrainPntr -= 5;
                }
            }
            setTarget();
            game.stopDrill();
        }
        
        api.setAttRateTarget(drillTarget);
        api.setPositionTarget(currentTarget); // maintain our current position while drilling
    }
    
    // bool geyserOnMe = ;
    // DEBUG(("%d", geyserOnMe));
    
    // This will only trigger if the geyser hits us while we are collecting the first sample at a location
    if (game.isGeyserHere(myState) && state != MOVING_TO_DRILL) {
        state = MOVING_TO_DRILL;
        // setTerrainHeights(true);
        DEBUG(("WE got a geyser on the first drill."));
        terrainPntr += 8; // hope that this is far enough to correct for the geyser

        setTarget();
        game.stopDrill();
    }

    
    if (state == DROPPING_OFF) {
        DEBUG(("DROPPING OFF"));
        if (game.getNumSamplesHeld() == 0) {
            state = MOVING_TO_DRILL;
            // setTerrainHeights(false);
            setTarget();
        }
        DEBUG(("DROPPING OFF"));
        api.setPositionTarget(zero);
        
        api.setAttitudeTarget(farDisMat);
        
        if (game.atBaseStation()) {
            game.dropSample(0);
            game.dropSample(1);
            game.dropSample(2);
            game.dropSample(3);
            game.dropSample(4);
            if (game.getFuelRemaining() > .02) {
                // setTerrainHeights(false);
                state = MOVING_TO_DRILL;
            }
            setTarget();

        }
    }
}

// Paly Robotics Move Function
// Returns the Distance From the Target
// If you are further than a threshold, move with setVelocityTarget
// If you are close, use setPositionTarget
float moveTo(float movement[3]) {

    float vecBet[3];
    mathVecSubtract(vecBet, movement, myState,3);
    
    float scalar;
    if (sqrtf(vecBet[0]*vecBet[0] + vecBet[1]*vecBet[1]) > .3) {
        scalar = 1.01;
    }
    else {
        scalar = 1.35;
    }
    
    mathVecScale(vecBet, scalar);
    // mathVecNormalize(vecBet, 3);    

    float distance = mathVecMagnitude(vecBet,3);
    if (distance > .295) {
        vecBet[2] -= .05;
        api.setVelocityTarget(vecBet);
    }
    else {
        api.setPositionTarget(movement);
    }
    
    return distance;

}

// Scales a vector by a quantity
void mathVecScale(float vec[3], float amount) {
    for (int i = 0; i < 3; i++) vec[i] *= amount;
}

//End page main