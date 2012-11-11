/*

Partyvator LED Strip Control - r4
PComp Midterm 2012 - NYU ITP
Tom Arthur & Alexandra Diracles

Based off Adafruit example test code for LPD8806-based RGB LED strip

Fast and smooth random colour patterns for a LPD8066 addressable RGB LED strip.
By happyinmotion (jez.weston@yahoo.com)

Simplex noise code taken from Stephen Carmody's Java implementation at:
http://stephencarmody.wikispaces.com/Simplex+Noise

*/

#include "LPD8806.h"
#include "SPI.h"

// Strip variables:
const int LEDs_in_strip = 100;      // Used for noise based scrips, not working with more than 100 LEDs
const int LEDs_for_simplex = 6;

// Number of RGB LEDs in strand:
int nLEDs = 135;

//  Hardware SPI for faster writes Data = pin 11, clock = pin 13
LPD8806 strip = LPD8806(nLEDs);


// Extra fake LED at the end, to avoid fencepost problem.
// It is used by simplex node and interpolation code.
float LED_array_red[LEDs_in_strip + 1];
float LED_array_green[LEDs_in_strip + 1];
float LED_array_blue[LEDs_in_strip + 1];
float LED_array_hue[LEDs_in_strip + 1];
float LED_array_brightness[LEDs_in_strip + 1];

// Perlin noise global variables:
float x1, y1, x2, y2;
// Set up Perlin globals:
//persistence affects the degree to which the "finer" noise is seen
float persistence = 0.25;
//octaves are the number of "layers" of noise that get computed
int octaves = 3;
// Simplex noise global variables:
int i, j, k, A[] = {0, 0, 0};
float u, v, w, s;
static float onethird = 0.333333333;
static float onesixth = 0.166666667;
int T[] = {0x15, 0x38, 0x32, 0x2c, 0x0d, 0x13, 0x07, 0x2a};

int node_spacing = LEDs_in_strip / LEDs_for_simplex;

boolean rainexit = false;

// Simplex noise parameters:
// Useable values for time increment range from 0.005 (barely perceptible) to 0.2 (irritatingly flickery)
// 0.02 seems ideal for relaxed screensaver
float timeinc = 0.02;
// Useable values for space increment range from 0.8 (LEDS doing different things to their neighbours), to 0.02 (roughly one feature present in 15 LEDs).
// 0.05 seems ideal for relaxed screensaver
float spaceinc = 0.1;
// Simplex noise variables:
// So that subsequent calls to SimplexNoisePattern produce similar outputs, this needs to be outside the scope of loop()
float yoffset = 0.0;
float saturation = 1.0;

// For fade in/out effects
const prog_uint8_t Luminance[] PROGMEM =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 16, 19, 22, 25, 29,
    33, 38, 42, 47, 53, 59, 65, 71, 78, 85, 93, 101, 109, 118, 127
};

const prog_uint8_t Brightness[] PROGMEM =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 11, 11, 12, 12, 12, 13,
    13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17,
    18, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 20, 21, 21,
    21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23,
    24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26,
    26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28,
    28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31
};

void setup()
{
    // initialize the serial communication:
    Serial.begin(9600);
    // Start up the LED strip
    strip.begin();
    // Update the strip, to start they are all 'off'
    strip.show();
    // fade into white standby color
    FadeUpToWhite();
}

void loop()
{
    switch (Serial.read())
    {
        case '1':                            // start first sequence
            FadeDownToBlack();
            //        FadeUpToBlue();
            sequence1();
            break;
        case '2':                            // start second sequence
            FadeDownToBlack();
            sequence2();
            break;
            //  case '3':                    // start third sequence
            //  FadeDownToBlack();
            //    sequence3();
            //    break;
    }
}

void sequence1()
{
    boolean exit = false;
    while (Serial.read() != 'B')        // wait for 'B' on serial to end sequnece
    {
        int repeats = 200;
        for ( int i = 0; i < repeats; i++ )
        {
            SimplexNoisePatternInterpolated( spaceinc, timeinc, yoffset);
            yoffset += timeinc;
            if (Serial.read() == 'B')
            {
                exit = true;
                break;
            }
        }
        delay(500);
        if (Serial.read() == 'B' || exit == true)
        {
            lightReset(); // reset the lights and wait for next sequence
            break;
        }
    }
}

