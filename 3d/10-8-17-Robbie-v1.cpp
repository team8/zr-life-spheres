//Declare any variables shared between functions here

enum ROBOT_STATE {MOVING_TO_DRILL, STARTING_DRILL, DRILLING, DROPPING_OFF};

#define DISTANCE_FROM_CENTER 0.12f

ROBOT_STATE state;

float drillTarget[3];
float movementTarget[3];

float zero[3];
float myState[12];
float farDisMat[3];
float attTarget[3];
float velocity;

float newAttTarget[3];
bool hasPickedUp;
bool newLocTargetSet;

bool color;

void init(){
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
	movementTarget[2] = .505 ;
	
	for (int i = 0; i <3; i++) zero[i] = 0;
	zero[2] += .08;
	farDisMat[0] = 0;
	farDisMat[1] = 0;
	farDisMat[2] = -0.5;

    attTarget[0] = 1;
	attTarget[1] = 0;
	attTarget[2] = 0;
	
	newLocTargetSet = false;
}

void flattenMe() {
    // float sphereGoal[3] = {0,0,0};
    // float newFTarget[3];
    // memcpy(newFTarget, movementTarget, sizeof(float)*3);
    // mathVecSubtract(sphereGoal,newFTarget, myState,3);
    
    // newFTarget[0] = sphereGoal[0];
    // newFTarget[1] = -1 * sphereGoal[2];
    // newFTarget[2] = 1 * sphereGoal[1];
    
    // float sphereGoal[3] = {0,  myState[7], 0};
    
    // DEBUG(("%f,%f,%f", sphereGoal[0], sphereGoal[1], sphereGoal[2]));
    // api.setAttitudeTarget(sphereGoal);
    
//   float sphereGoal[4] = {0.707f, 0.707f, 0.0f, 0.0f};
    float sphereGoal[4] = {0,1,0,0};
    api.setQuatTarget(sphereGoal);
}

void loop(){
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
    bool geyserOnMe = game.isGeyserHere(movementTarget);
    

    if (state == MOVING_TO_DRILL) {
        
        float distance = moveTo(movementTarget);
        
        // if (distance > .05) {
            flattenMe();
        // }
        
        // DEBUG(("%f", distance));
        
        if (distance < .04 && velocity < .001) {
            state = STARTING_DRILL;
        }
        
    }
    
    if (state == STARTING_DRILL) {
        if (rotV < .001 || myState[8] > .12) {
            DEBUG(("FLATTENING"));
             flattenMe();
        }
        else {
        game.startDrill();

        setTarget();
        DEBUG(("%f, %f, %f", drillTarget[0], drillTarget[1], drillTarget[2]));
        state = DRILLING;
        
        
         api.setAttRateTarget(drillTarget);
        }
    }
    
    if (state == DRILLING) {
        if(game.checkSample()){
           game.pickupSample();
       }
        
        api.setAttRateTarget(drillTarget);
        api.setPositionTarget(movementTarget);
        DEBUG(("%f", rotV));
        
        if (game.getNumSamplesHeld() == 3 || geyserOnMe || (game.getNumSamplesHeld() > 0 && api.getTime() > 160)) {
            state = DROPPING_OFF;
             game.stopDrill();

        }
    }
    
    if (state == DROPPING_OFF) {
        api.setPositionTarget(zero);
        api.setAttitudeTarget(farDisMat);
        
        if (game.atBaseStation()) {
            game.dropSample(0);
            game.dropSample(1);
            game.dropSample(2);
            state = MOVING_TO_DRILL;
            movementTarget[0] += 0.08;
            // attTarget[2] += .5;
        }
    }

    
    
}

float moveTo(float movement[3]) {

    float vecBet[3];
        
    mathVecSubtract(vecBet, movement, myState,3);
    mathVecScale(vecBet, 1.15);
    // mathVecNormalize(vecBet, 3);    

    float distance = mathVecMagnitude(vecBet,3);
    
    if (distance > .35) {
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

void setTarget() {
    // drillTarget[0] = -myState[6];
    drillTarget[0] = 0;
    drillTarget[1] = 0;
    drillTarget[2] = 1;
}