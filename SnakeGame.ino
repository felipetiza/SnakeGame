/*

TODO
------

> Add walls
> The fruit blink       Bug - The blink also affects to the player

Future Feature
---------------
2 players (2 snakes). If one crosses the body of the other, this one is shortened
from where it was crossed to the head. Lose what is left with <= 3 parts

*/

#include "LedControlMS.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define _Matrices 8
#define _SnakeParts 35
#define _SnakePartsMainMenu 3
#define _SnakeSpeed 80
#define _MenuAnimationSpeed 60
#define _TimeTransition 3000

LedControl m(12, 11, 10, _Matrices);    // DIN, CLK, CS/LOAD, #chips
LiquidCrystal_I2C lcd(0x27, 16, 2);

int score;
bool startGame = false,
     gameOver  = false,
     pause     = false;

bool showedMenu  = false,
     showedScore = false;

// Animation MainMenu
int mA[_SnakePartsMainMenu], xA[_SnakePartsMainMenu], yA[_SnakePartsMainMenu];
bool goUp[_SnakePartsMainMenu], changeDirection[_SnakePartsMainMenu];

// Joystick
int analogInputPinX = 6,
analogInputPinY = 7,
digitalInputPin = 8;
long joystickX, joystickY;
int joystickInput;
bool pressedPause = false;

// ***** Snake
int matriz[_SnakeParts], x[_SnakeParts], y[_SnakeParts];
int matrizHead, xHead, yHead;                                               // No array > head there's only 1
int matrizBody[_SnakeParts], xBody[_SnakeParts], yBody[_SnakeParts];        // Array > body's parts there's a lots
bool up, down, left, right;
String currentDirection;
int parts = 1;
bool headPasted = false;

// ***** Food
bool foodEnable = false;
int foodM, foodX, foodY;

void setup(){
    Serial.begin(9600);
    // Joystick
    pinMode(digitalInputPin, INPUT);
    digitalWrite(digitalInputPin, HIGH);
    // LCD
    lcd.init();
    lcd.backlight();
    // Matrixes
    for (int i=0;i<_Matrices;i++){
        m.shutdown(i, false);
        m.setIntensity(i, 0);
    }
    reset();
}

void loop(){

    // readFromKeyboard();
    readFromJoystick();
    if(!pause){
        if(!startGame){
            if(!showedMenu){
                showScreenLCD("menu");
                screenMenu();
                showedMenu = true;
            }
            screenAnimationMenu();
            delay(_MenuAnimationSpeed);
        }else{
            if(!showedScore){
                transitionAnimation();
                refresh("allLeds");
                showScreenLCD("score");
                showedScore = true;
            }
            snake();
            food();
            collision();

            delay(_SnakeSpeed);
        }
    }
    // showInfo();
    // showPositionJoystick();
}

void reset(){
    refresh("allLeds");
    matriz[0] = 3;
    x[0] = 0;
    y[0] = 0;
    parts = 1;
    score = 0;
    startGame = false;
    gameOver = false;
    currentDirection = loadDirection("right");

    showedMenu = false;
    showedScore = false;

    // Snake's animation of screenMenu
    mA[0] = 7;
    xA[0] = 3;
    yA[0] = 0;

    mA[1] = 7;
    xA[1] = 4;
    yA[1] = 1;

    mA[2] = 7;
    xA[2] = 5;
    yA[2] = 2;

    for(int i=0;i<_SnakePartsMainMenu;i++){
        goUp[i] = false;
        changeDirection[i] = false;
    }
}

void food(){
    if(!foodEnable){
        foodM = random(8);  // Show of 0-7
        foodX = random(8);
        foodY = random(8);

        // If food's position is equal snake's position, then put the food in other place
        for(int i=0;i<parts;i++){
            if( (foodM == matriz[i]) && (foodX == x[i]) && (foodY == y[i]) ){
                foodM = random(8);
                foodX = random(8);
                foodY = random(8);

                i = 0;
            }
        }
        foodEnable = true;
    }
    m.setLed(foodM, foodX, foodY, true);
}

