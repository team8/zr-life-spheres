enum BehaviorState {
    CHECKING_CLOSE, DRILLING_ZONE, DROPPING_OFF_SAMPLES
  };
  
  #define GRID_SIZE 0.08f
  #define GRID_CHECK 0.04f // Distance to box check if we are inside a square
  #define MAX_SURFACE_CHECK 0.64f
  #define MIN_SURFACE_CHECK 0.51f
  #define DRILL_ROTATE_VELOCITY 0.39f
  #define MAX_POSITION_VELOCITY 0.01f // Max positional velocity for doing various game things
  #define MAX_ATTITUDE_VELOCITY 0.04f // Max attitude velocity for starting various game things
  #define BASE_RADIUS 0.15f
  #define MIN_SAMPLE_CONCENTRATION 0.1f
  #define MAX_DRILLING_ANGLE_TILT 0.03125f
  
  float state[12]; // State of the sphere
  float p [3]; // Sphere position
  float pv[3]; // Sphere positional veloicty
  float a [3]; // Sphere attitude
  float av[3]; // Sphere attitude velocity
  
  float ds[4][2][3]; // Drill spots. First index is quadrant, second indexes which spot, and third indexes x, y, or z
  
  float ap[2][3]; // Analyzer positions
  
  float sc[3]; // Value of samples held
  
  bool isRed; // Whether or not we are the red sphere
  
  BehaviorState behaviorState;
  BehaviorState lastBehaviorState;
  
  bool drilling; // Whether or not we are drilling
  int hasAnalyzer;
  
  float dav[3]; // Target drill attitude velocity
  float bq [4]; // Target base station quaternion
  float dq [4]; // Target drill quaternion
  float mav[3]; // Max attitude velocity for doing things
  
  float z[3]; // Zero vector
  float bp[3]; // Base station position
  
  void setBehaviorState(BehaviorState newState) {
      
      DEBUG(("New state: %i", newState));
  
      lastBehaviorState = behaviorState;
      behaviorState = newState;
  }
  
  // Close and far quadrant indicies
  int cqi; int fqi;
  
  int cdc; // Close drill count
  int fdc; // Far drill count
  
  int ai;  // Drill zone area index
  int aqi; // Drill zone area quadrant index
  
  void init() {
  
      dc = 0;
      zdc = 0;
      fdc = 0;
      cdc = 0;
      
      ai = 0;
  
      z  [0] = 0.0f; z  [1] = 0.0f; z  [2] = 0.0f;
      bp [0] = 0.0f; bp [1] = 0.0f; bp [2] = 0.08f;
      dav[0] = 0.0f; dav[1] = 0.0f; dav[2] = DRILL_ROTATE_VELOCITY;
      mav[0] = 0.0f; mav[1] = 0.0f; mav[2] = MAX_ATTITUDE_VELOCITY;
      
      bq[0] = 0.707f; bq[1] = 0.0f; bq[2] = 0.707f; bq[3] = 0.0f;
      dq[0] = 0.0f  ; dq[1] = 1.0f; dq[2] =   0.0f; dq[3] = 0.0f;
      
      float brp[2][3];
      brp[0][0] = GRID_SIZE*4.5f; brp[0][1] = GRID_SIZE*3.5f; brp[0][2] = 0.51f;
      brp[1][0] = GRID_SIZE*3.5f; brp[1][1] = GRID_SIZE*6.5f; brp[1][2] = 0.51f;
      
      // Set position of drill spots
      for (int i = 0; i < 4; i++) {
          int xm = (i == 0 || i == 3) ? 1 : -1;
          int ym = (i == 2 || i == 3) ? 1 : -1;
          for (int j = 0; j < 2; j++) {
              ds[i][j][0] = xm*brp[j][0] + 0.01f*xm;
              ds[i][j][1] = ym*brp[j][1] + 0.01f*ym;
              ds[i][j][2] = brp[j][2];
          } 
      }
      
      // Set position of analyzers
      for (int i = 0; i < 2; i++) {
          int xm = i == 0 ? -1 : 1;
          int ym = i == 0 ? -1 : 1;
          ap[i][0] = xm * 0.3f;
          ap[i][1] = ym * -0.48f;
          ap[i][2] = -0.36f;
      }
  
      getMyState();
  
      isRed = p[1] < 0.0f;
      
      cqi = isRed ? 1 : 3;
      fqi = isRed ? 0 : 2;
      
      behaviorState = CHECKING_CLOSE;
  }
  
  void loop() {
  
      getMyState();
  
      switch(behaviorState) {
          case CHECKING_CLOSE: {
              drillingClose();
              break;
          }
          case DRILLING_ZONE: {
              drillingZone();
              break;
          }
          case DROPPING_OFF_SAMPLES: {
              droppingOffSamples();
              break;
          }
      }
  }
  
  void getMyState() {
      
      api.getMyZRState(state);
      
      for (int i = 0; i < 3; i++) p[i] = state[i  ]; for (int i = 0; i < 3; i++) pv[i] = state[i+3];
      for (int i = 0; i < 3; i++) a[i] = state[i+6]; for (int i = 0; i < 3; i++) av[i] = state[i+9];
      
      hasAnalyzer = game.hasAnalyzer();
      drilling = game.getDrillEnabled();
  }
  
  void drillingClose() {
      
      unsigned int drillTimes = cdc == 0 ? 1 : 2;
      
      if (drill(ds[cqi][cdc], drillTimes)) cdc++;
      
      if (cdc == 2) setBehaviorState(DROPPING_OFF_SAMPLES);
  }
  
  unsigned int zdc;
  
  void drillingZone() {
      
      DEBUG(("aqi: %i, ai: %i", aqi, ai));
      
      float av[3] = {GRID_SIZE, 0.0f, 0.0f};
      float gzp[3];
      mathVecAdd(gzp, ds[aqi][ai], av, 3);
      
      if (drill(gzp, 3)) zdc++;
      
      DEBUG(("zdc: %i", zdc));
      
      if (zdc == 2) setBehaviorState(DROPPING_OFF_SAMPLES);
  }
  
  unsigned int dc;
  
  bool drill(float drillPos[], unsigned const int count) {
      
      api.setPositionTarget(drillPos);
  
      if (!drilling) {
         
         float gp[3] = {drillPos[0], drillPos[1]};
         if (game.isGeyserHere(gp)) setBehaviorState(DROPPING_OFF_SAMPLES);
         
          flatten();
         
          if (fabsf(av[2]) < MAX_ATTITUDE_VELOCITY) {
              if (
                  fabs(p[0] - drillPos[0]) < GRID_CHECK && fabs(p[1] - drillPos[1]) < GRID_CHECK
                  && mathVecMagnitude(pv, 3) < MAX_POSITION_VELOCITY
                  && p[2] < MAX_SURFACE_CHECK && p[2] > MIN_SURFACE_CHECK
                  //&& a[2] < MAX_DRILLING_ANGLE_Z
              ) {
                  game.startDrill();
              }
          } else {
              api.setAttRateTarget(mav);
          }
          
      } else {
         
          if (game.getDrillError()) game.stopDrill();
         
          api.setAttRateTarget(dav);
      
          if (game.checkSample()) {
          
              dc++;
              
              game.pickupSample();
              
              if (dc == count) {
                  
                  game.stopDrill();
                  dc = 0;
                  return true;
              }
          } 
      }
      return false;
  }
  
  void droppingOffSamples() {
      
      api.setPositionTarget(bp);
      api.setQuatTarget(bq);
      
      if (game.getNumSamplesHeld() > 0) {
          
          if (fabsf(av[2]) < MAX_ATTITUDE_VELOCITY ) {
     
              if (game.atBaseStation()) {
                  
                  int maxIndex = 3; float max = MIN_SAMPLE_CONCENTRATION;
                  for (int i = 0; i < 3; i++) {
                      
                      float concentration = game.dropSample(i);
                      sc[i] = concentration;
                      
                      if (concentration > max) {
                          max = concentration;
                          maxIndex = i;
                      }
                  }
                  
                  ai = maxIndex;
                  
                  aqi = ((ai == 3) ? fqi : cqi);
                  
                  setBehaviorState(DRILLING_ZONE);
              }
          } else {
              api.setAttRateTarget(mav);
          }
      }
  }
  
  void flatten() {
      
      api.setQuatTarget(dq);
  }
  
  bool boxCheck(const float d, float cp[], float tp[]) {
      
      return (fabsf(cp[0] - tp[0]) < d && fabsf(cp[1] - tp[1]) < d && fabsf(cp[2] - tp[2]) < d);
  }