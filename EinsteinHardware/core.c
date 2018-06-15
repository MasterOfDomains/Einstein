
#include "hwglobal.h"


// Random number seed
uint8_t seed0;
uint8_t seed1;
uint8_t seed2;

char phonemes[128];
char modifier[128];	// must be same size as 'phonemes'
char g_text[64];

uint8_t default_pitch = 7;



#ifdef _AVR_
// Define routines to log to the UART
void loggerP(const char * s)
{
    rprintfProgStr(s);
}

void logger(const char * s)
{
    rprintfStr((char *)s);
}

void loggerc(char c)
{
    rprintfChar(c);
}

void logger_uint8(uint8_t n)
{
    rprintfNum(10, 3, 0, ' ', n);
}
void loggerCRLF(void)
{
    loggerP(PSTR("\n"));
}
#else
FILE * log_dest = stderr;
void setLogFile(FILE * file)
{
    log_dest = file;
}
void logger(const char* s)
{
    fprintf(log_dest,"%s",s);
}

void loggerc(char c)
{
    fprintf(log_dest,"%c",c);
}
void logger_uint8(uint8_t n)
{
    fprintf(log_dest,"%03d",n);
}
void loggerCRLF(void)
{
    fprintf(log_dest,"\n");
}
#endif



// Lookup user specified pitch changes
static const uint8_t PROGMEM PitchesP[]  = { 1, 2, 4, 6, 8, 10, 13, 16 };

/**
*
*  Find the single character 'token' in 'vocab'
*  and append its phonemes to dest[x]
*
*  Return new 'x'
*/

int copyToken(char token,char * dest, int x, const VOCAB* vocab)
{
    int ph;
    const char* src;

    for(ph = 0; ph < numVocab; ph++) {
        const char *txt = (const char *)pgm_read_word(&vocab[ph].txt);
        if(pgm_read_byte(&txt[0]) == token && pgm_read_byte(&txt[1])==0) {

            src = (const char *)pgm_read_word(&vocab[ph].phoneme);
            while(pgm_read_byte(src)!=0) {
                dest[x++] = pgm_read_byte(src);
                src++;
            }
            break;
        }
    }

    return x;
}

uint8_t whitespace(char c)
{
    return (c==0 || c==' ' || c==',' || c=='.' || c=='?' || c=='\''
            || c=='!' || c==':' || c=='/') ? 1 : 0;
}