// Each body's part get the position from his previous part, except the head
// Head > Put the position + Controls the direction
// Body > Get the position
void snake(){
    refresh("snake");
    // showPartsSnake();

    for(int i=0;i<parts;i++){

        // Movement
        if(i == 0){                     // Part 1 = head
            // Copiar cabeza
            xHead = x[i];
            yHead = y[i];
            matrizHead = matriz[i];

            if(up)         x[i]--;
            else if(down)  x[i]++;
            else if(left)  y[i]--;
            else if(right) y[i]++;
        }else if(i > 0){                // (Parts != 1) = body

            // Copy body, except the last part
            if(i+1 < parts){
                xBody[i]      = x[i];
                yBody[i]      = y[i];
                matrizBody[i] = matriz[i];
            }

            // Paste head (only once)
            if(!headPasted){
                x[i]      = xHead;
                y[i]      = yHead;
                matriz[i] = matrizHead;

                headPasted = true;
            }else{
                x[i]      = xBody[i-1];
                y[i]      = yBody[i-1];
                matriz[i] = matrizBody[i-1];
            }
        }

        // Draw
        joinMap(matriz[i], x[i], y[i]);
        m.setLed(matriz[i], x[i], y[i], true);

        // showPositionSnake(i);
    }
    headPasted = false;
    // Serial.println();
}

void collision(){
    if( (matriz[0] == foodM) && (x[0] == foodX) && (y[0] == foodY) ){                // Snake - Food
        parts++;
        if(parts == _SnakeParts){
            parts--;
            // Blink all snake's pixels
            for(int j=100;j<150;j+=2){
                for(int i=0;i<parts;i++)
                m.setLed(matriz[i], x[i], y[i], false);
                delay(j);
                for(int i=0;i<parts;i++)
                m.setLed(matriz[i], x[i], y[i], true);
            }
            refresh("allLeds");
            showScreenLCD("win");
            screenWin();
            delay(_TimeTransition);
            reset();
        }else{
            score++;
            showScreenLCD("score");
            foodEnable = false;
        }
    }else{
        // To crash himself, it's need 5 parts minimum, it's impossible with 4
        if(parts > 4){                                                                 // Snake - Snake
            for(int i=3;i<parts;i++){
                if( (matriz[0] == matriz[i]) && (x[0] == x[i]) && (y[0] == y[i]) ){

                    // Pixel's blink that has crashed
                    for(int j=100;j<150;j+=2){
                        m.setLed(matriz[i], x[i], y[i], false);
                        delay(j);
                        m.setLed(matriz[i], x[i], y[i], true);
                    }
                    refresh("allLeds");
                    showScreenLCD("gameOver");
                    screenGameOver();
                    delay(_TimeTransition);
                    reset();
                }
            }
        }
    }
}

void readFromKeyboard(){
    String keyReaded;

    if(Serial.available() > 0){
        keyReaded = (String)(char) Serial.read();

        if(startGame){
            if(keyReaded == "p") { pause = !pause;                                 }
            else if(keyReaded == "w") { currentDirection = loadDirection("up");    }
            else if(keyReaded == "s") { currentDirection = loadDirection("down");  }
            else if(keyReaded == "a") { currentDirection = loadDirection("left");  }
            else if(keyReaded == "d") { currentDirection = loadDirection("right"); }
            else if(keyReaded == "l") { refresh("food"); foodEnable = false; }
            else if(keyReaded == "b") { refresh("allLeds"); }
            else if(keyReaded == "r") { reset(); }
            else if(keyReaded == "m" && parts < _SnakeParts) { parts++; }
            else if(keyReaded == "n" && parts > 1) { parts--; m.setLed(matriz[parts], x[parts], y[parts], false); }
        }else{
            if(keyReaded == "e") { startGame = true; refresh("allLeds"); }
        }
    }
}

void readFromJoystick(){
    joystickX = analogRead(analogInputPinX);
    joystickY = analogRead(analogInputPinY);
    joystickInput = digitalRead(digitalInputPin);

    if(startGame){
        // Pause
        if(joystickInput == 0) pressedPause = true;
        else if(joystickInput == 1 && pressedPause == true){ pause = !pause; pressedPause = false; }

        if(joystickY < 490 && !pause)      { currentDirection = loadDirection("up");    }
        else if(joystickY > 520 && !pause) { currentDirection = loadDirection("down");  }
        else if(joystickX < 490 && !pause) { currentDirection = loadDirection("left");  }
        else if(joystickX > 520 && !pause) { currentDirection = loadDirection("right"); }
    }else{
        if(joystickInput == 0) { startGame = true; }
    }
}

