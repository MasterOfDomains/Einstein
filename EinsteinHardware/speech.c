
#include "global.h"


#ifndef _AVR_
#include <direct.h>
#include <string.h>
#include <io.h>

/**
   Maintain a 'ref'erence file
   and compare the 'test' file against it.
   This is used to test if the output has been
   changed unintentionally by code changes.
*/
void test(const char* txt){
	FILE* fileOut;
	FILE* refFile;
	char refFilename[64];
	char testFilename[64];

	mkdir("ref");
	mkdir("test");

	strcpy(refFilename,"ref/");
	strncpy(refFilename+4,txt,40);
	strcat(refFilename,".txt");

	strcpy(testFilename,"test/");
	strncpy(testFilename+5,txt,40);
	strcat(testFilename,".txt");

	refFile = fopen(refFilename,"r");
	if(refFile==NULL){
		// First time
		fileOut = fopen(refFilename,"w");
	}else{
		// We already have a reference
		fileOut = fopen(testFilename,"w");
	}
	setLogFile(fileOut);	// redirect any logging to the file
	say(txt);
	fclose(fileOut);
	setLogFile(stdout);		// redirect any logging back to stdout

	if(refFile!=NULL){
		char refBuf[1];
		char testBuf[1];
		int  pos=0;

		fileOut = fopen(testFilename,"r");
		if(_filelength(fileOut->_file) != _filelength(refFile->_file)){
			printf("'%s'=>Files are different sizes\n",txt);
		}else{
			while(!feof(refFile)){
				fread(refBuf,1,1,refFile);
				fread(testBuf,1,1,fileOut);
				if(refBuf[0] != testBuf[0]){
					printf("'%s'=>Files are different at offset %d\n",txt,pos);
					break;
				}
				pos++;
			}
		}


		fclose(fileOut);
		fclose(refFile);
	}

}
#endif


/**
	The main entry point
*/
/*
int main(int argc, char* argv[]){


#ifdef _AVR_
	int txtptr=0;
	char text[64];
	init();

	while(1){

		int c = uart0GetByte();
		if(c!=-1){
			if(c=='\r' || c=='\n'){
				// We have an entire line
				rprintfCRLF();
				text[txtptr]=0;
				if(txtptr!=0){
					if(text[0]=='*'){
						// Speak the phonemes
						speak(text+1);
					}else if(text[0]=='!' && text[1]>='A' && text[1]<='Z' && text[2]==0){
						// set the pitch
						setPitch(text[1]-'A');
					}else{
						// say the English text
						say(text);
					}
				}	txtptr=0;
			}else{
				if(txtptr < sizeof(text)-1){
					text[txtptr++]=c;
					rprintfChar(c);
				}
			}
		}else{
			sleep_mode();
		}
	};
#else
	// Test sentences - to check for new errors
	test("Society of Robots");
	test("Admin has gone crazy");
	test("What is your name");
	test("Now is the winter of our discontent");
	test("made glorious summer by this");
	test("son of york.");
	test("The quick brown fox jumped");
	test("over the lazy dog");
#endif	

	return 0;
}
*/