/**
*  Enter:
*  src => English text in upper case
*  vocab => VOCAB array
*  dest => address to return result
*  return 1 if ok, or 0 if error
*/
int textToPhonemes(const char * src, const VOCAB* vocab, char * dest)
{
    int outIndex = 0;// Current offset into dest
    int inIndex = -1; // Starts at -1 so that a leading space is assumed

    while(inIndex==-1 || src[inIndex]!= 0) {	// until end of text
        int maxMatch=0;	// Max chars matched on input text
        int numOut=0;	// Number of characters copied to output stream for the best match
        int ph;
        boolean endsInWhiteSpace=FALSE;
        int maxWildcardPos = 0;

        // Get next phoneme, P2
        for(ph = 0; ph < numVocab; ph++) {
            int y,x;
            char wildcard=0; // modifier
            int wildcardInPos=0;
            boolean hasWhiteSpace=FALSE;
            const char* text = (const char*)pgm_read_word(&vocab[ph].txt);
            const char* phon = (const char*)pgm_read_word(&vocab[ph].phoneme);



            for(y=0;; y++) {

                char nextVocabChar = pgm_read_byte(&text[y]);
                char nextCharIn = (y + inIndex==-1) ? ' ' : src[y + inIndex];
                if(nextCharIn>='a' && nextCharIn<='z') {
                    nextCharIn = nextCharIn - 'a' + 'A';
                }

                if(nextVocabChar=='#' && nextCharIn >= 'A' && nextCharIn <= 'Z') {
                    wildcard = nextCharIn; // The character equivalent to the '#'
                    wildcardInPos=y;
                    continue;
                }

                if(nextVocabChar=='_') {
                    // try to match against a white space
                    hasWhiteSpace=TRUE;
                    if(whitespace(nextCharIn)) {
                        continue;
                    }
                    y--;
                    break;
                }

                // check for end of either string
                if(nextVocabChar==0 || nextCharIn==0) {
                    break;
                }

                if(nextVocabChar != nextCharIn) {
                    break;
                }
            }

            // See if its the longest complete match so far
            if(y<=maxMatch || pgm_read_byte(&text[y])!=0) {
                continue;
            }


            // This is the longest complete match
            maxMatch = y;
            maxWildcardPos = 0;
            x = outIndex; // offset into phoneme return data

            // Copy the matching phrase changing any '#' to the phoneme for the wildcard
            for(y=0;; y++) {
                char c = pgm_read_byte(&phon[y]);
                if(c==0) {
                    break;
                }
                if(c=='#') {
                    if(pgm_read_byte(&phon[y+1])==0) {
                        // replacement ends in wildcard
                        maxWildcardPos = wildcardInPos;
                    } else {
                        x = copyToken(wildcard,dest,x, vocab); // Copy the phonemes for the wildcard character
                    }
                } else {
                    dest[x++] = c;
                }
            }
            dest[x]=0;
            endsInWhiteSpace = hasWhiteSpace;

            // 14
            numOut = x - outIndex;	// The number of bytes added

        }// check next phoneme
        // 15 - end of vocab table

        //16
        if(endsInWhiteSpace==TRUE) {
            maxMatch--;
        }

        //17
        if(maxMatch==0) {
            loggerP(PSTR("Mistake in SAY, no token for "));
            logger(&src[inIndex]);
            loggerCRLF();
            return 0;
        }

        //20
        outIndex += numOut;
        if(outIndex > 256-16) {
            loggerP(PSTR("Mistake in SAY, text too long\n"));
            return 0;
        }

        //21

        inIndex += (maxWildcardPos>0) ? maxWildcardPos : maxMatch;
    }
    return 1;
}