String loadDirection(String newDir){
    up    = false;
    down  = false;
    left  = false;
    right = false;

    // Snake move any direction except to his opposite (it avoid that pass through his own body)
    if(parts > 1){
        if(newDir == "up" && currentDirection != "down")         up    = true;
        else if(newDir == "down" && currentDirection != "up")    down  = true;
        else if(newDir == "left" && currentDirection != "right") left  = true;
        else if(newDir == "right" && currentDirection != "left") right = true;
        else{
            // If snake try to go in opposite direction, we avoid it putting it the initial direction
            if(newDir == "up"){         down  = true;  newDir = "down";  }
            else if(newDir == "down"){  up    = true;  newDir = "up";    }
            else if(newDir == "left"){  right = true;  newDir = "right"; }
            else if(newDir == "right"){ left  = true;  newDir = "left";  }
        }
    }else{
        if(newDir == "up")         up    = true;
        else if(newDir == "down")  down  = true;
        else if(newDir == "left")  left  = true;
        else if(newDir == "right") right = true;
    }

    return newDir;
}

void refresh(String who){
    if(who == "snake")                                                 // Delete only the snake's last part
        m.setLed(matriz[parts-1], x[parts-1], y[parts-1], false);
    else if(who == "food")
        m.setLed(foodM, foodX, foodY, false);
    else if(who == "allLeds"){
        for (int i=0;i<_Matrices;i++)
            m.clearDisplay(i);
    }
}

void joinMap(int newMatriz, int newX, int newY){
    // Desplazamiento vertical
    if(newX > 7){
        x[0] = 0;

        if(newMatriz == 0)       matriz[0] = 4;
        else if(newMatriz == 1)  matriz[0] = 5;
        else if(newMatriz == 2)  matriz[0] = 6;
        else if(newMatriz == 3)  matriz[0] = 7;
        else if(newMatriz == 4)  matriz[0] = 0;
        else if(newMatriz == 5)  matriz[0] = 1;
        else if(newMatriz == 6)  matriz[0] = 2;
        else if(newMatriz == 7)  matriz[0] = 3;
    }else if(newX < 0){
        x[0] = 7;

        if(newMatriz == 0)       matriz[0] = 4;
        else if(newMatriz == 1)  matriz[0] = 5;
        else if(newMatriz == 3)  matriz[0] = 7;
        else if(newMatriz == 2)  matriz[0] = 6;
        else if(newMatriz == 4)  matriz[0] = 0;
        else if(newMatriz == 5)  matriz[0] = 1;
        else if(newMatriz == 6)  matriz[0] = 2;
        else if(newMatriz == 7)  matriz[0] = 3;
    }

    // Desplazamiento horizontal
    if(newY > 7){
        y[0] = 0;

        if(newMatriz == 0)       matriz[0] = 3;   // Doy la vuelta
        else if(newMatriz == 1)  matriz[0] = 0;
        else if(newMatriz == 2)  matriz[0] = 1;
        else if(newMatriz == 3)  matriz[0] = 2;
        else if(newMatriz == 4)  matriz[0] = 7;   // Doy la vuelta
        else if(newMatriz == 6)  matriz[0] = 5;
        else if(newMatriz == 5)  matriz[0] = 4;
        else if(newMatriz == 7)  matriz[0] = 6;
    }else if(newY < 0){
        y[0] = 7;

        if(newMatriz == 0)       matriz[0] = 1;
        else if(newMatriz == 1)  matriz[0] = 2;
        else if(newMatriz == 2)  matriz[0] = 3;
        else if(newMatriz == 3)  matriz[0] = 0;    // Doy la vuelta
        else if(newMatriz == 4)  matriz[0] = 5;
        else if(newMatriz == 5)  matriz[0] = 6;
        else if(newMatriz == 6)  matriz[0] = 7;
        else if(newMatriz == 7)  matriz[0] = 4;    // Doy la vuelta
    }
}

