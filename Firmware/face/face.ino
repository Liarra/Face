/* Serial-controlled Face */

int led_pins[16] = {5,6,8,9,10,11,12,13,A0,A1,A2,A3,A4,A5,A6,A7};
int button_pins[3] = {2,4,7};
int speaker_pin = 3;





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

void setup() {
    // Initialize serial and printf
    Serial.begin(9600);
    fdev_setup_stream(&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &uartout;
}

void loop() {
    char input[80];
    double freq;
    char freq_s[20];
    nextLine(input, 80);
    if (startsWith("face 0x", input)) {
        digitalWrite(13, HIGH);
    } else if (startsWith("face ", input)) {
    } else if (startsWith("face", input)) {
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