/**
*
*   Convert phonemes to data string
*   Enter: textp = phonemes string
*   Return: phonemes = string of sound data
*			modifier = 2 bytes per sound data
*
*/
int phonemesToData(const char* textp, const PHONEME* phoneme)
{

    int phonemeOut = 0; // offset into the phonemes array
    int modifierOut = 0; // offset into the modifiers array
    unsigned int L81=0; // attenuate
    unsigned int L80=16;

    while(*textp != 0) {
        // P20: Get next phoneme
        boolean anyMatch=FALSE;
        int longestMatch=0;
        int ph;
        int numOut=0;	// The number of bytes copied to the output for the longest match


        // Get next phoneme, P2
        for(ph = 0; ph<numPhoneme; ph++) {
            int numChars;

            // Locate start of next phoneme
            const char* ph_text = (const char*)pgm_read_word(&phoneme[ph].txt);


            // Set 'numChars' to the number of characters
            // that we match against this phoneme
            for(numChars=0; textp[numChars]!=0 ; numChars++) {

                // get next input character and make lower case
                char nextChar = textp[numChars];
                if(nextChar>='A' && nextChar<='Z') {
                    nextChar = nextChar - 'A' + 'a';
                }

                if(nextChar!=pgm_read_byte(&ph_text[numChars])) {
                    break;
                }
            }

            // if not the longest match so far then ignore
            if(numChars <= longestMatch) {
                continue;
            }

            if(pgm_read_byte(&ph_text[numChars])!=0) {
                // partial phoneme match
                continue;

            }

            // P7: we have matched the whole phoneme
            longestMatch = numChars;

            // Copy phoneme data to 'phonemes'
            {
                const char* ph_ph = (const char*)pgm_read_word(&phoneme[ph].phoneme);
                for(numOut=0; pgm_read_byte(&ph_ph[numOut])!= 0; numOut++) {
                    phonemes[phonemeOut+numOut] = pgm_read_byte(&ph_ph[numOut]);
                }
            }
            L81 = pgm_read_byte(&phoneme[ph].attenuate)+'0';
            anyMatch=TRUE; // phoneme match found

            modifier[modifierOut]=-1;
            modifier[modifierOut+1]=0;

            // Get char from text after the phoneme and test if it is a numeric
            if(textp[longestMatch]>='0' && textp[longestMatch]<='9') {
                // Pitch change requested
                modifier[modifierOut] = pgm_read_byte(&PitchesP[textp[longestMatch]-'1'] );
                modifier[modifierOut+1] = L81;
                longestMatch++;
            }

            // P10
            if(L81!='0' && L81 != L80 && modifier[modifierOut]>=0) {
                modifier[modifierOut - 2] = modifier[modifierOut];
                modifier[modifierOut - 1] = '0';
                continue;
            }

            // P11
            if( (textp[longestMatch-1] | 0x20) == 0x20) {
                // end of input string or a space
                modifier[modifierOut] = (modifierOut==0) ? 16 : modifier[modifierOut-2];
            }

        } // next phoneme

        // p13
        L80 = L81;
        if(longestMatch==0 && anyMatch==FALSE) {
            loggerP(PSTR("Mistake in speech at "));
            logger(textp);
            loggerCRLF();
            return 0;
        }

        // Move over the bytes we have copied to the output
        phonemeOut += numOut;

        if(phonemeOut > sizeof(phonemes)-16) {
            loggerP(PSTR("Line too long\n"));
            return 0;
        }

        // P16

        // Copy the modifier setting to each sound data element for this phoneme
        if(numOut > 2) {
            int count;
            for(count=0; count != numOut; count+=2) {
                modifier[modifierOut + count + 2] = modifier[modifierOut + count];
                modifier[modifierOut + count + 3] = 0;
            }
        }
        modifierOut += numOut;

        //p21
        textp += longestMatch;
    }

    phonemes[phonemeOut++]='z';
    phonemes[phonemeOut++]='z';
    phonemes[phonemeOut++]='z';
    phonemes[phonemeOut++]='z';

    while(phonemeOut < sizeof(phonemes)) {
        phonemes[phonemeOut++]=0;
    }

    while(modifierOut < sizeof(modifier)) {
        modifier[modifierOut++]=-1;
        modifier[modifierOut++]=0;
    }

    return 1;
}




/*
*   A delay loop that doesn't change with different optimisation settings
*/
void loop(uint8_t delay)
{
#ifdef _AVR_
    __asm__ volatile (
        "1: dec %0" "\n\t"
        "brne 1b"
        : "=r" (delay)
        : "0" (delay)
    );
#endif
}

void pause(uint8_t delay)
{
    uint8_t r;
    for(r=TIME_FACTOR; r>0; r--) {
        loop(delay);
    }

}

void delay(uint8_t d)
{
    while(d!=0) {
        pause(0);	// 256
        pause(0);	// 256
        d--;
    }
}




/*
	Generate a random number
*/
uint8_t random(void)
{
    uint8_t tmp = (seed0 & 0x48) + 0x38;
    seed0<<=1;
    if(seed1 & 0x80) {
        seed0++;
    }
    seed1<<=1;
    if(seed2 & 0x80) {
        seed1++;
    }
    seed2<<=1;
    if(tmp & 0x40) {
        seed2++;
    }
    return seed0;
}




void soundOff(void)
{
#ifdef _AVR_

#ifdef _AVR_PWM_
    TCCR1A &= ~(BV(COM1B1));	// Disable PWM
    DDRB &= ~(BV(PB2));			// make B2 an input pin
#endif

#ifdef _AVR_BINARY_
    SOUND_DDR &= ~(SOUND_BIT);	// Set speaker port as input
#endif

#ifdef _AVR_QUAD_
    QUAD_DDR &= ~(QUAD_MASK);	// Set ports as input
#endif
#endif
}