void joinMapA(int newMatriz, int newX, int newY, int i){
    // Desplazamiento vertical
    if(newX > 7){
        xA[i] = 0;

        if(newMatriz == 0)       mA[i] = 4;
        else if(newMatriz == 1)  mA[i] = 5;
        else if(newMatriz == 2)  mA[i] = 6;
        else if(newMatriz == 3)  mA[i] = 7;
        else if(newMatriz == 4)  mA[i] = 0;
        else if(newMatriz == 5)  mA[i] = 1;
        else if(newMatriz == 6)  mA[i] = 2;
        else if(newMatriz == 7)  mA[i] = 3;
    }else if(newX < 0){
        xA[i] = 7;

        if(newMatriz == 0)       mA[i] = 4;
        else if(newMatriz == 1)  mA[i] = 5;
        else if(newMatriz == 3)  mA[i] = 7;
        else if(newMatriz == 2)  mA[i] = 6;
        else if(newMatriz == 4)  mA[i] = 0;
        else if(newMatriz == 5)  mA[i] = 1;
        else if(newMatriz == 6)  mA[i] = 2;
        else if(newMatriz == 7)  mA[i] = 3;
    }

    // Desplazamiento horizontal
    if(newY > 7){
        yA[i] = 0;

        if(newMatriz == 0)       mA[i] = 3;   // Doy la vuelta
        else if(newMatriz == 1)  mA[i] = 0;
        else if(newMatriz == 2)  mA[i] = 1;
        else if(newMatriz == 3)  mA[i] = 2;
        else if(newMatriz == 4)  mA[i] = 7;   // Doy la vuelta
        else if(newMatriz == 6)  mA[i] = 5;
        else if(newMatriz == 5)  mA[i] = 4;
        else if(newMatriz == 7)  mA[i] = 6;
    }else if(newY < 0){
        yA[i] = 7;

        if(newMatriz == 0)       mA[i] = 1;
        else if(newMatriz == 1)  mA[i] = 2;
        else if(newMatriz == 2)  mA[i] = 3;
        else if(newMatriz == 3)  mA[i] = 0;    // Doy la vuelta
        else if(newMatriz == 4)  mA[i] = 5;
        else if(newMatriz == 5)  mA[i] = 6;
        else if(newMatriz == 6)  mA[i] = 7;
        else if(newMatriz == 7)  mA[i] = 4;    // Doy la vuelta
    }
}

void showScreenLCD(String todo){
    lcd.clear();
    if(todo == "menu")
        lcd.print("Welcome to Snake");
    else if(todo == "win")
        lcd.print("Congratulations!");
    else if(todo == "gameOver")
        lcd.print("Game Over");
    else if(todo == "score"){
        lcd.print("Score: ");
        lcd.print(score);
    }
}

void showInfo(){
    Serial.print("Parts: ");
    Serial.print(parts);
    Serial.print("  Score: ");
    Serial.println(score);
}

void showPositionSnake(int i){
    Serial.print("   P");
    Serial.print(i+1);
    Serial.print(": ");
    Serial.print(matriz[i]);
    Serial.print(" ");
    Serial.print(x[i]);
    Serial.print(" ");
    Serial.print(y[i]);
}

void showPositionJoystick(){
    Serial.print("Pulsar: ");
    Serial.print(joystickInput);
    Serial.print(" X: ");
    Serial.print(joystickX);
    Serial.print(" Y: ");
    Serial.println(joystickY);
}