void sequence2()                      
{
    boolean ended = true;
    while (Serial.read() != 'B')
    {
        ended = false;
        rainbowCycle(0);
        if (Serial.read() == 'B')
        {
            lightReset(); // reset the lights and wait for next sequence
            ended = true;
            break;
        }
    }
    if (ended == false)
    {
        lightReset();
    }
}

//void sequence3() {
//while (Serial.read() != 'B'){
//
//wave(strip.Color(20, 0, 90), 1, 0);
//
//  }
//
//lightReset(); // reset the lights and wait for next sequence
//
//}

void lightReset()
{
    FadeDownToBlack();
    FadeUpToWhite();
}


void setAll(uint32_t c)
{
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, c);
    }
    strip.show();
}

// Slightly different, this one makes the rainbow wheel equally distributed
// along the chain
void rainbowCycle(uint8_t wait)
{
    uint16_t i, j;
    for (j = 0; j < 384 * 1; j++)     // 5 cycles of all 384 colors in the wheel
    {
        for (i = 0; i < strip.numPixels(); i++)
        {
            // tricky math! we use each pixel as a fraction of the full 384-color
            // wheel (thats the i / strip.numPixels() part)
            // Then add in j which makes the colors go around per pixel
            // the % 384 is to make the wheel cycle around
            strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
        }
        strip.show();   // write all the pixels out
        delay(wait);
    }
}

uint32_t Wheel(uint16_t WheelPos)
{
    byte r, g, b;
    switch (WheelPos / 128)
    {
        case 0:
            r = WheelPos % 18; // red down
            // Serial.println(r);
            g = 0;
            b = WheelPos % 128;
            break;
        case 1:
            g = 0;   //green down
            b = 127 - WheelPos % 128;      //blue up
            r = 0;                  //red off
            break;
        case 2:
            b = WheelPos % 128;  //blue down
            r = 0;   //red up
            g = 0;                  //green off
            break;
    }
    return (strip.Color(r, g, b));
}

// Sine wave effect
#define PI 3.14159265
void wave(uint32_t c, int cycles, uint8_t wait)
{
    float y;
    byte  r, g, b, r2, g2, b2, r3, g3, b3;
    // Need to decompose color into its r, g, b elements
    g = (c >> 16) & 0x7f;
    r = (c >>  8) & 0x7f;
    b =  c        & 0x7f;
    for (int x = 0; x < (strip.numPixels() * 5); x++)
    {
        for (int i = 0; i < strip.numPixels(); i++)
        {
            y = sin(PI * (float)cycles * (float)(x + i) / (float)strip.numPixels());
            if (y >= 0.0)
            {
                // Peaks of sine wave are white
                y  = 1.0 - y; // Translate Y to 0.0 (top) to 1.0 (center)
                r2 = 127 - (byte)((float)(127 - r) * y);
                g2 = 127 - (byte)((float)(127 - g) * y);
                b2 = 127 - (byte)((float)(127 - b) * y);
            }
            else
            {
                // Troughs of sine wave are black
                y += 1.0; // Translate Y to 0.0 (bottom) to 1.0 (center)
                r2 = (byte)((float)r * y);
                g2 = (byte)((float)g * y);
                b2 = (byte)((float)b * y);
            }
            r3 = map(r2, 0, 127, 0, 20);
            g3 = map(g2, 0, 127, 0, 127);
            b3 = map(b2, 0, 127, 0, 127);
            //Serial.println(r3);
            strip.setPixelColor(i, r3, g3, b3);
        }
        strip.show();
        delay(wait);
    }
}

void FadeUpToWhite()
{
    int j;
    for (j = 0; j < 32; j++)
    {
        IncreaseBrightness();
        delay(10);
    }
}