void soundOn(void)
{
#ifdef _AVR_

#ifdef _AVR_PWM_
    DDRB |= BV(PB2);			// make B2 an output pin
    TCCR1A = 0;					// disable PWM
    ICR1 = PWM_TOP;
    TCCR1B = PWM_FLAGS;
    TCNT1=0;
    TCCR1A |= BV(COM1B1);		// ENABLE PWM ON B2 USING OC1B, OCR1B
#endif

#ifdef _AVR_BINARY_
    SOUND_DDR |= (SOUND_BIT);	// Set speaker port as output
    SOUND_PORT &= ~(SOUND_BIT);
#endif

#ifdef _AVR_QUAD_
    QUAD_DDR |= (QUAD_MASK);	// Set ports as output
    QUAD_PORT &= ~(QUAD_MASK);	// all off
#endif


    // initialise random number seed
    seed0=0xecu;
    seed1=7;
    seed2=0xcfu;
#endif
}

#ifdef _AVR_PWM_
// Logarithmic scale
//static const int16_t PROGMEM Volume[8] = {0,PWM_TOP * 0.01,PWM_TOP * 0.02,PWM_TOP * 0.03,PWM_TOP * 0.06,PWM_TOP * 0.12,PWM_TOP * 0.25,PWM_TOP * 0.5};

// Linear scale
static const int16_t PROGMEM Volume[8] = {0,PWM_TOP * 0.07,PWM_TOP * 0.14,PWM_TOP * 0.21,PWM_TOP * 0.29,PWM_TOP * 0.36,PWM_TOP * 0.43,PWM_TOP * 0.5};
#endif


void sound(uint8_t b)
{
    b = (b & 15);

#ifdef _LOG_
    loggerc((char)(b+'0'));
#endif

#ifdef _AVR_

#ifdef _AVR_PWM_
    // Update PWM volume
    int16_t duty = pgm_read_word(&Volume[b>>1]);	  // get duty cycle
    if(duty!=OCR1B) {
        TCNT1=0;
        OCR1B = duty;
    }
#endif

#ifdef _AVR_BINARY_
    // Update Port Volume
    if(b>=8) {
        SOUND_PORT |= SOUND_BIT;
    } else {
        SOUND_PORT &= ~(SOUND_BIT);
    }
#endif

#ifdef _AVR_QUAD_
    b <<= QUAD_BIT;
    b &= QUAD_MASK;
    QUAD_PORT = (QUAD_PORT & ~(QUAD_MASK)) | b;
#endif

#endif

}



uint8_t playTone(uint8_t soundNum,uint8_t soundPos,char pitch1, char pitch2, uint8_t count, uint8_t volume)
{
    const uint8_t* soundData = &SoundData[soundNum * 0x40];
    while(count-- > 0 ) {
        uint8_t s;

        s = pgm_read_byte(&soundData[soundPos & 0x3fu]);
        sound((uint8_t)(s & volume));
        pause(pitch1);
        sound((uint8_t)((s>>4) & volume));
        pause(pitch2);

        soundPos++;
    }
    return soundPos & 0x3fu;
}

void play(uint8_t duration, uint8_t soundNumber)
{
    while(duration-- != 0) {
        playTone(soundNumber,random(), 7,7, 10, 15);
    }
}


void setPitch(uint8_t pitch)
{
    default_pitch = pitch;
}


