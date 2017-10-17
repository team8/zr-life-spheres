//Declare any variables shared between functions here

enum ROBOT_STATE {MOVING_TO_DRILL, STARTING_DRILL, AVOIDING_CAMPERS, DRILLING, DROPPING_OFF};

#define DISTANCE_FROM_CENTER 0.12f
#define CLOSEST_TO_WALL .365f
#define DROPOFF_TIMEOUT 45
#define DISTANCE_TO_ZONE_FOR_DROP .225f

#define DOWN_DIST_TARG .357f

ROBOT_STATE state;

float drillTarget[3];
float movementTarget[3];

float geyserEvationTarget[3];

float zero[3];
float myState[12];
float farDisMat[3];
float attTarget[3];
float velocity;

float newAttTarget[3];
bool hasPickedUp;

bool color;
bool evadedGeyser;

float spinAlignQuat[4];

int dropOffTimer;



void init(){
    dropOffTimer = DROPOFF_TIMEOUT;
    api.getMyZRState(myState);
    
    color = (myState[0] > 0); // TRUE FOR BLUE, FALSE FOR RED

    // api.setPosGains(1,0,5);
	//This function is called once when your code is first loaded.

	//IMPORTANT: make sure to set any variables that need an initial value.
	//Do not assume variables will be set to 0 automatically!
	state = MOVING_TO_DRILL;
	hasPickedUp = false;
	movementTarget[0] = .2 * (color ? 1 : -1);
	movementTarget[1] = .2 * (color ? 1 : -1);
	movementTarget[2] = DOWN_DIST_TARG ;
	
	for (int i = 0; i <3; i++) zero[i] = 0;
// 	zero[2] += .10;
	farDisMat[0] = 0;
	farDisMat[1] = 0;
	farDisMat[2] = -0.5;

    attTarget[0] = 1;
	attTarget[1] = 0;
	attTarget[2] = 0;
	
    drillTarget[0] = 0;
    drillTarget[1] = 0;
    drillTarget[2] = 1;
    evadedGeyser = false;
    
    for (int i = 0; i < 4; i++) {
        spinAlignQuat[i] = 0;
    }
    
    if (color) {
        spinAlignQuat[1] = 1;
    }
    else {
        spinAlignQuat[0] = -1;
    }
    

}

void flattenMe() {
    api.setQuatTarget(spinAlignQuat);
}

