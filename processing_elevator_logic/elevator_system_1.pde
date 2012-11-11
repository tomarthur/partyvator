/*

Partyvator Elevator System - r4
 >> accelerometer reading from arduino via serial
 >> audio playback with Minim library
 >> LED light control via serial to second arduino

PComp Midterm 2012 - NYU ITP
Tom Arthur & Alexandra Diracles

*/

import processing.serial.*;             // Processing Serial Library
import ddf.minim.*;                         // Music Library

// for serial
Serial arduinoAccel;                // Serial port for accelerometer
Serial arduinoLights;               // Serial port for LED lights
boolean firstContact = false;       // Whether we've heard from the microcontroller
int zVal;                           // zVal of accelerometer
// int lastZ = 0;                             // use to filter values out of bounds
// int tempZ = 0;                           // use to filter values out of bounds

// for music
boolean fading = false;                   // Is music fading?
float currentVolume;
Minim minim;
AudioPlayer player;

// Elevator movement logic
boolean motionLockout = false;      // Motion in progress - used to prevent looping of sequence start/false starts
boolean motionUp = false;           // Motion up started
boolean motionDown = false;         // Motion down started

int upCounter = 0;                  // Count the number of readings indicating upward movement
int downCounter = 0;                // Count hte number of readings indicating downward movement

int accelUpLim = 624;               // ACCELEROMETER values indicating moving down
int accelDownLim = 610;             // ACCELEROMETER values indicating moving up

boolean cntStart = false;           // Has timer started?
int savedTime;                      // Timer for reset
int totalTime = 5000;               // Reset wait period
int passedTime = 0;                 // Time passed

void setup()
{
    size (500, 100);
    // Print a list of the serial ports, for debugging purposes:
    println(Serial.list());
    String Accel = Serial.list()[4];  //accelerometer
    String Lights = Serial.list()[6]; //LEDs
    arduinoAccel = new Serial(this, Accel, 9600);
    arduinoLights = new Serial(this, Lights, 9600);
    minim = new Minim(this);
    // load a file, give the AudioPlayer buffers that are 2048 samples long
    player = minim.loadFile("test3.mp3", 2048);
    player.play();                  // play the file
    player.loop();                  // loop (might just need this and not play too)
    player.shiftGain(0, -80, 0);    // turn volume down
    println ("setup done. ready to make elevator go.");
    // set up for recording data
    // output = createWriter("elevator_playrest1.txt");
}

void draw()
{
    if (motionLockout == false && motionUp == true)                         // check to see if motion up begins & start sequences
    {
        motionLockout = true;                                               // lockout from a false start
        background(#2B045C);
        text("the elevator has begun moving up", 10, 10);
        // output.println("the elevator has begun motion moving up");
        lightScript('1');                                                   // start light script
        musicFadeIn();                                                      // fade music in
    }
    else if (motionLockout == false && motionDown == true)                  // check to see if motion up begins & start sequences
    {
        motionLockout = true;                                               // lockout from a false start
        background(#2B045C);
        text("the elevator has begun moving down", 10, 10);
        // output.println("the elevator has begun motion moving down");
        lightScript('1');                                                   // start light script
        musicFadeIn();                                                      // fade music in
    }
    if (motionLockout == true && motionUp == true && motionDown == true ||  // end sequence when elevator stops (detected by movement in oposite direction of start movement)
            motionLockout == true && motionDown == true && motionUp == true )
    {
        musicFadeOut();                                                     // fade music out
        lightScript('B');                                                   // break/end light script
        background(#2B045C);
        text("the elevator is stopping", 10, 10);
        if (cntStart == false)                                              // start timer to wait for elevator to come to a complete stop
        {
            savedTime = millis();
            cntStart = true;
        }
        passedTime = millis() - savedTime;
        // println(savedTime);
        // println(passedTime);
        if (passedTime > totalTime)                                         // after a reasonable wait (5 seconds) reset everything to be ready for the next elevator movement
        {
            upCounter = 0;
            downCounter = 0;
            motionUp = false;
            motionDown = false;
            background(#2B045C);
            text("the elevator has reset", 10, 10);
            savedTime = 0;
            passedTime = 0;
            motionLockout = false;
            cntStart = false;
        }
    }
}


void musicFadeIn()                                 // fade in music
{
    float currentVolume = player.getGain();
    player.shiftGain(currentVolume, 13, 1000);
}

void musicFadeOut()                                // fade out music
{
    float currentVolume = player.getGain();
    player.shiftGain(currentVolume, -80, 1000);
}

void lightScript(char seqeunce)                    // send light sequence to lighting Arduino
{
    arduinoLights.write(seqeunce);
}

void motionDetection(int checkVal)                 // check to see if the zValue indicates motion
{
    if (checkVal >= accelUpLim)                    // accel values go up from rest, so elevator is moving down
    {
        downCounter++;
        // print("upcounter: ");
        // println(upcounter);)
    }
    if (checkVal <= accelDownLim)                  // accel values go down from rest, so elevator is moving up
    {
        upCounter++;
        // print("downcounter: ");
        // println(downCounter);
    }
    if (downCounter >= 6)                         // when 6 values highter than the limit are detected, motion sequence start
    {
        motionUp = true;
    }
    else
    {
        motionUp = false;
    }
    if (upCounter >= 6)                            // when 6 values highter than the limit are detected, motion sequence start
    {
        motionDown = true;
    }
    else
    {
        motionDown = false;
    }
}

void serialEvent(Serial arduinoAccel)            // try removing the serialevent details
{
    // read a byte from the serial port:
    int inByteAccel = arduinoAccel.read();
    // int inByteLights= arduinoLights.read();  // we don't really need to know what's going on with the lights, but just in case
    if (firstContact == false)                  // make contact with accelerometer arduino
    {
        if (inByteAccel == 'r')
        {
            arduinoAccel.clear();              // clear the serial port buffer
            firstContact = true;               // you've had first contact from the microcontroller
            arduinoAccel.write('A');           // ask for more
        }
    }
    else
    {
        // tempZ = inByteAccel;
        zVal = inByteAccel * 4;
        // Send a capital A to request new sensor readings:
        arduinoAccel.write('A');
        // if (tempZ > (lastZ + 200) || tempZ < (lastZ - 200)) {      // prevent eronious values from fucking stuff up
        //   zVal = lastZ;
        // }
        // else {
        // zVal = tempZ;
        // }
        // println(zVal);
        motionDetection(zVal); //send zVal to be checked for motion
        println(zVal);
    }
}


void stop()
{
    // always close Minim audio classes when you are done with them
    player.close();
    // always stop Minim before exiting
    minim.stop();
    super.stop();
}