/*
*  Speak a string of phonemes
*/
void speak(const char* textp)
{
    uint8_t
    phonemeIn,				// offset into text
    byte2,
    modifierIn,				// offset into stuff in modifier
    punctuationPitchDelta;	// change in pitch due to fullstop or question mark
    char byte1;
    char phoneme;
    const SOUND_INDEX* soundIndex;
    uint8_t sound1Num;			// Sound data for the current phoneme
    uint8_t sound2Num;			// Sound data for the next phoneme
    uint8_t sound2Stop;			// Where the second sound should stop
    char pitch1;			// pitch for the first sound
    char pitch2;			// pitch for the second sound
    short i;
    uint8_t sound1Duration;		// the duration for sound 1

    if(phonemesToData(textp,s_phonemes)) {
        // phonemes has list of sound bytes
#ifdef _LOG_
        loggerP(PSTR("Data:"));
        logger(phonemes);
        loggerCRLF();
#endif

        soundOn();

        // _630C
        byte1=0;
        punctuationPitchDelta=0;

        //Q19
        for(phonemeIn=0,modifierIn=0; phonemes[phonemeIn]!=0; phonemeIn+=2, modifierIn+=2) {
            uint8_t	duration;	// duration from text line
            uint8_t SoundPos;	// offset into sound data
            uint8_t fadeSpeed=0;

            phoneme=phonemes[phonemeIn];
            if(phoneme=='z') {
                delay(15);
                continue;
            } else if(phoneme=='#') {
                continue;
            } else {

                // Collect info on sound 1
                soundIndex = &SoundIndex[phoneme - 'A'];
                sound1Num = pgm_read_byte(&soundIndex->SoundNumber);
                byte1 = pgm_read_byte(&soundIndex->byte1);
                byte2 = pgm_read_byte(&soundIndex->byte2);

                duration = phonemes[phonemeIn+1] - '0';	// Get duration from the input line
                if(duration!=1) {
                    duration<<=1;
                }
                duration += 6;							// scaled duration from the input line (at least 6)

                sound2Stop = 0x40>>1;


                pitch1 = modifier[modifierIn];
                if(modifier[modifierIn + 1]==0 || pitch1==-1) {
                    pitch1 = 10;
                    duration -= 6;
                } else if(modifier[modifierIn + 1]=='0' || duration==6) {
                    duration -= 6;
                }


                //q8
                pitch2 = modifier[modifierIn+2];
                if(modifier[modifierIn + 3]==0 || pitch2 == -1) {
                    pitch2 = 10;
                }

                //q10

                if(byte1<0) {
                    sound1Num = 0;
                    random();
                    sound2Stop=(0x40>>1)+2;


                } else {
                    // is positive
                    if(byte1==2) {
                        // 64A4
                        // Make a white noise sound !
                        uint8_t volume;					// volume mask
#ifdef _LOG_
                        loggerP(PSTR("\nA *******"));
                        logger_uint8(duration);
                        loggerCRLF();
#endif
                        volume = (duration==6) ? 15 : 1;  /// volume mask
                        for(duration <<= 2; duration>0; duration--) {
                            playTone(sound1Num,random(),8,12,11, volume);
                            // Increase the volume
                            if(++volume==16) {
                                volume = 15;	// full volume from now on
                            }

                        }
                        continue;

                    } else {
                        //q11
                        if(byte1 != 0) {
                            delay(25);
                        }
                    }
                }

            }


            // 6186
            pitch1 += default_pitch + punctuationPitchDelta;
            if(pitch1<1) {
                pitch1=1;
            }

            pitch2 += default_pitch + punctuationPitchDelta;
            if(pitch2<1) {
                pitch2=1;
            }

            // get next phoneme
            phoneme=phonemes[phonemeIn + 2];

            if(phoneme==0 || phoneme=='z') {
                if(duration==1) {
                    delay(60);
                }
                phoneme='a';	// change to a pause
            } else {
                // s6
                if(byte2 != 1) {
                    byte2 = (byte2 + pgm_read_byte(&SoundIndex[phoneme-'A'].byte2))>>1;
                }

                if(byte1 < 0 || pgm_read_byte(&SoundIndex[phoneme-'A'].byte1) != 0) {
                    phoneme ='a'; // change to a pause
                }
            }

            // S10
            sound2Num = pgm_read_byte(&SoundIndex[phoneme-'A'].SoundNumber);

            sound1Duration = 0x80;			// play half of sound 1
            if(sound2Num==sound1Num) {
                byte2 = duration;
            }

            // S11
            if( (byte2>>1) == 0 ) {
                sound1Duration = 0xff;				// play all of sound 1
            } else {
                // The fade speed between the two sounds
                fadeSpeed = (sound1Duration + (byte2>>1))/byte2;

                if(duration==1) {
                    sound2Stop = 0x40;	// dont play sound2
                    sound1Duration = 0xff;			// play all of sound 1
                    pitch1 = 12;
                }
            }

            SoundPos = 0;
            do {
                uint8_t sound1Stop = (sound1Duration>>2) & 0x3fu;
                uint8_t sound1End = min(sound1Stop, sound2Stop);

                if( sound1Stop != 0 ) {

#ifdef _LOG_
                    loggerP(PSTR("\nB Remaining="));
                    logger_uint8(duration);
                    loggerP(PSTR("Repeat min("));
                    logger_uint8(sound1Stop);
                    loggerP(PSTR(","));
                    logger_uint8(sound2Stop);
                    loggerP(PSTR(") times,Sound#="));
                    loggerc((char)(sound1Num+'A'));
                    loggerP(PSTR("["));
                    logger_uint8(SoundPos);
                    loggerP(PSTR("]\n"));
#endif
                    SoundPos = playTone(sound1Num,SoundPos,pitch1,pitch1, sound1End, 15);
                }

                // s18
                if(sound2Stop != 0x40) {
#ifdef _LOG_
                    loggerP(PSTR("\nC Remaining="));
                    logger_uint8(duration);
                    loggerP(PSTR("Repeat "));
                    logger_uint8((uint8_t)(sound2Stop-sound1End));
                    loggerP(PSTR(" times,Sound#="));
                    loggerc((char)(sound2Num+'A'));
                    loggerP(PSTR("["));
                    logger_uint8(SoundPos);
                    loggerP(PSTR("]\n"));
#endif

                    SoundPos = playTone(sound2Num,SoundPos,pitch2,pitch2, (uint8_t)(sound2Stop - sound1End), 15);
                }

                //s23
                if(sound1Duration!=0xff && duration<byte2) {
                    // Fade sound1 out
                    sound1Duration -= fadeSpeed;
                    if( sound1Duration >= (uint8_t)0xC8) {
                        sound1Duration=0;	// stop playing sound 1
                    }
                }


                // Call any additional sound
                if(byte1==-1) {
                    play(3,30);	// make an 'f' sound
                } else if(byte1==-2) {
                    play(3,29);	// make an 's' sound
                } else if(byte1==-3) {
                    play(3,33);	// make a 'th' sound
                } else if(byte1==-4) {
                    play(3,27);	// make a 'sh' sound
                }

            } while(--duration!=0);


            // Scan ahead to find a '.' or a '?' as this will change the pitch
            punctuationPitchDelta=0;
            for(i=6; i>0; i--) {
                char next = phonemes[phonemeIn + (i * 2)];
                if(next=='i') {
                    // found a full stop
                    punctuationPitchDelta = 6 - i; // Lower the pitch
                } else if(next=='h') {
                    // found a question mark
                    punctuationPitchDelta = i - 6; // Raise the pitch
                }
            }

            if(byte1 == 1) {
                delay(25);
            }


        } // next phoneme

    }
    soundOff();
}

/*
*   Speak an English command line of text
*/
void say(const char * original)
{
    int i;
    if(textToPhonemes(original,s_vocab, phonemes)) {
#ifdef _LOG_
        loggerP(PSTR("Say:'"));
        logger(original);
        loggerP(PSTR("' => Phonemes:'"));
        logger(phonemes);
        loggerCRLF();
#endif
        // copy string from phonemes to text
        for(i = 0; phonemes[i]!=0; i++) {
            g_text[i]=phonemes[i];
        }
        while(i<sizeof(g_text)) {
            g_text[i++]=0;
        }

        speak(g_text);
    }
}





void init(void)
{
#ifdef _AVR_
    /* TODO - Put this back in
    uartInit();  // initialize the UART (serial port)
    uartSetBaudRate(0, 19200); // set the baud rate of the UART for our debug/reporting output
    rprintfInit(uart0SendByte); // initialize rprintf system
    */

    // Power save
    PRR = BV(PRTWI)	// turn off TWI
          | BV(PRSPI)	// turn off SPI
          | BV(PRADC) // turn off ADC
          | BV(PRTIM0) // turn off TIMER0
          | BV(PRTIM2) // turn off TIMER2
          ;
#endif
}