void FadeDownToBlack()
{
    int j;
    for (j = 0; j < 32; j++)
    {
        DecreaseBrightness();
        delay(10);
    }
}

void IncreaseBrightness()
{
    int i;
    for (i = 0; i < strip.numPixels(); i++)
    {
        uint32_t oldValue, newValue;
        uint8_t luminance, brightness;
        oldValue = strip.getPixelColor(i);
        luminance = (oldValue >> 16) & 0x7F;  // green
        if (luminance < 100)
        {
            brightness = pgm_read_byte( & Brightness[luminance]);
            brightness++;
            luminance = pgm_read_byte( & Luminance[brightness]);
        }
        newValue = luminance;
        luminance = (oldValue >> 8) & 0x7F;  // red
        if (luminance < 100)
        {
            brightness = pgm_read_byte( & Brightness[luminance]);
            brightness++;
            luminance = pgm_read_byte( & Luminance[brightness]);
        }
        newValue = (newValue << 8) + luminance;
        luminance = oldValue & 0x7F;  // blue
        if (luminance < 100)
        {
            brightness = pgm_read_byte( & Brightness[luminance]);
            brightness++;
            luminance = pgm_read_byte( & Luminance[brightness]);
        }
        newValue = (newValue << 8) + luminance;
        strip.setPixelColor(i, newValue);
    }
    strip.show();
}


void DecreaseBrightness()
{
    int i;
    for (i = 0; i < strip.numPixels(); i++)
    {
        uint32_t oldValue, newValue;
        uint8_t luminance, brightness;
        oldValue = strip.getPixelColor(i);
        luminance = (oldValue >> 16) & 0x7F;  // green
        if (luminance)
        {
            brightness = pgm_read_byte( & Brightness[luminance]);
            brightness--;
            luminance = pgm_read_byte( & Luminance[brightness]);
        }
        newValue = luminance;
        luminance = (oldValue >> 8) & 0x7F;  // red
        if (luminance)
        {
            brightness = pgm_read_byte( & Brightness[luminance]);
            brightness--;
            luminance = pgm_read_byte( & Luminance[brightness]);
        }
        newValue = (newValue << 8) + luminance;
        luminance = oldValue & 0x7F;  // blue
        if (luminance)
        {
            brightness = pgm_read_byte( & Brightness[luminance]);
            brightness--;
            luminance = pgm_read_byte( & Luminance[brightness]);
        }
        newValue = (newValue << 8) + luminance;
        strip.setPixelColor(i, newValue);
    }
    strip.show();
}

void SimplexNoisePatternInterpolated( float spaceinc, float timeinc, float yoffset)
{
    // Calculate simplex noise for LEDs that are nodes:
    // Store raw values from simplex function (-0.347 to 0.347)
    float xoffset = 0.0;
    for (int i = 0; i <= LEDs_in_strip; i = i + node_spacing)
    {
        xoffset += spaceinc;
        LED_array_red[i] = SimplexNoise(xoffset, yoffset, 0);
        LED_array_green[i] = SimplexNoise(xoffset, yoffset, 1);
        LED_array_blue[i] = SimplexNoise(xoffset, yoffset, 2);
    }
    // Interpolate values for LEDs between nodes
    for (int i = 0; i < LEDs_in_strip; i++)
    {
        int position_between_nodes = i % node_spacing;
        int last_node, next_node;
        // If at node, skip
        if ( position_between_nodes == 0 )
        {
            // At node for simplex noise, do nothing but update which nodes we are between
            last_node = i;
            next_node = last_node + node_spacing;
        }
        // Else between two nodes, so identify those nodes
        else
        {
            // And interpolate between the values at those nodes for red, green, and blue
            float t = float(position_between_nodes) / float(node_spacing);
            float t_squaredx3 = 3 * t * t;
            float t_cubedx2 = 2 * t * t * t;
            LED_array_red[i] = LED_array_red[last_node] * ( t_cubedx2 - t_squaredx3 + 1.0 ) + LED_array_red[next_node] * ( -t_cubedx2 + t_squaredx3 );
            LED_array_green[i] = LED_array_green[last_node] * ( t_cubedx2 - t_squaredx3 + 1.0 ) + LED_array_green[next_node] * ( -t_cubedx2 + t_squaredx3 );
            LED_array_blue[i] = LED_array_blue[last_node] * ( t_cubedx2 - t_squaredx3 + 1.0 ) + LED_array_blue[next_node] * ( -t_cubedx2 + t_squaredx3 );
        }
    }
    // Convert values from raw noise to scaled r,g,b and feed to strip
    for (int i = 0; i < LEDs_in_strip; i++)
    {
        int r = int(LED_array_red[i] * 403 + 16);
        int g = int(LED_array_green[i] * 403 + 16);
        int b = int(LED_array_blue[i] * 403 + 16);
        if ( r > 20 )
        {
            r = 20;
        }
        else if ( r < 0 )
        {
            r = 0;    // Adds no time at all. Conclusion: constrain() sucks.
        }
        if ( g > 127 )
        {
            g = 127;
        }
        else if ( g < 0 )
        {
            g = 0;
        }
        if ( b > 127 )
        {
            b = 127;
        }
        else if ( b < 20 )
        {
            b = 20;
        }
        strip.setPixelColor(i, r, 0, b);
    }
    // Update strip
    strip.show();
}



