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

uint8_t faceAddState(uint16_t face, uint8_t delay=1, uint8_t sound=0) {
    if (state_count >= sizeof(states)) return 255;
    states[state_count] = {face, delay, sound};
    return state_count++;
}
uint8_t faceAddTransition(uint8_t from, uint8_t next, uint8_t p) {
    if (transition_count >= sizeof(transitions)) return 255;
    transitions[transition_count] = {from, next, p};
    return transition_count++;
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


/* Utility functions */

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

// Get the next token from the current string
#define token() strtok(NULL, " ")


/* Commands given over serial interface */

void commandFace(void) {
    char *arg1 = token();
    uint16_t face;
    if (strlen(arg1) == 16) {
        face = strtol(arg1, NULL, 2);
    } else {
        face = strtol(arg1, NULL, 16);
    }
    faceOutput(face);
}

uint8_t commandState(void){
    char *face = token();
    char *duration = token();
    char *sound = token();
    return faceAddState(strtol(face, NULL, 16), atoi(duration), atoi(sound));
}

uint8_t commandTrans(void){
    char *state1 = token();
    char *state2 = token();
    char *prob = token();
    return faceAddTransition(atoi(state1), atoi(state2), atoi(prob));
}


/* Main program logic */

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
    uint8_t a = faceAddState(0b1101000001101011, 10, 0);
    uint8_t b = faceAddState(0b1001000110010101, 10, 0);
    faceAddTransition(a, b, 30);
    faceAddTransition(b, a, 10);
    faceAddTransition(a, a, 10);
    faceAddTransition(b, b, 10);
    // */
}

void loop() {
    char input[80];
    if (readyLine(input, sizeof(input))) {
        char *cmd;
        cmd = strtok(input, " ");
        if (strcmp("face", cmd)==0) {
            commandFace();
        } else if (strcmp("trans", cmd)==0) {
            commandTrans();
        } else if (strcmp("state", cmd)==0) {
            commandState();
        } else if (strcmp("forget", cmd)==0) {
            state_count = 0;
            transition_count = 0;
        //~ } else if (strcmp("", cmd)==0) {
            //~ docmd
        } else if (strcmp("speed", cmd)==0) {
            speed = strtol(token(), NULL, 10);
        } else if (strcmp("echo", cmd)==0) {
            echo = !echo;
        } else if (strcmp("reset", cmd)==0) {
            asm volatile ("  jmp 0");
        } else {
            // TODO: keep this updated!
            printf("Invalid command. Try face, echo, speed, forget, reset.\r\n");
        }
    }
    faceAdvance();
    delay(speed);
}