void loop(){
    // DEBUG(("WEEKEND SUBMISSION"));
    api.getMyZRState(myState);
    velocity = sqrtf(myState[3] * myState[3]  + myState[4] * myState[4] + myState[5] * myState[5]);
    float rotV = sqrtf(myState[9] * myState[9]  + myState[10] * myState[10] + myState[11] * myState[11]);
    // DEBUG(("%f, %f, %f", myState[6], myState[7], myState[8]));
    
    float mySquare[3];
    float myPos[3];
    myPos[0] = myState[0];
    myPos[1] = myState[1];
    myPos[2] = myState[2];
    // game.pos2square(myPos, mySquare);
    
    bool geyserOnMe = game.isGeyserHere(myState);
    // DEBUG(("GEYSER ON ME? %d", geyserOnMe));
    

    if (state == MOVING_TO_DRILL) {
        evadedGeyser = false;
        float distance = moveTo(movementTarget);
        
        // if (distance > .05) {
            flattenMe();
        // }
        
        // DEBUG(("%f < Z DIST", (fabsf(myState[2] - movementTarget[2]))));
        // DEBUG(("%f < CLOSE TO WALL", (myState[2] - CLOSEST_TO_WALL)));
        // DEBUG(("%f, DISTANCE", distance));
        // DEBUG(("%f, VELOCITY", velocity));

        // checks: Overall distance < .07. Velocity < .055, we are above the target, we are closer than .10 in the z direction
        if (distance < .07 && velocity < .0030 && myState[2] < CLOSEST_TO_WALL && fabsf(myState[2] - movementTarget[2]) < .07) {
             state = STARTING_DRILL;
        }
        
    }
    
    if (state == STARTING_DRILL) {
        if (rotV > .025 || fabsf(myState[8]) > .075) {
            // DEBUG(("FLATTENING"));
             flattenMe();
        }
        else {
        game.startDrill();
        // DEBUG(("%f, %f, %f", drillTarget[0], drillTarget[1], drillTarget[2]));
        state = DRILLING;
        DEBUG(("STARTED DRILL"));
        
         api.setAttRateTarget(drillTarget);
        }
    }
    
    if (state == DRILLING) {
        if(game.checkSample()){
           game.pickupSample();
       }
        
        api.setAttRateTarget(drillTarget);
        api.setPositionTarget(movementTarget);
        // DEBUG(("%f", rotV));
        
        if (game.getNumSamplesHeld() == 4 || geyserOnMe || (game.getNumSamplesHeld() > 0 && api.getTime() > 155 && game.getFuelRemaining() > .18)) {
            state = DROPPING_OFF;
             game.stopDrill();
             
             // CODE FOR CALCULATING TARGET IN SPHERE
         	for (int i = 0; i <3; i++) zero[i] = 0;
            float vecBet[3];
            mathVecSubtract(vecBet, myState, zero,3);
            mathVecScale(vecBet, DISTANCE_TO_ZONE_FOR_DROP/mathVecMagnitude(vecBet,3));
            memcpy(zero, vecBet, sizeof(float)*3);
            
            geyserEvationTarget[0] = -1*myState[0];
            geyserEvationTarget[1] = -1*myState[1];
            geyserEvationTarget[2] = 0;
            mathVecNormalize(geyserEvationTarget,3);
             
             
            if (geyserOnMe) {
                // DEBUG(("GEYSER ON ME EVADING"));
            	evadedGeyser = true;
            }
            else {
                evadedGeyser = false;
            }

        }
    }
    
    // DEBUG(("%d << DROPOFF", dropOffTimer));
    if (state == DROPPING_OFF) {
        DEBUG(("DROPPING OFF"));
        if (geyserOnMe) {
            evadedGeyser = true;
        }
        
        if (geyserOnMe) {
        api.setVelocityTarget(geyserEvationTarget);
        //   DEBUG(("TARGET %f, %f, %f", geyserEvationTarget[0], geyserEvationTarget[1], geyserEvationTarget[2]));

        }
        else {
         api.setPositionTarget(zero);
        // DEBUG(("TARGET %f, %f, %f", zero[0], zero[1], zero[2]));

        }
        
        // DEBUG(("DROPPING OFF"));

        api.setAttitudeTarget(farDisMat);
        
        if (game.atBaseStation()) {
            game.dropSample(0);
            game.dropSample(1);
            game.dropSample(2);
            game.dropSample(3);
            game.dropSample(4);
            dropOffTimer = DROPOFF_TIMEOUT;
            if (game.getFuelRemaining() > .02) {
                state = MOVING_TO_DRILL;
            }
            else {
        
            }
            movementTarget[0] -= 0.08 * (color ? 1 : -1); // move our drill location
            if (!evadedGeyser) {
                movementTarget[2] = DOWN_DIST_TARG;
            }
            else {
                DEBUG(("CHANGING TARGET"));
                movementTarget[2] = DOWN_DIST_TARG - .005; // hacky way to reduce overshooting
            }

        }
    }

    
    
}

float moveTo(float movement[3]) {

    float vecBet[3];
    mathVecSubtract(vecBet, movement, myState,3);
    mathVecScale(vecBet, 1.2);
    // mathVecNormalize(vecBet, 3);    

    float distance = mathVecMagnitude(vecBet,3);
    
    if (distance > .25) {
        // vecBet[2] -= .13; // let positionTarget take care of Z
        api.setVelocityTarget(vecBet);
    }
    else {
        api.setPositionTarget(movement);
    }
    
    return distance;

}

void mathVecScale(float vec[3], float amount) {
    for (int i = 0; i < 3; i++) vec[i] *= amount;
}