/*****************************************************************************/
// Simplex noise code:
// From an original algorythm by Ken Perlin.
// Returns a value in the range of about [-0.347 .. 0.347]
float SimplexNoise(float x, float y, float z)
{
    // Skew input space to relative coordinate in simplex cell
    s = (x + y + z) * onethird;
    i = fastfloor(x + s);
    j = fastfloor(y + s);
    k = fastfloor(z + s);
    // Unskew cell origin back to (x, y , z) space
    s = (i + j + k) * onesixth;
    u = x - i + s;
    v = y - j + s;
    w = z - k + s;;
    A[0] = A[1] = A[2] = 0;
    // For 3D case, the simplex shape is a slightly irregular tetrahedron.
    // Determine which simplex we're in
    int hi = u >= w ? u >= v ? 0 : 1 : v >= w ? 1 : 2;
    int lo = u < w ? u < v ? 0 : 1 : v < w ? 1 : 2;
    return k_fn(hi) + k_fn(3 - hi - lo) + k_fn(lo) + k_fn(0);
}


int fastfloor(float n)
{
    return n > 0 ? (int) n : (int) n - 1;
}


float k_fn(int a)
{
    s = (A[0] + A[1] + A[2]) * onesixth;
    float x = u - A[0] + s;
    float y = v - A[1] + s;
    float z = w - A[2] + s;
    float t = 0.6f - x * x - y * y - z * z;
    int h = shuffle(i + A[0], j + A[1], k + A[2]);
    A[a]++;
    if (t < 0) return 0;
    int b5 = h >> 5 & 1;
    int b4 = h >> 4 & 1;
    int b3 = h >> 3 & 1;
    int b2 = h >> 2 & 1;
    int b = h & 3;
    float p = b == 1 ? x : b == 2 ? y : z;
    float q = b == 1 ? y : b == 2 ? z : x;
    float r = b == 1 ? z : b == 2 ? x : y;
    p = b5 == b3 ? -p : p;
    q = b5 == b4 ? -q : q;
    r = b5 != (b4 ^ b3) ? -r : r;
    t *= t;
    return 8 * t * t * (p + (b == 0 ? q + r : b2 == 0 ? q : r));
}


int shuffle(int i, int j, int k)
{
    return b(i, j, k, 0) + b(j, k, i, 1) + b(k, i, j, 2) + b(i, j, k, 3) + b(j, k, i, 4) + b(k, i, j, 5) + b(i, j, k, 6) + b(j, k, i, 7);
}


int b(int i, int j, int k, int B)
{
    return T[b(i, B) << 2 | b(j, B) << 1 | b(k, B)];
}


int b(int N, int B)
{
    return N >> B & 1;
}


void AllOff()
{
    // Reset LED strip
    strip.begin();
    strip.show();
}
