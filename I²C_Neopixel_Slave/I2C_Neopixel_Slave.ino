
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#include <Wire.h>

#define SLAVEID 4
#define JEWEL    0
#define RING16   1
#define STRIP216 0

#define NUMBLOCKS 5
// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        4 // On Trinket M0

// How many NeoPixels are attached to the Arduino?
#if   JEWEL
#define NUMPIXELS   7   // Popular NeoPixel ring size
#elif RING16
#define NUMPIXELS  16   // Popular NeoPixel ring size
#elif STRIP216
#define NUMPIXELS 216   // Popular NeoPixel ring size
#endif


#define HSV_PINK   64000
#define HSV_AQUA   33000
#define HSV_ORANGE  4000

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
const uint32_t RED      = pixels.Color(255,  0, 0) ;
const uint32_t BLACK    = pixels.Color(  0,  0, 0) ;
const uint32_t ORANGE   = pixels.Color(255, 34, 0) ;
const uint32_t YELLOW   = pixels.Color(255,170, 0) ;
const uint32_t GREEN    = pixels.Color(  0,255, 0) ;
const uint32_t CYAN     = pixels.Color(  0,255,255) ;
const uint32_t BLUE     = pixels.Color(  0,  0,255) ;
const uint32_t VIOLET   = pixels.Color(153,  0,255) ;
const uint32_t MAGENTA  = pixels.Color(255,  0, 51) ;
const uint32_t PINK     = pixels.Color(255, 51,119) ;
const uint32_t AQUA     = pixels.Color( 85,125,255) ;
const uint32_t WHITE    = pixels.Color(255,255,255) ;

// save some unsigned ints
const uint32_t palette[] = { BLACK, RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, VIOLET, MAGENTA, PINK, AQUA, WHITE } ;

enum funcs {
    OFF = 0   ,
    ON        ,
    BLINK     ,
    RAINBOW   ,
    BREATH    ,
    SINGLEPIX ,
    SPARKLE   ,
    SPINWHEEL ,
    EMPTY = 99,
} ;

enum cmds {
    CMD_NOP = 0   ,
    CMD_SETBLOCK  ,
    CMD_SETBLKFUNC,
    CMD_SETBLKCOLOR,
    CMD_CLRBLOCK,
    CMD_SETPIXEL  ,
    CMD_SETPIXELS ,
} ;


struct block {
    uint16_t  first ;
    uint16_t  num ;
    funcs func ;
    uint16_t  dly ;
    uint32_t color ;
    bool     state ;
    unsigned long timer ;
    unsigned long aux ;
} ;

struct block blocks[NUMBLOCKS] ;
uint8_t rcvbuf[8] ;
bool cmd_rcved = false ;

void setup() {
    // put your setup code here, to run once:
    Wire.begin(SLAVEID);          // join i2c bus with address SLAVEID
    Wire.onReceive(receiveEvent); // register event
    Serial.begin(9600);           // start serial for output
    blocks[0].first =   0 ;
    blocks[0].num   = NUMPIXELS ;
    blocks[0].func  =  RAINBOW ;
    blocks[0].dly   =   50 ;
    blocks[0].timer =   0 ;
    blocks[0].color =  ORANGE ;
    blocks[0].state = false ;
    blocks[1] = {   0, 16, SPINWHEEL,  100,  WHITE, false } ;
    blocks[2] = {   4,  0, OFF,  0,MAGENTA, false } ;
    blocks[3] = {   7,  0, OFF,  0, ORANGE, false } ;
    blocks[4] = {  12,  0, OFF,  0, YELLOW, false } ;
    for ( uint8_t i=5; i<NUMBLOCKS ; i++ ) {
        blocks[i] = { 0, 0, OFF, 0, BLACK, false } ; 
    }
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    pixels.clear();
    pixels.setBrightness(50) ;
    pixels.show() ;
}

long firstPixelHue ;

