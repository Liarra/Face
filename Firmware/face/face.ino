/* Serial-controlled Face */

#define debug true

int led_pins[16] = {8,A0,A2,A3,A1,6,5,11,10,9,A5,A4,4,7,13,12};
int button_pins[3] = {2,A6,A7};
int speaker_pin = 3;

uint8_t speed = 100; // 0.1s time granularity, all time is expressed as a multiple of this

// Behaviour is modelled as a Discrete Time Markov Chain (DTMC) extended with fixed per-state delay
// TODO: evaluate extending to CTMC plus minimum delay
struct State {
    uint16_t face;
    uint8_t delay;
    uint8_t sound; // Reserved, index in sounds table
};
struct Transition {
    uint8_t from;
    uint8_t next;
    uint8_t p;
};

State states[32];
Transition transitions[64];
uint8_t state_count = 0;
uint8_t transition_count = 0;

// Linear feedback shift register, a 16-bit PRNG with period 2^16
uint16_t lfsr = 0xACE1;
void lfsrNext(void) { // Advance LFSR, gets next 16 pseudorandom bits
    // taps: 16 14 13 11; characteristic polynomial: x^16 + x^14 + x^13 + x^11 + 1
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400);
}

// Update face, to be called every speed ms
void faceAdvance(void) {
    static uint8_t delay;
    static uint8_t cur_state = 0;

    int i;
    
    // Stay in current state for the right duration
    if (delay > 0) {
        delay--;
        return;
    }
    
    // Transition into next state, DTMC logic
    uint16_t ptotal = 0; // sum of probabilities
    for (i=0; i<transition_count; i++) {
        if (transitions[i].from == cur_state) {
            ptotal += transitions[i].p;
        }
    }
    lfsrNext();
    uint16_t pnow = lfsr % ptotal; // Uniform random choice
    for (i=0; i<transition_count; i++) {
        if (transitions[i].from == cur_state) {
            //~ printf("eval %x, %d, %d, %d\r\n", lfsr, ptotal, transitions[i].p, pnow);
            if (transitions[i].p > pnow) {
                if (debug) printf("Transition %d -(%d)-> %d\r\n", cur_state, i, transitions[i].next);
                cur_state = transitions[i].next;
                faceOutput(states[cur_state].face);
                delay = states[cur_state].delay;
                break;
            }
            pnow -= transitions[i].p;
        }
    }
}

void faceOutput(uint16_t face) {
    for (int i=0; i<16; i++) {
        digitalWrite(led_pins[i], !((face>>i)&1));
    }
}

/* User serial interface */
bool echo = true;
int readLine(int readch, char *buffer, int len) {
    // Process input one readch at a time
    static int pos = 0;
    int rpos;
  
    if (readch > 0) {
        switch (readch) {
            case '\n': // Ignore new-lines
                break;
            case '\b': // Backspace is special
            case 0x7f: // Delete
                if (pos>0) {
                    pos--;
                    buffer[pos] = 0;
                    if (echo) printf("\b \b");
                }
                break;
            case '\r': // Return on CR
                if (echo) printf("\r\n");
                rpos = pos;
                pos = 0;  // Reset position index ready for next time
                return rpos;
            default:
                if (pos < len-1) {
                    buffer[pos++] = readch;
                    buffer[pos] = 0;
                    if (echo) printf("%c", readch);
                }
        }
    }
    // No end of line has been found, so return -1.
    return -1;
}

// Reads the line until the end or as far as it can, does not block
bool readyLine(char *buffer, int len) {
    char c = Serial.read();
    while (c>0) {
        if (readLine(c, buffer, len) > 0) {
            return true;
        }
        c = Serial.read();
    }
    return false;
}

// Blocks until next line is completely read
inline void nextLine(char *buffer, int len) {
    while (readLine(Serial.read(), buffer, len) <= 0);
}

// Direct printf to Serial.write()
static FILE uartout = {0};
static int uartPutchar(char c, FILE *stream) {
    Serial.write(c);
    return 0;
}

bool startsWith(char *pat, char *str) {
    // true iff str starts with pat
    int i = 0;
    while ((pat[i]==str[i]) && (pat[i]!=0) && (str[i]!=0))
        i++;
    return pat[i]==0;
}

bool scanHex(char *pos, uint8_t *buf, uint16_t bytes) {
    /* WARNING: no sanitization or error-checking whatsoever */
    for(int i=0; i<bytes; i++) {
        sscanf(pos, "%2hhx", &buf[i]);
        pos += 2 * sizeof(char);
    }
}


void lightFaceFromHex(char * hexstring){
    int number = (int)strtol(hexstring, NULL, 16);
    
    for(int i=0;i<16;i++){
        if(number & ( 1<<i)) {
            digitalWrite(led_pins[i], LOW);
        }
        else{
            digitalWrite(led_pins[i], HIGH);
        }
    }
}

void setup() {
    // Initialize serial and printf
    Serial.begin(9600);
    fdev_setup_stream(&uartout, uartPutchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &uartout;
    
    for (int i=0; i<16; i++) {
        pinMode(led_pins[i], OUTPUT);
        digitalWrite(led_pins[i], HIGH); // All off
    }

    faceOutput(states[0].face);
    
    /*/ Test segments
    int i=0;
    while (1) {
        digitalWrite(led_pins[i], HIGH);
        i = (i+1) % 16;
        digitalWrite(led_pins[i], LOW);
        delay(500);
    } //*/
    
    // Test initial behaviour
    states[0] = {0b1101000001101011, 10, 0};
    states[1] = {0b1001000110010101, 10, 0};
    state_count=2;
    transitions[0] = {0, 1, 30};
    transitions[1] = {1, 0, 10};
    transitions[2] = {0, 0, 10};
    transitions[3] = {1, 1, 10};
    transition_count=4;
    // */
}

void loop() {
    char input[80];
    if (readyLine(input, sizeof(input))) {
        if (startsWith("face ", input)) {
            lightFaceFromHex(input+5);
        } else if (startsWith("on ", input)) { // To remove
            digitalWrite(led_pins[atoi(input+3)], LOW);
        } else if (startsWith("off ", input)) { // To remove
            digitalWrite(led_pins[atoi(input+4)], HIGH);
        } else if (startsWith("echo", input)) {
            echo = !echo;
        } else if (startsWith("speed ", input)) {
            speed = strtol(input+6, NULL, 10);
        } else if (startsWith("forget", input)) {
            state_count = 0;
            transition_count = 0;
        } else if (startsWith("reset", input)) {
            asm volatile ("  jmp 0");
        } else {
            // TODO: keep this updated!
            printf("Invalid command. Try face, echo, speed, forget, reset.\r\n");
        }
    }
    faceAdvance();
    delay(speed);
}
