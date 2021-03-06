////GENERIC VARIABLES
bool inChooser = true;
enum widgetModes {COIN, D6, SPINNER, TIMER, RPS};
byte currentWidget = TIMER;
byte currentVal = 1;
enum goSignals {INERT, GOING, RESOLVING, EXEMPT};
byte goSignal = EXEMPT;
bool isAnimating = false;
Timer animTimer;
byte animFrame = 0;

////WIDGET SPECIFIC VARIABLES
Color spinnerColors[6] = {RED, ORANGE, YELLOW, GREEN, BLUE, MAGENTA};
int spinInterval = 25;
byte spinLength;

bool inHiding = false;
enum rpsChoices {NADA, ROCK, PAPER, SCISSOR};
byte rpsSignal = 0;

Color headsColor;
Color tailsColor;

int ticksRemaining;
Timer tickTimer;
Timer tickOffsetTimer;
byte tickFace;
enum timerStates {SETTING, TIMING, COMPLETE};
byte timerState = SETTING;

void setup() {
}

////////////////
//MASTER LOOPS//
////////////////

void loop() {
  //listen for long press to switch in and out of chooser
  if (buttonLongPressed()) {
    if (inChooser) {//move to the game itself
      inChooser = false;
      switch (currentWidget) {
        case COIN:
          goSignal = INERT;
          headsColor = spinnerColors[rand(2)];
          tailsColor = spinnerColors[rand(2) + 3];
          coinDisplay(inChooser, 3);
          break;
        case D6:
          goSignal = INERT;
          d6Display(currentVal, inChooser);
          break;
        case SPINNER:
          goSignal = INERT;
          spinnerDisplay(7, false);
          break;
        case TIMER:
          currentVal = 1;
          break;
        case RPS:
          currentVal = 1;
          rpsDisplay(currentVal);
          break;
      }
    } else {
      inChooser = true;
      goSignal = EXEMPT;
      inHiding = false;
    }
  }

  //run the appropriate loop();
  if (inChooser) {
    osLoop();
  } else {
    //before the loops, update goSignal if we're not exempt
    if (goSignal != EXEMPT) {
      goSignalLoop();
    }
    //now run the loops
    switch (currentWidget) {
      case COIN:
        coinLoop();
        break;
      case D6:
        d6Loop();
        break;
      case SPINNER:
        spinnerLoop();
        break;
      case TIMER:
        timerLoop();
        break;
      case RPS:
        rpsLoop();
        break;
    }
  }

  //set up communication
  byte sendData = (inChooser << 5) + (goSignal << 3) + (inHiding << 2) + (rpsSignal);
  setValueSentOnAllFaces(sendData);

  //dump click data
  buttonSingleClicked();
  buttonDoubleClicked();
}

void osLoop() {
  //listen for clicking to cycle through widgets
  if (buttonSingleClicked()) {
    nextWidget();
  }

  //listen for animation updates
  if (animTimer.isExpired()) {
    switch (currentWidget) {
      case COIN:
        currentVal ++;
        if (currentVal == 3) {
          currentVal = 1;
        }
        coinDisplay(inChooser, currentVal);
        animTimer.set(400);
        break;
      case D6:
        currentVal++;
        if (currentVal == 7) {
          currentVal = 1;
        }
        d6Display(currentVal, true);
        animTimer.set(250);
        break;
      case SPINNER:
        currentVal = nextClockwise(currentVal);
        spinnerOSDisplay(currentVal);
        animTimer.set(100);
        break;
      case TIMER:
        currentVal ++;
        if (currentVal == 10) {
          currentVal = 0;
        }
        timerOSDisplay(currentVal);
        animTimer.set(100);
        break;
      case RPS:
        animFrame++;
        if (animFrame > 1) {
          animFrame = 0;
        }
        rpsOSDisplay(animFrame);
        animTimer.set(600);
        break;
    }
  }
}

void nextWidget() {
  switch (currentWidget) {
    case COIN:
      currentWidget = D6;
      currentVal = 1;
      break;
    case D6:
      currentWidget = SPINNER;
      currentVal = 0;
      break;
    case SPINNER:
      currentWidget = TIMER;
      currentVal = 0;
      break;
    case TIMER:
      currentWidget = RPS;
      currentVal = 1;
      animFrame = 0;
      break;
    case RPS:
      currentWidget = COIN;
      currentVal = 1;
      break;
  }
}