void loop() {
  // put your main code here, to run repeatedly:
uint8_t  _blknum ;
uint8_t  _pixfst ;
uint8_t  _pixnum ;
funcs    _func   ;
uint16_t _delay  ;
uint32_t _color  ;
bool     _state  ;
                    
    if (cmd_rcved) {
        cmd_rcved = false ;
        Serial.print  ("Cmd: ") ;
        Serial.println(rcvbuf[0]) ;
        switch (rcvbuf[0]) {
            case CMD_SETBLOCK:
                    _blknum = rcvbuf[1] ;
                    _pixfst = rcvbuf[2] ;
                    _pixnum = rcvbuf[3] ;
                    _func   = (funcs)rcvbuf[4] ;
                    _delay  = rcvbuf[5]*10 ;    // delay 10ms to 2550ms
                    _color  = palette[rcvbuf[6]] ;       // color palette indexing
                    blocks[_blknum] = { _pixfst, _pixnum, _func, _delay, _color, false, millis(), 0 } ;
                break;
            case CMD_SETBLKFUNC:
                    _blknum = rcvbuf[1] ;
                    _func   = (funcs)rcvbuf[2] ;
                    _delay  = rcvbuf[3]*10 ;    // delay 10ms to 2550ms
                    blocks[_blknum].func = _func  ;
                    blocks[_blknum].dly  = _delay ;
                    blocks[_blknum].timer= millis() ;
                    blocks[_blknum].aux  = 0 ;
                break;
            case CMD_SETBLKCOLOR:
                    _blknum = rcvbuf[1] ;
                    _color  = palette[rcvbuf[2]] ;       // color palette indexing
                    blocks[_blknum].color = _color  ;
                break;
            case CMD_CLRBLOCK:
                    _blknum = rcvbuf[1] ;
                    blocks[_blknum].num = 0 ;
                break;
            case CMD_SETPIXEL:
                    _pixfst  = rcvbuf[1] ;
                    _pixnum  = 1 ;
                    _color   = palette[rcvbuf[2]] ;       // color palette indexing
                    pixels.setPixelColor(_pixfst, _color);
                break;
            case CMD_SETPIXELS:
                    _pixfst  = rcvbuf[1] ;
                    _pixnum  = rcvbuf[2] ;
                    _color   = palette[rcvbuf[3]] ;       // color palette indexing
                    pixels.fill(_color, _pixfst, _pixnum) ;
                break;
            default:
                // Statement(s)
                break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
        }
    }
    for ( uint8_t m=0 ; m < NUMBLOCKS ; m++ ) {
#if DEBUG
            Serial.print  ("blk: ") ;
            Serial.print  (m) ;
            Serial.print  ("  num: ") ;
            Serial.print  (blocks[m].num) ;
            Serial.print  ("  fnc: ") ;
            Serial.print  (blocks[m].func) ;
            Serial.print  ("  dly: ") ;
            Serial.print  (blocks[m].dly) ;
            Serial.print  ("  clr: ") ;
            Serial.print  (blocks[m].color) ;
            Serial.print  ("  tmr: ") ;
            Serial.print  (blocks[m].timer) ;
            Serial.print  ("  sts: ") ;
            Serial.print  (blocks[m].state) ;
            Serial.println("") ;
#endif
        if ( blocks[m].num != 0 ) {
            int16_t p, q, _aux, _fst, _clr ;
            switch (blocks[m].func) {
                case OFF:
                    pixels.fill(BLACK, blocks[m].first, blocks[m].num) ;
                    blocks[m].state = false ;
                    break ;   
                case ON:
                    pixels.fill(blocks[m].color, blocks[m].first, blocks[m].num) ;
                    blocks[m].state = true ;
                    break ;   
                case BLINK:
                    if ( millis() >= (blocks[m].timer + blocks[m].dly) ) {
                        blocks[m].timer += blocks[m].dly ;
                        blocks[m].state = !blocks[m].state ;
                    }
                    if ( blocks[m].state ) {
                        pixels.fill(blocks[m].color, blocks[m].first, blocks[m].num) ;
                    } else {
                        pixels.fill(BLACK, blocks[m].first, blocks[m].num) ;
                    }
                    break ;   
                case RAINBOW:
                    if ( millis() >= (blocks[m].timer + blocks[m].dly) ) {
                        blocks[m].timer += blocks[m].dly ;
                     // for(long firstPixelHue = 0; firstPixelHue < 3*65536; firstPixelHue += 256) {
                        firstPixelHue = blocks[m].aux ; blocks[m].aux += 256 ;
                        for ( int i=blocks[m].first ; i < blocks[m].num ; i++ ) { // For each pixel in strip...
                          int pixelHue = firstPixelHue + (i * 65536L / blocks[m].num );
                          pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
                        }
                    //  pixels.show(); // Update strip with new contents
                    //   delay(blocks[m].dly);  // Pause for a moment
                    // }                
                    }
                    break ;             
                case BREATH:
                    _aux = blocks[m].aux ;
                    q = _aux < 249 ? 0 : 2 ;
                    p = _aux + ( 249 - _aux ) * q ;
                    pixels.fill(pixels.ColorHSV(HSV_ORANGE,255,p), 0, pixels.numPixels());
                    blocks[m].aux++ ;
                    if ( blocks[m].aux >= 488 ) {
                       blocks[m].aux = 10 ;
                    }
                    break ;
                case EMPTY:
                    break ;
                case SPARKLE:
                    if ( millis() >= (blocks[m].timer + blocks[m].dly) ) {
                        blocks[m].timer += blocks[m].dly ;
                        pixels.setPixelColor(_fst+blocks[m].aux, BLACK) ;
                        blocks[m].aux = random(blocks[m].num) ;
                    }
                        pixels.setPixelColor(_fst+blocks[m].aux, WHITE) ;
                        blocks[m].state = true ; 
                     //   delay(20) ;
                    break ;
                case SINGLEPIX:
                    _aux = blocks[m].aux ;
                    _fst = blocks[m].first ;
                    if ( _aux == 0 ) {
                        pixels.setPixelColor(_fst, RED) ;
                        blocks[m].state = false ;
                    }
                    if ( blocks[m].state ) {
                        pixels.setPixelColor(_fst+_aux, BLACK) ;
                        blocks[m].state = false ;                        
                    }
                    blocks[m].aux++ ;
                    if ( blocks[m].aux >= blocks[m].num ) {
                        blocks[m].aux = 0 ;
                    } else {
                        pixels.setPixelColor(_fst+blocks[m].aux, blocks[m].color) ;
                        blocks[m].state = true ;                                                
                    }
                    delay(100);
                    break ;
                case SPINWHEEL:  // Spinny wheel (4 LEDs on at a time)
                    if ( millis() >= (blocks[m].timer + blocks[m].dly) ) {
                        blocks[m].timer += blocks[m].dly ;
                        blocks[m].aux++ ;
                    }
                    _aux = blocks[m].aux ;
                    _fst = blocks[m].first ;
                    _clr = blocks[m].color ;
                    for( uint8_t i=0 ; i<blocks[m].num ; i++ ) {  // For each LED...
                        uint32_t c = BLACK ;                      // Assume pixel will be "off" color
                        if ((( _aux + i) % 6 ) < 1 ) {               // For each 8 pixels, 2 will be...
                            c = blocks[m].color ;                 // ...assigned the current color
                            pixels.setPixelColor(_fst + i, c);  // Set color of pixel 'i'
                        }
//                        pixels.setPixelColor(_fst + i, c);  // Set color of pixel 'i'
                    }
                    // pixels.show();                 // Refresh LED states
                    // delay(50);                     // 50 millisecond delay
                    break ;
                default:
                    // Statement(s)
                    break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
            }
        }
    }
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(5) ;
}


// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
uint8_t k = 0 ;
    while(Wire.available()) {     // loop through all but the last
        rcvbuf[k] = Wire.read();    // receive byte as a character
//        Serial.print(rcvbuf[k]);    // print the character
        k++ ;
    }
//    Serial.println(":");         // print newline
    cmd_rcved = true ;
}