void screenMenu(){
    // "SNAKE"
    m.setLed(1, 1, 0, true);
    m.setLed(2, 1, 0, true);
    m.setLed(0, 1, 1, true);
    m.setLed(1, 1, 1, true);
    m.setLed(0, 1, 2, true);
    m.setLed(1, 1, 2, true);
    m.setLed(0, 1, 3, true);
    m.setLed(3, 1, 3, true);
    m.setLed(0, 1, 4, true);
    m.setLed(1, 1, 4, true);
    m.setLed(2, 1, 4, true);
    m.setLed(3, 1, 4, true);
    m.setLed(3, 1, 5, true);
    m.setLed(2, 1, 6, true);
    m.setLed(3, 1, 6, true);
    m.setLed(1, 1, 7, true);
    m.setLed(2, 1, 7, true);
    m.setLed(2, 2, 0, true);
    m.setLed(0, 2, 1, true);
    m.setLed(2, 2, 1, true);
    m.setLed(1, 2, 2, true);
    m.setLed(3, 2, 3, true);
    m.setLed(1, 2, 4, true);
    m.setLed(2, 2, 4, true);
    m.setLed(1, 2, 6, true);
    m.setLed(2, 2, 6, true);
    m.setLed(1, 3, 0, true);
    m.setLed(2, 3, 0, true);
    m.setLed(0, 3, 1, true);
    m.setLed(1, 3, 1, true);
    m.setLed(0, 3, 2, true);
    m.setLed(1, 3, 2, true);
    m.setLed(2, 3, 2, true);
    m.setLed(0, 3, 3, true);
    m.setLed(3, 3, 3, true);
    m.setLed(1, 3, 4, true);
    m.setLed(2, 3, 4, true);
    m.setLed(3, 3, 4, true);
    m.setLed(1, 3, 5, true);
    m.setLed(3, 3, 5, true);
    m.setLed(2, 3, 6, true);
    m.setLed(3, 3, 6, true);
    m.setLed(2, 3, 7, true);
    m.setLed(2, 4, 0, true);
    m.setLed(0, 4, 1, true);
    m.setLed(1, 4, 2, true);
    m.setLed(2, 4, 3, true);
    m.setLed(1, 4, 4, true);
    m.setLed(2, 4, 4, true);
    m.setLed(1, 4, 6, true);
    m.setLed(2, 4, 6, true);
    m.setLed(3, 4, 6, true);
    m.setLed(2, 5, 0, true);
    m.setLed(0, 5, 1, true);
    m.setLed(0, 5, 2, true);
    m.setLed(1, 5, 2, true);
    m.setLed(0, 5, 3, true);
    m.setLed(3, 5, 3, true);
    m.setLed(0, 5, 4, true);
    m.setLed(1, 5, 4, true);
    m.setLed(2, 5, 4, true);
    m.setLed(3, 5, 4, true);
    m.setLed(3, 5, 5, true);
    m.setLed(2, 5, 6, true);
    m.setLed(3, 5, 6, true);
    m.setLed(1, 5, 7, true);
}

