// Copied from https://github.com/joshlf/magspoof.git
// and https://github.com/samyk/magspoof.git
// and port something to Arduino Nano
// - miaoski

void setPolarity(int polarity);
void playBit(int bit);

#define PIN_A    3      // left pin, to L293D pin 2
#define PIN_B    4      // right pin, to L293D pin 7
#define ENABLE_PIN 13
#define BUTTON_PIN 12
#define CLOCK_US 400
#define BETWEEN_ZERO 53 // 53 zeros between track1 & 2

int polarity = 0;
unsigned int curTrack = 0;
char revTrack[80];

const int sublen[] = { 32, 48, 48 };
const int bitlen[] = {  7,  5,  5 };

const char* tracks[] = {
"%B4503111122223333^MIAOSKI LIN              ^2010200999909999999900990000000?",	// Track 1; %BPAN^NAME^YYMMSSSCVV? + LRC
";4503111122223333=200120099999999?" // Track 2 ;PAN=YYMMSSSDIS? + LRC
};

void setup() {
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  storeRevTrack(2);
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

  while(revTrack[i++] != '\0');
  i--;
  while(i--)
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
  for(int i = 0; i < 25; i++)
    playBit(0);

  Serial.print("Sending: ");
  for(int i = 0; tracks[track][i] != '\0'; i++) {
    Serial.print(tracks[track][i]);     // The delay that Track 1 needs.
    crc = 1;
    tmp = tracks[track][i] - sublen[track];

    for(int j = 0; j < bitlen[track]-1; j++) {
      crc ^= (tmp & 1);
      lrc ^= (tmp & 1) << j;
      playBit(tmp & 1);
      tmp >>= 1;
    }
    playBit(crc);
  }
  Serial.println();

  // finish calculating and send last "byte" (LRC)
  tmp = lrc;
  crc = 1;
  for(int j = 0; j < bitlen[track]-1; j++) {
    crc ^= tmp & 1;
    playBit(tmp & 1);
    tmp >>= 1;
  }
  playBit(crc);

  // if track 1, play 2nd track in reverse (like swiping back?)
/*
  if(track == 0) {
    for(int i = 0; i < BETWEEN_ZERO; i++)
      playBit(0);
    reverseTrack(2);
  }
  */
  // finish with 0's
  for (int i = 0; i < 25; i++)
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
  memset(revTrack, 1, sizeof(revTrack));
  while(digitalRead(BUTTON_PIN) == HIGH);

  delay(50);
  while(digitalRead(BUTTON_PIN) == HIGH);
  Serial.print("Track ");
  Serial.println(1 + (curTrack % 2));
  playTrack(1 + (curTrack++ % 2)); 
  delay(400);
}
