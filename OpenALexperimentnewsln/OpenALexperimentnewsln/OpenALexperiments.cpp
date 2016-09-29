// OpenALexperiments.cpp : Defines the entry point for the console application.
//

//#define _USE_MATH_DEFINES
#include "stdafx.h"

SOCKET SetupSocketNew(void);
int ShutdownSocketConnNew(SOCKET ClientSocket);

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5002"

#define CASE_RETURN(err) case (err): return "##err"
const char* al_err_str(ALenum err) {
	switch (err) {
		CASE_RETURN(AL_NO_ERROR);
		CASE_RETURN(AL_INVALID_NAME);
		CASE_RETURN(AL_INVALID_ENUM);
		CASE_RETURN(AL_INVALID_VALUE);
		CASE_RETURN(AL_INVALID_OPERATION);
		CASE_RETURN(AL_OUT_OF_MEMORY);
	}
	return "unknown";
}
#undef CASE_RETURN

#define __al_check_error(file,line) \
    do { \
        ALenum err = alGetError(); \
        for(; err!=AL_NO_ERROR; err=alGetError()) { \
            std::cerr << "AL Error " << al_err_str(err) << " at " << file << ":" << line << std::endl; \
        } \
    }while(0)

#define al_check_error() \
    __al_check_error(__FILE__, __LINE__)


void init_al() {
	ALCdevice *dev = NULL;
	ALCcontext *ctx = NULL;

	const char *defname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	std::cout << "Default device: " << defname << std::endl;

	dev = alcOpenDevice(defname);
	ctx = alcCreateContext(dev, NULL);
	alcMakeContextCurrent(ctx);
}

void exit_al() {
	ALCdevice *dev = NULL;
	ALCcontext *ctx = NULL;
	ctx = alcGetCurrentContext();
	dev = alcGetContextsDevice(ctx);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(ctx);
	alcCloseDevice(dev);
}

int main(int argc, char* argv[]) {
	/* initialize OpenAL */
	init_al();
	SOCKET DataInSocket = SetupSocketNew();

	/* Create buffer to store samples */
	ALuint buf;
	alGenBuffers(1, &buf);
	al_check_error();

	/* Fill buffer with Sine-Wave */
	float freq = 440.f;
	float seconds = 0.5;
	float sintime = 2;
	unsigned sample_rate = 22050;
	size_t buf_size = seconds * sample_rate;

	short *samples;
	samples = new short[buf_size];
	for (int i = 0; i<buf_size; ++i) {
		samples[i] = 0;
	}
	for (int i = 0; i<(buf_size/sintime); ++i) {
		samples[i] = (16380 * ((buf_size/sintime)-i) * sintime * sin((2.f*float(3.141592)*freq) / sample_rate * i)) / buf_size;
	}

	/* Download buffer to OpenAL */
	alBufferData(buf, AL_FORMAT_MONO16, samples, buf_size*2, sample_rate);
	al_check_error();
	
	int xpts = 16;
	int ypts = 32;

	int num_sources = xpts;

	//Generate 16 sources
	ALuint *srclist;
	srclist = new ALuint[num_sources];
	alGenSources(num_sources, srclist);

	for (int i = 0; i < num_sources; ++i) {
		alSourcei(srclist[i], AL_BUFFER, buf);
		alSourcef(srclist[i], AL_REFERENCE_DISTANCE, 1.0f);
		alSourcei(srclist[i], AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcei(srclist[i], AL_LOOPING, AL_TRUE);
		alSourcei(srclist[i], AL_SAMPLE_OFFSET, (buf_size*i)/num_sources);
		alSourcePlay(srclist[i]);
	}

	char newsframe[8] = "1234567";

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int iResult;
	unsigned char pointdist;
	float pointdistnorm;
	float angle;
	float zdist;
	float xdist;
	do {
		iResult = recv(DataInSocket, newsframe, 8, MSG_WAITALL);
		while ((newsframe[0] != 'n') && (newsframe[7] != 'e')) {
			iResult = recv(DataInSocket, newsframe, 8, MSG_WAITALL);
		}
		strcpy_s(newsframe, "1234567");
		iResult = recv(DataInSocket, recvbuf, recvbuflen, MSG_WAITALL);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			
			for (int i = 0; i < num_sources; ++i) {
				// Set-up sound source and play buffer 
				//alSourceStop(srclist[i]);
				
				pointdist = recvbuf[(i*ypts) + (ypts / 2)];
				pointdistnorm = float(pointdist) / 255;
				angle = 3.141592*(0 + ((float(i)+0.5) / float(num_sources)));
				xdist = cos(angle)*pointdistnorm;
				zdist = sin(angle)*pointdistnorm;

				//printf("%2.2lf ", zdist);
				//printf("%2.2lf ", xdist);
				//printf("%u ", pointdist);
				printf("%2.2lf ", pointdistnorm);
				alSource3f(srclist[i], AL_POSITION, xdist*-10, 0, zdist*10);
				//alSourcePlay(srclist[i]);

				//While sound is playing, sleep 
				//al_check_error();
				//ALint state = 0;
				//while (state != AL_STOPPED) {
				//	alGetSourcei(srclist[i], AL_SOURCE_STATE, &state);
				//}
			}
			
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			ShutdownSocketConnNew(DataInSocket);
		}

	} while (iResult > 0);

	ShutdownSocketConnNew(DataInSocket);
	
	alDeleteSources(num_sources, srclist);

	/* Dealloc OpenAL */
	exit_al();
	al_check_error();
	return 0;
}