void screenWin(){
    m.setLed(1, 1, 0, true);
    m.setLed(5, 1, 0, true);
    m.setLed(1, 1, 1, true);
    m.setLed(2, 1, 1, true);
    m.setLed(5, 1, 1, true);
    m.setLed(1, 1, 3, true);
    m.setLed(5, 1, 3, true);
    m.setLed(2, 1, 4, true);
    m.setLed(6, 1, 5, true);
    m.setLed(1, 1, 6, true);
    m.setLed(2, 1, 6, true);
    m.setLed(2, 1, 7, true);
    m.setLed(5, 1, 7, true);
    m.setLed(6, 1, 7, true);
    m.setLed(7, 1, 7, true);
    m.setLed(5, 2, 0, true);
    m.setLed(1, 2, 1, true);
    m.setLed(2, 2, 1, true);
    m.setLed(1, 2, 3, true);
    m.setLed(5, 2, 3, true);
    m.setLed(2, 2, 4, true);
    m.setLed(5, 2, 4, true);
    m.setLed(6, 2, 5, true);
    m.setLed(1, 2, 6, true);
    m.setLed(2, 2, 6, true);
    m.setLed(5, 2, 7, true);
    m.setLed(7, 2, 7, true);
    m.setLed(5, 3, 0, true);
    m.setLed(6, 3, 0, true);
    m.setLed(1, 3, 1, true);
    m.setLed(2, 3, 2, true);
    m.setLed(6, 3, 2, true);
    m.setLed(1, 3, 3, true);
    m.setLed(2, 3, 3, true);
    m.setLed(5, 3, 3, true);
    m.setLed(2, 3, 4, true);
    m.setLed(6, 3, 4, true);
    m.setLed(5, 3, 5, true);
    m.setLed(1, 3, 6, true);
    m.setLed(2, 3, 6, true);
    m.setLed(5, 3, 7, true);
    m.setLed(5, 4, 0, true);
    m.setLed(6, 4, 0, true);
    m.setLed(1, 4, 1, true);
    m.setLed(6, 4, 2, true);
    m.setLed(1, 4, 3, true);
    m.setLed(5, 4, 3, true);
    m.setLed(2, 4, 4, true);
    m.setLed(6, 4, 4, true);
    m.setLed(1, 4, 6, true);
    m.setLed(2, 4, 6, true);
    m.setLed(5, 4, 6, true);
    m.setLed(5, 4, 7, true);
    m.setLed(5, 5, 0, true);
    m.setLed(1, 5, 1, true);
    m.setLed(5, 5, 1, true);
    m.setLed(6, 5, 1, true);
    m.setLed(1, 5, 3, true);
    m.setLed(2, 5, 3, true);
    m.setLed(5, 5, 3, true);
    m.setLed(6, 5, 3, true);
    m.setLed(1, 5, 6, true);
    m.setLed(2, 5, 6, true);
    m.setLed(5, 5, 7, true);
    m.setLed(6, 5, 7, true);
    m.setLed(1, 6, 0, true);
    m.setLed(1, 6, 1, true);
    m.setLed(2, 6, 1, true);
    m.setLed(2, 6, 2, true);
    m.setLed(1, 6, 3, true);
    m.setLed(1, 6, 4, true);
    m.setLed(1, 6, 5, true);
    m.setLed(1, 6, 6, true);
    m.setLed(2, 6, 6, true);
    m.setLed(2, 6, 7, true);
}

void screenGameOver(){
    m.setLed(4, 1, 0, true);
    m.setLed(5, 1, 0, true);
    m.setLed(6, 1, 0, true);
    m.setLed(5, 1, 1, true);
    m.setLed(5, 1, 2, true);
    m.setLed(7, 1, 2, true);
    m.setLed(7, 1, 3, true);
    m.setLed(6, 1, 4, true);
    m.setLed(7, 1, 4, true);
    m.setLed(5, 1, 5, true);
    m.setLed(7, 1, 5, true);
    m.setLed(5, 1, 6, true);
    m.setLed(5, 1, 7, true);
    m.setLed(6, 1, 7, true);
    m.setLed(0, 2, 0, true);
    m.setLed(2, 2, 0, true);
    m.setLed(4, 2, 0, true);
    m.setLed(6, 2, 0, true);
    m.setLed(2, 2, 1, true);
    m.setLed(1, 2, 2, true);
    m.setLed(2, 2, 2, true);
    m.setLed(3, 2, 2, true);
    m.setLed(7, 2, 2, true);
    m.setLed(2, 2, 3, true);
    m.setLed(3, 2, 3, true);
    m.setLed(3, 2, 4, true);
    m.setLed(6, 2, 4, true);
    m.setLed(1, 2, 5, true);
    m.setLed(3, 2, 5, true);
    m.setLed(5, 2, 5, true);
    m.setLed(7, 2, 5, true);
    m.setLed(1, 2, 6, true);
    m.setLed(2, 2, 6, true);
    m.setLed(1, 2, 7, true);
    m.setLed(6, 2, 7, true);
    m.setLed(2, 3, 0, true);
    m.setLed(4, 3, 0, true);
    m.setLed(5, 3, 0, true);
    m.setLed(6, 3, 0, true);
    m.setLed(1, 3, 1, true);
    m.setLed(1, 3, 2, true);
    m.setLed(3, 3, 2, true);
    m.setLed(7, 3, 2, true);
    m.setLed(2, 3, 3, true);
    m.setLed(6, 3, 4, true);
    m.setLed(1, 3, 5, true);
    m.setLed(5, 3, 5, true);
    m.setLed(7, 3, 5, true);
    m.setLed(2, 3, 6, true);
    m.setLed(5, 3, 6, true);
    m.setLed(2, 3, 7, true);
    m.setLed(5, 3, 7, true);
    m.setLed(6, 3, 7, true);
    m.setLed(1, 4, 0, true);
    m.setLed(2, 4, 0, true);
    m.setLed(2, 4, 1, true);
    m.setLed(6, 4, 1, true);
    m.setLed(1, 4, 2, true);
    m.setLed(2, 4, 2, true);
    m.setLed(3, 4, 2, true);
    m.setLed(7, 4, 2, true);
    m.setLed(2, 4, 3, true);
    m.setLed(6, 4, 3, true);
    m.setLed(3, 4, 4, true);
    m.setLed(1, 4, 5, true);
    m.setLed(3, 4, 5, true);
    m.setLed(5, 4, 5, true);
    m.setLed(7, 4, 5, true);
    m.setLed(1, 4, 6, true);
    m.setLed(2, 4, 6, true);
    m.setLed(5, 4, 7, true);
    m.setLed(6, 4, 7, true);
    m.setLed(2, 5, 0, true);
    m.setLed(4, 5, 0, true);
    m.setLed(5, 5, 0, true);
    m.setLed(5, 5, 1, true);
    m.setLed(1, 5, 2, true);
    m.setLed(3, 5, 2, true);
    m.setLed(5, 5, 2, true);
    m.setLed(6, 5, 2, true);
    m.setLed(7, 5, 2, true);
    m.setLed(2, 5, 3, true);
    m.setLed(7, 5, 3, true);
    m.setLed(7, 5, 4, true);
    m.setLed(1, 5, 5, true);
    m.setLed(3, 5, 5, true);
    m.setLed(5, 5, 5, true);
    m.setLed(7, 5, 5, true);
    m.setLed(2, 5, 6, true);
    m.setLed(6, 5, 7, true);
    m.setLed(0, 6, 0, true);
    m.setLed(2, 6, 0, true);
    m.setLed(1, 6, 2, true);
    m.setLed(3, 6, 2, true);
    m.setLed(2, 6, 3, true);
    m.setLed(3, 6, 3, true);
    m.setLed(3, 6, 4, true);
    m.setLed(1, 6, 5, true);
    m.setLed(3, 6, 5, true);
    m.setLed(1, 6, 6, true);
    m.setLed(2, 6, 6, true);
    m.setLed(1, 6, 7, true);
}