void goSignalLoop() {
  byte currentSignal = goSignal;//this is to avoid the loops automatically completing
  switch (currentSignal) {
    case INERT:
      if (goCheck() == true) {
        goSignal = GOING;
      }
      break;
    case GOING:
      if (resolveCheck() == true) {
        goSignal = RESOLVING;
      }
      break;
    case RESOLVING:
      if (inertCheck() == true) {
        goSignal = INERT;
      }
      break;
  }
}

////////////////
//WIDGET LOOPS//
////////////////

void coinLoop() {

  if (!isAnimating) {
    //there are two ways to start flipping: get clicked or be commanded
    if (buttonSingleClicked() || goSignal == GOING) {//were we clicked?
      isAnimating = true;
      animFrame = 32 + (rand(1) * 8);
      goSignal = GOING;
    }
  }

  if (isAnimating) {
    if (animTimer.isExpired()) {
      coinDisplay(inChooser, currentVal);//display the animation at frame X
      if (animFrame % 4 == 2) {//this is the inversion point of value
        if (currentVal == 1) {
          currentVal = 2;
        } else if (currentVal == 2) {
          currentVal = 1;
        }
      }
      animFrame --;
      animTimer.set(75);
    }//end of timer loop

    if (animFrame == 3) {
      isAnimating = false;
    }
  }
}

void d6Loop() {

  if (!isAnimating) {
    //there are two ways to start rolling: get clicked or be commanded
    if (buttonSingleClicked() || goSignal == GOING) {//were we clicked?
      isAnimating = true;
      animFrame = 0;
      spinInterval = 50;
      goSignal = GOING;
    }
  }

  if (isAnimating) {
    if (animTimer.isExpired()) {
      currentVal = rand(5) + 1;
      d6Display(currentVal, false);
      animFrame ++;
      spinInterval += 10;
      animTimer.set(spinInterval);
    }

    if (animFrame == 20) {
      isAnimating = false;
      d6Display(currentVal, false);
    }
  }
}

void spinnerLoop() {
  if (!isAnimating) {
    //there are two ways to start spinning: get clicked or be commanded
    if (buttonSingleClicked() || goSignal == GOING) {
      isAnimating = true;
      spinLength = rand(5) + 42;
      spinInterval = 25;
      goSignal = GOING;
    }
  }

  if (isAnimating) {
    if (animTimer.isExpired()) {
      currentVal = nextClockwise(currentVal);
      spinnerDisplay(currentVal, false);
      spinLength--;
      animTimer.set(spinInterval);

      if (spinLength < 24) {
        spinInterval = (spinInterval * 23) / 20;
      }
    }

    if (spinLength == 0) {
      isAnimating = false;
      spinnerDisplay(currentVal, true);
    }
  }
}

void timerLoop() {
  switch (timerState) {
    case SETTING:
      //in here we listen for button clicks to increment currentVal, which represents minutes on the timer
      if (buttonSingleClicked()) {
        currentVal++;
        animFrame = 0;
        animTimer.set(0);
        if (currentVal == 6) {
          currentVal = 1;
        }
      }

      //if double clicked, we move on
      if (buttonDoubleClicked()) {
        ticksRemaining = currentVal * 60;
        tickFace = 1;
        tickOffsetTimer.set(500);
        timerState = TIMING;
      }
      break;
    case TIMING:
      //first check to make sure we haven't been cancelled via double click
      if (buttonDoubleClicked()) {
        timerState = SETTING;
      }
      //otherwise we simply count down the remaining ticks
      if (tickTimer.isExpired()) {
        timerCountdownDisplay(true);

        tickFace = nextClockwise(tickFace);
        tickTimer.set(1000);
        tickOffsetTimer.set(500);
      }
      if (tickOffsetTimer.isExpired()) {
        timerCountdownDisplay(false);
        ticksRemaining--;
        tickOffsetTimer.set(1000);
      }
      if (ticksRemaining == 0) {
        timerState = COMPLETE;
      }
      break;
    case COMPLETE:
      if (buttonSingleClicked()) { //back to SETTING!
        timerState = SETTING;
        animFrame = 0;
      }
      break;
  }
  timerDisplay();
}

