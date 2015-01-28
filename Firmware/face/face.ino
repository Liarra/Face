/* Serial-controlled Face */

int led_pins[16] = {8,A0,A2,A3,A1,6,5,11,10,9,A5,A4,4,7,13,12};
int button_pins[3] = {2,A6,A7};
int speaker_pin = 3;

uint8_t speed = 100; // 0.1s time granularity, all time is expressed as a multiple of this

// Behaviour is modelled as a Discrete Time Markov Chain (DTMC) extended with fixed per-state delay
struct State {
    uint16_t face;
    uint8_t delay;
    uint8_t sound;
} state;

struct Transition {
    uint8_t from;
    uint8_t p;
    uint8_t next;
} transition;

State states[32];
Transition transitions[64];
uint8_t state_count = 0;
uint8_t transition_count = 0;

// Linear feedback shift register, a 16-bit PRNG with period 2^16
uint16_t lfsr = 0;
void lfsr_next(void) { // Advance LFSR, gets next 16 pseudorandom bits
    // taps: 16 14 13 11; characteristic polynomial: x^16 + x^14 + x^13 + x^11 + 1
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400);
}

// Update face, to be called every speed ms
void face_advance(void) {
    static uint8_t delay;
    static uint8_t cur_state = 0;
    
    //~ printf("delay = %d\r", delay);
    
    int i;
    uint16_t ptotal = 0; // sum of probabilities
    uint16_t pnow = 0;
    
    // Stay in current state for the right duration
    if (delay > 0) {
        delay--;
        return;
    }
    printf("transition\r\n");
    
    // Transition into next state, Markov chain logic
    for (i=0; i<transition_count; i++) {
        if (transitions[i].from == cur_state) {
            ptotal += transitions[i].p;
        }
    }
    lfsr_next();
    pnow = lfsr % ptotal;
    for (i=0; i<transition_count; i++) {
        if (transitions[i].from == cur_state) {
            if (transitions[i].p > pnow) {
                cur_state = transitions[i].next;
                printf("into %d\r\n", cur_state);
                output_face(states[cur_state].face);
                delay = states[cur_state].delay;
                break;
            }
            pnow -= transitions[i].p;
        }
    }
}

void output_face(uint16_t face) {
    printf("setting face %d\r\n", face);
    for (int i=0; i<16; i++) {
        printf(" %d", i, (face>>i)&1);
        digitalWrite(led_pins[i], (face>>i)&1);
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
inline bool readyLine(char *buffer, int len) {
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
static int uart_putchar(char c, FILE *stream) {
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
    fdev_setup_stream(&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &uartout;
    
    for (int i=0; i<16; i++) {
        pinMode(led_pins[i], OUTPUT);
        digitalWrite(led_pins[i], HIGH); // All off
    }

    output_face(states[0].face);
    
    /*/ test segments
    int i=0;
    while (1) {
        digitalWrite(led_pins[i], HIGH);
        i = (i+1) % 16;
        digitalWrite(led_pins[i], LOW);
        delay(500);
    } //*/
    
    // Initial behaviour
    states[0] = {0x5555, 10, 0};
    states[1] = {0xAAAA, 10, 0};
    state_count=2;
    transitions[0] = {0, 1, 1};
    transitions[1] = {1, 1, 0};
    transition_count=2;
    // */
}

void loop() {
    char input[80];
    face_advance();
    if (readyLine(input, sizeof(input))) {
        if (startsWith("face ", input)) {
            lightFaceFromHex(&input[5]);
        } else if (startsWith("on ", input)) {
            digitalWrite(led_pins[atoi(&input[2])], LOW);
        } else if (startsWith("off ", input)) {
            digitalWrite(led_pins[atoi(&input[3])], HIGH);
        } else if (startsWith("echo", input)) {
            echo = !echo;
        } else if (startsWith("events", input)) {
            digitalWrite(13, LOW);
        } else if (startsWith("speed ", input)) {
            speed = strtol(&input[6], NULL, 10);
        } else if (startsWith("clear", input)) {
            state_count = 0;
            transition_count = 0;
        } else if (startsWith("reset", input)) {
            asm volatile ("  jmp 0");
        } else {
            printf("Invalid command. Try face, echo, events, state.\r\n");
        }
    }
    delay(speed);
}