void screenAnimationMenu(){
    // Turn off the leds each time these are moved
    for(int i=0;i<_SnakePartsMainMenu;i++)
        m.setLed(mA[i], xA[i], yA[i], false);

    for(int i=0;i<_SnakePartsMainMenu;i++){
        if(yA[i] == 8) yA[i] = 0;
        else           yA[i]++;

        if(xA[i] == 5){      goUp[i] = true;  changeDirection[i] = !changeDirection[i]; }
        else if(xA[i] == 2){ goUp[i] = false; changeDirection[i] = !changeDirection[i]; }

        if(goUp[i] && !changeDirection[i])       xA[i]--;
        else if(!goUp[i] && !changeDirection[i]) xA[i]++;

        joinMapA(mA[i], xA[i], yA[i], i);
        m.setLed(mA[i], xA[i], yA[i], true);
    }
}

void transitionAnimation(){
    // Move from left to right
    for (int x=0; x<16; x++){               // Vertical
        for (int y=0; y<32; y++){           // Horizontal

            if(x < 8){
                if(y < 8)
                    m.setLed(3, x, y, true);
                else if(y >= 8 && y < 16)
                    m.setLed(2, x, y - 8, true);
                else if(y >= 16 && y < 24)
                    m.setLed(1, x, y - 16, true);
                else if(y >= 24 && y < 32)
                    m.setLed(0, x, y - 24, true);
            }else if(x >=8 && x < 16){
                if(y < 8)
                    m.setLed(7, x - 8, y, true);
                else if(y >= 8 && y < 16)
                    m.setLed(6, x - 8, y - 8, true);
                else if(y >= 16 && y < 24)
                    m.setLed(5, x - 8, y - 16, true);
                else if(y >= 24 && y < 32)
                    m.setLed(4, x - 8, y - 24, true);
            }
            delay(5);
        }
    }
}