void rpsLoop() {
  if (!inHiding) {
    if (buttonSingleClicked()) {
      currentVal ++;
      if (currentVal == 4) {
        currentVal = 1;
      }
      rpsDisplay(currentVal);
      rpsSignal = currentVal;
    }

    if (buttonDoubleClicked()) { //toggle hiding mode
      inHiding = true;
      rpsDisplay(4);
    }
  }

  if (inHiding) {//check for double clicks or combat
    //we need to evaluate all neighbors, see if they are in RPS hidden mode
    byte neighborsIWin = 0;
    byte neighborsILose = 0;
    byte neighborsITie = 0;

    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) { //there is a neighbor here
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getRPSHiddenSignal(neighborData) == 1) { //this neighbor is in hiding, ready to play
          int victoryCheck = rpsSignal - getRPSSignal(neighborData);
          if (victoryCheck == 1 || victoryCheck == -2) {
            neighborsIWin ++;
          } else if (victoryCheck == 2 || victoryCheck == -1) {
            neighborsILose ++;
          } else {
            neighborsITie ++;
          }
        }
      }
    }

    if (animTimer.isExpired()) {
      if (animFrame == 0) {
        animFrame = 1;
      } else {
        animFrame = 0;
      }
      animTimer.set(250);
    }

    //decide how to display win/loss/tie state
    if (neighborsIWin + neighborsILose + neighborsITie > 0) {//first, do I have neighbors at all?
      if (neighborsILose > 0) {
        rpsCombatDisplay(rpsSignal, 0);
      } else if (neighborsITie > 0) {
        rpsCombatDisplay(rpsSignal, 1);
      } else if (neighborsIWin > 0) {
        rpsCombatDisplay(rpsSignal, 2);
      }
    } else {//so I'm alone
      rpsDisplay(4);
    }

    if (buttonDoubleClicked()) { //toggle hiding mode
      inHiding = false;
      rpsDisplay(currentVal);
    }
  }
}

/////////////////
//DISPLAY LOOPS//
/////////////////

void coinDisplay(bool osMode, byte val) {
  if (osMode) {//OS display
    switch (val) {
      case 1:
        setColor(dim(WHITE, 128));
        setColorOnFace(WHITE, 0);
        setColorOnFace(WHITE, 1);
        setColorOnFace(WHITE, 2);
        break;
      case 2:
        setColor(dim(WHITE, 128));
        setColorOnFace(WHITE, 3);
        setColorOnFace(WHITE, 4);
        setColorOnFace(WHITE, 5);
        break;
    }
  } else {//not in OS mode
    if (val == 3) { //this is the two face display when you exit chooser
      setColorOnFace(headsColor, 0);
      setColorOnFace(headsColor, 1);
      setColorOnFace(headsColor, 2);
      setColorOnFace(tailsColor, 3);
      setColorOnFace(tailsColor, 4);
      setColorOnFace(tailsColor, 5);
    } else {//actual animation
      Color currentColor;
      if (val == 1) {
        currentColor = headsColor;
      } else {
        currentColor = tailsColor;
      }
      switch (animFrame % 4) {
        case 0:
          setColor(currentColor);
          break;
        case 1:
          setColor(currentColor);
          setColorOnFace(OFF, 1);
          setColorOnFace(OFF, 4);
          break;
        case 2:
          setColor(OFF);
          break;
        case 3:
          setColor(currentColor);
          setColorOnFace(OFF, 1);
          setColorOnFace(OFF, 4);
          break;
      }
    }
  }
}

