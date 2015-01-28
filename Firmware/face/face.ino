/* Serial-controlled Face */

int led_pins[16] = {4,5,6,7,8,9,10,11,12,13,A0,A1,A2,A3,A4,A5};
int button_pins[3] = {2,A6,A7};
int speaker_pin = 3;

uint8_t speed = 100; // 0.1s time granularity, all time is expressed as a multiple of this

struct State {
    uint16_t face;
    uint8_t delay;
    uint8_t sound;
};

struct Transition {
    uint8_t from;
    uint8_t p;
    uint8_t next;
};

State states[32];
Transition transitions[64];




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

inline void nextLine(char *buffer, int len) {
    // Blocks until next line is completely read
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

void setup() {
    // Initialize serial and printf
    Serial.begin(9600);
    fdev_setup_stream(&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &uartout;
    
    for (int i=0; i<16; i++) {
        pinMode(led_pins[i], OUTPUT);
        digitalWrite(led_pins[i], LOW);
    }
}

void loop() {
    char input[80];
    double freq;
    char freq_s[20];
    uint8_t face[2];
    nextLine(input, 80);
    if (startsWith("face ", input)) {
        digitalWrite(13, HIGH);
    } else if (startsWith("on ", input)) {
        digitalWrite(led_pins[atoi(&input[2])], LOW);
    } else if (startsWith("off ", input)) {
        digitalWrite(led_pins[atoi(&input[3])], HIGH);
    } else if (startsWith("echo", input)) {
        echo = !echo;
    } else if (startsWith("events", input)) {
        digitalWrite(13, LOW);
    } else if (startsWith("state", input)) {
        
    } else if (startsWith("reset", input)) {
        asm volatile ("  jmp 0");
    } else {
        printf("Invalid command. Try face, echo, events, state.\r\n");
    }
}