// Copied from https://github.com/joshlf/magspoof.git
// and https://github.com/samyk/magspoof.git
// and port something to Arduino Nano
// - miaoski

void writeLow();
void setPolarity(int polarity);
void playBit(int bit);
unsigned char *getEncoding(unsigned char character);
void writeEncodedCharacter(char character);
void writeSequence(int len, unsigned char sequence[]);

#define PIN_A    3      // left pin, to L293D pin 2
#define PIN_B    4      // right pin, to L293D pin 7
#define ENABLE_PIN 13
#define BUTTON_PIN 12
#define CLOCK_US 200
#define BETWEEN_ZERO 53 // 53 zeros between track1 & 2
#define TRACKS 2

int polarity = 0;
unsigned int curTrack = 0;
char revTrack[41];

const int sublen[] = { 32, 48, 48 };
const int bitlen[] = {  7,  5,  5 };

const char* tracks[] = {
"%B123456781234567^LASTNAME/FIRST^YYMMSSSDDDDDDDDDDDDDDDDDDDDDDDDD?\0", // Track 1
";123456781234567=YYMMSSSDDDDDDDDDDDDDD?\0" // Track 2
};

void setup() {
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  storeRevTrack(2);
}

// Set the voltage to 0
void writeLow() {
  digitalWrite(PIN_A, LOW);
  digitalWrite(PIN_B, LOW);
}

// Set the polarity of the elctromagnet.
void setPolarity(int polarity) {
  if (polarity) {
    digitalWrite(PIN_B, LOW);
    digitalWrite(PIN_A, HIGH);
  } else {
    digitalWrite(PIN_A, LOW);
    digitalWrite(PIN_B, HIGH);
  }
}

void playBit(int bit) {
  polarity ^= 1;
  setPolarity(polarity);
  delayMicroseconds(CLOCK_US);

  if (bit == 1) {
    polarity ^= 1;
    setPolarity(polarity);
  }

  delayMicroseconds(CLOCK_US);
}

// when reversing
void reverseTrack(int track) {
  int i = 0;
  track--; // index 0
  polarity = 0;

  while (revTrack[i++] != '\0');
  i--;
  while (i--)
    for (int j = bitlen[track]-1; j >= 0; j--)
      playBit((revTrack[i] >> j) & 1);
}

// plays out a full track, calculating CRCs and LRC
void playTrack(int track)
{
  int tmp, crc, lrc = 0;
  track--; // index 0
  polarity = 0;

  // enable H-bridge and LED
  digitalWrite(ENABLE_PIN, HIGH);

  // First put out a bunch of leading zeros.
  for (int i = 0; i < 25; i++)
    playBit(0);

  //
  Serial.println(tracks[track]);
  for (int i = 0; tracks[track][i] != '\0'; i++)
  {
    crc = 1;
    tmp = tracks[track][i] - sublen[track];

    for (int j = 0; j < bitlen[track]-1; j++)
    {
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      playBit(tmp & 1);
      tmp >>= 1;
    }
    playBit(crc);
  } 

  // finish calculating and send last "byte" (LRC)
  tmp = lrc;
  crc = 1;
  for (int j = 0; j < bitlen[track]-1; j++)
  {
    crc ^= tmp & 1;
    playBit(tmp & 1);
    tmp >>= 1;
  }
  playBit(crc);

  // if track 1, play 2nd track in reverse (like swiping back?)
  if (track == 0)
  {
    // if track 1, also play track 2 in reverse
    // zeros in between
    for (int i = 0; i < BETWEEN_ZERO; i++)
      playBit(0);

    // send second track in reverse
    reverseTrack(2);
  }

  // finish with 0's
  for (int i = 0; i < 5 * 5; i++)
    playBit(0);

  digitalWrite(PIN_A, LOW);
  digitalWrite(PIN_B, LOW);
  digitalWrite(ENABLE_PIN, LOW);

}



// stores track for reverse usage later
void storeRevTrack(int track) {
  int i, tmp, crc, lrc = 0;
  track--; // index 0
  polarity = 0;

  for (i = 0; tracks[track][i] != '\0'; i++) {
    crc = 1;
    tmp = tracks[track][i] - sublen[track];

    for (int j = 0; j < bitlen[track]-1; j++) {
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      tmp & 1 ?
        (revTrack[i] |= 1 << j) :
        (revTrack[i] &= ~(1 << j));
      tmp >>= 1;
    }
    crc ?
      (revTrack[i] |= 1 << 4) :
      (revTrack[i] &= ~(1 << 4));
  } 

  // finish calculating and send last "byte" (LRC)
  tmp = lrc;
  crc = 1;
  for (int j = 0; j < bitlen[track]-1; j++) {
    crc ^= tmp & 1;
    tmp & 1 ?
      (revTrack[i] |= 1 << j) :
      (revTrack[i] &= ~(1 << j));
    tmp >>= 1;
  }
  crc ?
    (revTrack[i] |= 1 << 4) :
    (revTrack[i] &= ~(1 << 4));

  i++;
  revTrack[i] = '\0';
}

 
void loop() {
  while(digitalRead(BUTTON_PIN) == HIGH);

  delay(50);
  while(digitalRead(BUTTON_PIN) == HIGH);
  Serial.println("Sending");
  playTrack(1 + (curTrack++ % 2)); 
  delay(400);
}