void d6Display(byte num, bool osMode) {
  setColor(OFF);
  Color displayColor;
  byte displayBrightness = 255;
  byte rotationRandomizer = 0;
  bool displayArr[6] = {false, false, false, false, false, false};

  switch (num) {
    case 1:
      if (osMode) {
        displayArr[0] = true;
      } else {
        displayArr[rand(5)] = true;
      }
      displayColor = RED;
      break;
    case 2:
      if (osMode) {
        rotationRandomizer = 0;
      } else {
        rotationRandomizer = rand(2);
      }
      displayArr[rotationRandomizer] = true;
      displayArr[rotationRandomizer + 3] = true;
      displayColor = ORANGE;
      break;
    case 3:
      if (osMode) {
        rotationRandomizer = 0;
      } else {
        rotationRandomizer = rand(1);
      }
      displayArr[rotationRandomizer] = true;
      displayArr[rotationRandomizer + 2] = true;
      displayArr[rotationRandomizer + 4] = true;
      displayColor = YELLOW;
      break;
    case 4:
      if (osMode) {
        rotationRandomizer = 0;
      } else {
        rotationRandomizer = rand(2);
      }
      displayArr[0] = true;
      displayArr[1] = true;
      displayArr[2] = true;
      displayArr[3] = true;
      displayArr[4] = true;
      displayArr[5] = true;
      displayArr[rotationRandomizer] = false;
      displayArr[rotationRandomizer + 3] = false;
      displayColor = GREEN;
      break;
    case 5:
      displayArr[0] = true;
      displayArr[1] = true;
      displayArr[2] = true;
      displayArr[3] = true;
      displayArr[4] = true;
      displayArr[5] = true;
      if (osMode) {
        displayArr[3] = false;
      } else {
        displayArr[rand(5)] = false;
      }
      displayColor = BLUE;
      break;
    case 6:
      displayArr[0] = true;
      displayArr[1] = true;
      displayArr[2] = true;
      displayArr[3] = true;
      displayArr[4] = true;
      displayArr[5] = true;
      displayColor = MAGENTA;
      break;
  }

  if (osMode) {
    displayColor = WHITE;
    setColor(dim(WHITE, 128));
  }

  //set brightness
  if (isAnimating) {
    displayBrightness = 128;
  }

  FOREACH_FACE(f) {
    if (displayArr[f] == true) {
      setColorOnFace(dim(displayColor, displayBrightness), f);
    }
  }
}

void spinnerDisplay(byte face, bool isFinal) {

  FOREACH_FACE(f) {
    byte brightness = 160;
    if (isFinal) {
      brightness = 100;
    }
    setColorOnFace(dim(spinnerColors[f], brightness), f);
  }

  if (isFinal) {
    setColorOnFace(spinnerColors[face], face);
  } else {
    setColorOnFace(WHITE, face);
  }
}

void spinnerOSDisplay(byte face) {
  setColor(dim(WHITE, 128));
  setColorOnFace(dim(WHITE, 100), 0);
  setColorOnFace(dim(WHITE, 100), 2);
  setColorOnFace(dim(WHITE, 100), 4);
  setColorOnFace(WHITE, face);
}

void timerOSDisplay(byte count) {
  setColor(dim(WHITE, 100));
  if (count < 6) {
    setColorOnFace(WHITE, count);
  }
  setColorOnFace(WHITE, 0);
}

void timerDisplay() {//only handles SETTING and COMPLETE display, the actual countdown is handled elsewhere
  switch (timerState) {
    case SETTING:
      if (animTimer.isExpired()) {
        //so we need to tick on the lights every second
        setColor(OFF);
        if (animFrame < currentVal) {//we are not ready to reset
          FOREACH_FACE(f) {
            if (f <= animFrame) {
              setColorOnFace(spinnerColors[currentVal - 1], f + 1);
            }
          }
        }

        animFrame++;

        if (animFrame > currentVal) {
          animTimer.set(500);
          animFrame = 0;
        } else if (animFrame == currentVal) {
          animTimer.set(500);
        } else {
          animTimer.set(100);
        }
        setColorOnFace(WHITE, 0);
      }
      break;
    case COMPLETE:
      if (animTimer.isExpired()) {
        setColor(dim(RED, 128));
        if (animFrame == 0) {
          setColor(RED);
          animFrame = 1;
        } else if (animFrame == 1) {
          animFrame = 0;
        }
        animTimer.set(50);
      }
      setColorOnFace(WHITE, 0);
      break;
  }
}

void timerCountdownDisplay(bool tickOn) {
  //first, set background color
  int dimness = 255 - (((60 - (ticksRemaining - 1) % 60)) * 3);
  if (ticksRemaining > 240) { //still in the fifth minute
    setColor(dim(BLUE, dimness));
  } else if (ticksRemaining > 180) { //in the fourth minute
    setColor(dim(GREEN, dimness));
  } else if (ticksRemaining > 120) { //in the third book
    setColor(dim(YELLOW, dimness));
  } else if (ticksRemaining > 60) { //in the second minute
    setColor(dim(ORANGE, dimness));
  } else {//in the last minute
    setColor(dim(RED, dimness));
  }

  //now, if it's the appropriate time, turn on the tick face
  if (tickOn) {
    setColorOnFace(WHITE, tickFace);
  }
}

