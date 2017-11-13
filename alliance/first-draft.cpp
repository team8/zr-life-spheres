//Begin page main
//Declare any variables shared between functions here

// Terrain and Targeting Variables
#define MAX_TERRAIN_SPOTS_KNOWN 15
#define DISTANCE_TO_ZONE_FOR_DROP .195f

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
    int currentPntr = 0;
    for (float i = .14; i < .64; i += .08) { // need to start at .2 so that we don't get the 10 point zone drill penalty
        for (float j = .14; j < .64; j += .08) {
            float loc[2] = {i,j};
            float height = game.getTerrainHeight(loc);
            DEBUG(("%f, %f, %f", loc[0], loc[1], height));
            if (game.getTerrainHeight(loc) < .41 && currentPntr < MAX_TERRAIN_SPOTS_KNOWN) {
                DEBUG(("RUNNING"));
                terrainHeights[currentPntr][0] = i;
                terrainHeights[currentPntr][1] = j;
                currentPntr += 1;
            }
        }
    }
}

// INIT Function
void init(){
    state = MOVING_TO_DRILL;
    setTerrainHeights();
    color = (myState[0] > 0); // TRUE FOR BLUE, FALSE FOR RED
    terrainPntr = -1; // just the way it is written
    setTarget();
    
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
    currentTarget[0] = terrainHeights[terrainPntr][0];
    currentTarget[1] = terrainHeights[terrainPntr][1];
    currentTarget[2] = .40 - .11 - .015;  
    DEBUG(("CURRENT TARGET <%f,%f,%f>", currentTarget[0],currentTarget[1],currentTarget[2]));
}

void flattenMe() {
    DEBUG(("FLATTENING"));
    if (fabsf(myState[8]) > .075) {
         api.setQuatTarget(spinAlignQuat);
    }
    else {
        float real_zero[3] = {0,0,0};
        api.setAttRateTarget(real_zero);

    }
}

// LOOP Function
void loop(){
    api.getMyZRState(myState);
    velocity = sqrtf(myState[3] * myState[3]  + myState[4] * myState[4] + myState[5] * myState[5]);
    rotV = sqrtf(myState[9] * myState[9]  + myState[10] * myState[10] + myState[11] * myState[11]);
    bool geyserOnMe = game.isGeyserHere(myState);
    
    // MOVING TO DRILL STATE
    if (state == MOVING_TO_DRILL) {
        float distance = moveTo(currentTarget);
        
        if (distance < .06 && velocity < .0020 && fabsf(myState[2] - currentTarget[2]) < .04) {
             state = STARTING_DRILL;
             DEBUG(("SWITCHING STATE"));
        }
        flattenMe();
    }
    
    if (state == STARTING_DRILL) {
        if (rotV > .025 || fabsf(myState[8]) > .075) {
             flattenMe(); // we are not stopped or we are not flat
        }
        else {
            game.startDrill();
            state = DRILLING;
             api.setAttRateTarget(drillTarget);
        }
    }
    
    if (state == DRILLING) {
        bool justPickedUpSample = false;
        if(game.checkSample()){
           game.pickupSample();
           game.stopDrill();
           justPickedUpSample = true;
       }
       
       if (justPickedUpSample && (game.getNumSamplesHeld() == 5 ||  (game.getNumSamplesHeld() > 0 && api.getTime() > 155 && 
                game.getFuelRemaining() > .18))) {
            state = DROPPING_OFF;
            game.stopDrill();
            
            for (int i = 0; i <3; i++) zero[i] = 0;
            float vecBet[3];
            mathVecSubtract(vecBet, myState, zero,3);
            mathVecScale(vecBet, DISTANCE_TO_ZONE_FOR_DROP/mathVecMagnitude(vecBet,3));
            memcpy(zero, vecBet, sizeof(float)*3);
            
        }
        else if (justPickedUpSample) {
            state = MOVING_TO_DRILL;
            setTarget();
 
        }
        
        api.setAttRateTarget(drillTarget);
        api.setPositionTarget(currentTarget);
    }
    
    if (state == DROPPING_OFF) {
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
    mathVecScale(vecBet, 1.2);
    // mathVecNormalize(vecBet, 3);    

    float distance = mathVecMagnitude(vecBet,3);
    if (distance > .25) {
        vecBet[2] -= .03;
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