void rpsDisplay(byte choice) {
  setColor(OFF);
  switch (choice) {
    case 1://rock
      setColorOnFace(BLUE, 0);
      setColorOnFace(BLUE, 1);
      setColorOnFace(BLUE, 2);
      setColorOnFace(BLUE, 3);
      break;
    case 2://paper
      setColorOnFace(YELLOW, 1);
      setColorOnFace(YELLOW, 2);
      setColorOnFace(YELLOW, 4);
      setColorOnFace(YELLOW, 5);
      break;
    case 3://scissor
      setColorOnFace(RED, 0);
      setColorOnFace(RED, 2);
      setColorOnFace(RED, 4);
      break;
    case 4://hiding
      setColorOnFace(dim(RED, 128), 0);
      setColorOnFace(dim(YELLOW, 128), 1);
      setColorOnFace(dim(BLUE, 128), 2);
      setColorOnFace(dim(RED, 128), 3);
      setColorOnFace(dim(YELLOW, 128), 4);
      setColorOnFace(dim(BLUE, 128), 5);
      break;
  }
}

void rpsCombatDisplay(byte choice, byte outcome) {
  setColor(OFF);
  bool layout[6];
  bool rockLayout[6] = {true, true, true, true, false, false};
  bool paperLayout[6] = {false, true, true, false, true, true};
  bool scissorLayout[6] = {true, false, true, false, true, false};
  Color currentColor;
  //determine layout and default color
  switch (choice) {
    case 1:
      FOREACH_FACE(f) {
        layout[f] = rockLayout[f];
      }
      currentColor = BLUE;
      break;
    case 2:
      FOREACH_FACE(f) {
        layout[f] = paperLayout[f];
      }
      currentColor = YELLOW;
      break;
    case 3:
      FOREACH_FACE(f) {
        layout[f] = scissorLayout[f];
      }
      currentColor = RED;
      break;
  }

  //determine flash color change
  if (animFrame == 0) {
    switch (outcome) {
      case 0://losers flash off
        currentColor = OFF;
        break;
      case 2://winners flash on
        currentColor = WHITE;
        break;
    }
  }

  //apply the color
  FOREACH_FACE(f) {
    if (layout[f]) {
      setColorOnFace(currentColor, f);
    }
  }
}

void rpsOSDisplay(byte frame) {
  setColor(dim(WHITE, 128));
  if (frame == 0) {
    setColorOnFace(WHITE, 0);
    setColorOnFace(WHITE, 2);
    setColorOnFace(WHITE, 4);
  }
}

///////////////////////////
//COMMUNICATION FUNCTIONS//
///////////////////////////

byte getOSMode(byte data) {
  return (data >> 5);//first bit
}

byte getGoSignal(byte data) {
  return ((data >> 3) & 3);//second and third bit
}

byte getRPSHiddenSignal(byte data) { //fourth bit
  return ((data >> 2) & 1);
}

byte getRPSSignal(byte data) {
  return (data & 3);//last two bits
}

bool goCheck() {
  //check all neighbors for any neighbors giving go signal
  bool goBool = false;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGoSignal(neighborData) == GOING) {
        goBool = true;
      }
    }
  }
  return goBool;
}

bool resolveCheck() {
  bool resolveBool = true;
  //if all of my neighbors are going, resolving, or exempt, we can resolve
  //easy to determine, because the only remaining state is INERT
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGoSignal(neighborData) ==  INERT) { //this is the only thing that prevents this transition
        resolveBool = false;
      }
    }
  }

  return resolveBool;
}

bool inertCheck() {
  bool inertBool = true;
  //if all of my neighbors are resolving, inert, or exempt, we can go inert
  //easy to determine, because the only remaining state is GOING
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGoSignal(neighborData) ==  GOING) { //this is the only thing that prevents this transition
        inertBool = false;
      }
    }
  }

  return inertBool;
}

/////////////////////////
//CONVENIENCE FUNCTIONS//
/////////////////////////

byte nextClockwise (byte face) {
  if (face == 5) {
    return 0;
  } else {
    return face + 1;
  }
}

byte nextCounterclockwise (byte face) {
  if (face == 0) {
    return 5;
  } else {
    return face - 1;
  }
}
