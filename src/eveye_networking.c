//XXX NN
#include "eveye_networking.h"

// Global pointers to socket related structures
int gNNBasePort;
int *gNNSockfd;
struct sockaddr_in *gNNServaddr;
int gNNCounter;
float gNNCounter64x64;
float gNNCounter32x32;
float gNNCounter16x16;
float gNNCounter8x8;
float gNNCounter4x4;
float intra_IPD_DC;
float intra_IPD_HOR;
float intra_IPD_VER;
float intra_IPD_UL;
float intra_IPD_UR;
int gNNCntHEVC,  gNNCntEnh;
float gNNMSEHEVC, gNNMSEEnh;

void NN_setupServer(int basePort) {
  
  gNNSockfd = (int*) malloc(sizeof(int));
  gNNServaddr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
  
  if ( (*gNNSockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
      perror("socket creation failed"); 
      exit(EXIT_FAILURE); 
  }

  memset(gNNServaddr, 0, sizeof(*gNNServaddr)); 
    
  // Filling server information
  gNNBasePort = basePort;
  gNNServaddr->sin_family = AF_INET; 
  gNNServaddr->sin_port = htons(basePort); 
  gNNServaddr->sin_addr.s_addr = INADDR_ANY; 
  
  // Statistics initialization
  gNNCounter = 0;
  gNNCntHEVC = 0;
  gNNCntEnh = 0;
  gNNMSEHEVC = 0;
  gNNMSEEnh =  0;
  gNNCounter64x64 = 0.0;
  gNNCounter32x32 = 0.0;
  gNNCounter16x16 = 0.0;
  gNNCounter8x8 = 0.0;
  gNNCounter4x4 = 0.0;
  intra_IPD_DC = 0.0;
  intra_IPD_HOR = 0.0;
  intra_IPD_VER = 0.0;
  intra_IPD_UL = 0.0;
  intra_IPD_UR=0.0;
}


void NN_destroyServer() {
  close(*gNNSockfd);
  free(gNNSockfd);
  free(gNNServaddr);
}


void NN_CopyPredictorIntoContext (unsigned char *contextPtr, unsigned char *predictorPtr, int contextWidth, int contextHeight, int predictorWidth, int predictorHeight, int sizeofPixel) {
  
  // Setting off the context pointer to the first pixel of the predictor hole to be filled
  contextPtr += (((contextHeight - predictorHeight) * contextWidth) /* y offset */ +  (contextWidth - predictorHeight) /* x offset */) * sizeofPixel;
  
  // do the actual copy
  for (int y=0; y<predictorHeight; y++) {
    memcpy(contextPtr, predictorPtr, sizeofPixel * predictorWidth);
    contextPtr += contextWidth * sizeofPixel;
    predictorPtr += predictorWidth * sizeofPixel;
  }
}

void NN_CopyPredictorIntoContext16 (Pel *contextPtr, Pel *predictorPtr, int contextWidth, int contextHeight, int predictorWidth, int predictorHeight) {
  NN_CopyPredictorIntoContext ((unsigned char *)contextPtr, (unsigned char *)predictorPtr, contextWidth, contextHeight, predictorWidth, predictorHeight, sizeof(Pel));
}


// Example of using NN_CropBottomRight()
#if 0
//We crop the bottom-right cuw x cuh from the NN_PREDICTOR_SIZE x NN_PREDICTOR_SIZE network output, if needed
//Tested ok for 16x16 crops out of 32x32 patches so far
if (cuw < NN_PREDICTOR_SIZE) {
    pel *rcvdcrop=(pel*)malloc(sizeof(pel)*cuw*cuh);
    NN_CropBottomRight(rcvd16bpp, rcvdcrop, cuw, cuh);
    free(rcvd16bpp);
    rcvd16bpp = rcvdcrop; rcvdcrop = NULL;
    //NN_savePredictor("rcvdcrop16bpp.yuv", rcvd16bpp, cuw, cuh, cuw, true); 
}
#endif
void NN_CropBottomRight (Pel *rcvd16bpp, Pel *rcvdcrop, int cuw, int cuh )
{
  rcvd16bpp += ((NN_PREDICTOR_SIZE - cuh) * NN_PREDICTOR_SIZE) + (NN_PREDICTOR_SIZE - cuw);
  for (int i = 0; i < cuh; i++)
  {
     memcpy(rcvdcrop, rcvd16bpp, sizeof(Pel) * cuw);
          rcvd16bpp += NN_PREDICTOR_SIZE;
          rcvdcrop += cuw;
  }

}


unsigned char *NN_Pel2Char (Pel *buffer16bpp, int width, int height, int stride) {
  // Allocating the required memory in format 8bpp
  unsigned char *ret = (unsigned char *) malloc(width * height);
  
  // Doing the actual conversion using a copy of the returned pointer
  unsigned char *retPtr = ret;
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      *retPtr = (unsigned char) buffer16bpp[(y * stride) +x];
      retPtr++;
    }
  }
  
  return ret;
}

unsigned char *NN_Pel2Char16(Pel *buffer16bpp, int width, int height, int stride) {
	// Allocating the required memory in format 16bpp
	unsigned char *ret = (unsigned char *)malloc(2*width * height);
	   	  
	// Doing the actual conversion using a copy of the returned pointer
	unsigned char *retPtr = ret;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			*retPtr = (unsigned char)((buffer16bpp[(y * stride) + x]) & 0xFF);
			retPtr++;
			*retPtr = (unsigned char)((buffer16bpp[(y * stride) + x] >> 8) & 0xFF);
			retPtr++;
		}
	}
	return ret;
}

void NN_Char2Pel16(Pel *buffer16bpp, unsigned char *buffer16charbpp, int width, int height, int stride) {
	unsigned char pixelChar[2];

	// Doing the actual conversion usinga copy of the returned pointer
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			pixelChar[0] = buffer16charbpp[(y * 2*stride) + 2*x];
			pixelChar[1] = buffer16charbpp[(y * 2*stride) + (2*x + 1)];
	
			buffer16bpp[(y * stride) + x]= (Pel)(pixelChar[1] << 8)| (Pel)pixelChar[0];
		}
	}
}

void NN_Char2Pel (Pel *buffer16bpp, unsigned char *buffer8bpp, int width, int height, int stride) {
  
  // Doing the actual conversion usinga copy of the returned pointer
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      buffer16bpp[(y * stride) +x] = (Pel) buffer8bpp[(y * width) +x];
    }
  }
}


void NN_sendTo (unsigned char *msg, int msglen, int portDelta) {
  
  gNNServaddr->sin_port = htons(gNNBasePort + portDelta);
  sendto(*gNNSockfd,
         msg, msglen, 
         MSG_CONFIRM,
         (const struct sockaddr *) gNNServaddr,  
         sizeof(*gNNServaddr));
}


void NN_sendTo16 (Pel *msg, int msglen, int portDelta) {
  NN_sendTo ((unsigned char *)msg, msglen, portDelta);
}


int NN_recvFrom (unsigned char **msg) {
  
  // Here we store the info about the remote server address
  struct sockaddr_in cliaddr;
  memset(&cliaddr, 0, sizeof(cliaddr)); 
  socklen_t cliaddrlen = 0;
  
  // Allocating the required memory in format 8bpp
  *msg = (unsigned char *) malloc(NN_MAX_RCV_BUFFER_LEN);
  memset(*msg, 0, NN_MAX_RCV_BUFFER_LEN);
  
  int n = recvfrom(*gNNSockfd,
                   (unsigned char *)*msg,
                   NN_MAX_RCV_BUFFER_LEN,
                   MSG_WAITALL,
                   ( struct sockaddr *) &cliaddr,
                   &cliaddrlen);
  
  return n;
}


int NN_recvFrom16 (Pel **msg) {
  return NN_recvFrom ((unsigned char **)msg);
}


void NN_savePredictor(const char *fileName, Pel* predictor, int width, int height, int stride, bool appendMode) {
  FILE *fd;
  // Reading
  if (appendMode)
    fd = fopen(fileName, "ab");
  else
    fd = fopen(fileName, "wb");
  

  unsigned char pixelChar[2];
   
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
		// extract the individual bytes from  value
		pixelChar[0] = (predictor[(y*stride) + x]) & 0xFF;  // low byte
		pixelChar[1] = (predictor[(y*stride) + x] >> 8) & 0xFF;  // high byte
        fwrite(&pixelChar, sizeof(char), 2, fd);
    }
  }
  
  fclose(fd);
}


float NN_computeMSE(Pel* ptrA, Pel* ptrB, int width, int height, int stride) {
  float mse = 0;
  float tmp;
  
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      tmp = (float) (*ptrA) - (float) (*ptrB);
      mse += tmp * tmp;
      ptrA++;
      ptrB++;
    }
  }
  
  return mse/(width*height);
}


// AF TODO move this into eveye_pintra.c
bool NN_pintra_context_available (int x, int y, int cuw, int cuh) {
  bool res = false;
  
  if (cuw == 32 && x >= NN_CONTEXT_SIZE - cuw && cuh == 32 && y >= NN_CONTEXT_SIZE - cuh) {
    res = true;
  }
  if (cuw == 16 && x >= NN_CONTEXT_SIZE - cuw && cuh == 16 && y >= NN_CONTEXT_SIZE - cuh) {
    res = true;
  }
  if (cuw == 8 && x >= NN_CONTEXT_SIZE - cuw && cuh == 8 && y >= NN_CONTEXT_SIZE - cuh) {
    res = true;
  }
  if (cuw == 4 && x >= NN_CONTEXT_SIZE - cuw && cuh == 4 && y >= NN_CONTEXT_SIZE - cuh) {
    res = true;
  }
  
  return res;
}


void NN_statsHEVCUpdate(float mse) {
  gNNCntHEVC++;
  gNNMSEHEVC += mse;
}


void NN_statsEnhancedUpdate(float mse) {
  gNNCntEnh++;
  gNNMSEEnh += mse;
}



void NN_statsIPMupdate(int bestmode) {
	switch (bestmode)
	{
	case 0:
		//IPD_DC
		intra_IPD_DC++;
		break;

	case 1:
		//IPD_HOR
		intra_IPD_HOR++;
		break;

	case 2:
		//IPD_VER
		intra_IPD_VER++;
		break;

	case 3:
		//IPD_UL
		intra_IPD_UL++;
		break;

	case 4:
		//IPD_UR
		intra_IPD_UR++;
		break;
	}
}

void NN_statsEVCupdate(int width) {
	switch (width)
	{
	case 64:
		gNNCounter64x64++;
		break;

	case 32:
		gNNCounter32x32++;
		break;

	case 16:
		gNNCounter16x16++;
		break;

	case 8:
		gNNCounter8x8++;
		break;

	case 4:
		gNNCounter4x4++;
		break;
	}
}


void NN_statsPrint() {
  //printf ("XXX NN TotCoded %d HEVCCoded %d EnhCoded %d HEVCMSEGain %.2f EnhCMSEGain %.2f\n", gNNCntHEVC + gNNCntEnh, gNNCntHEVC, gNNCntEnh, gNNMSEHEVC/gNNCntHEVC, gNNMSEEnh/gNNCntEnh);
	printf("XXX NN TotOrg64x64 %.0f  TotOrg32x32 %.0f TotOrg16x16 %.0f TotOrg8x8 %.0f TotOrg4x4 %.0f \n", gNNCounter64x64, gNNCounter32x32, gNNCounter16x16, gNNCounter8x8, gNNCounter4x4);
}

void NN_statsIPMPrint() {
	printf("XXX NN TotIPD_DC %.0f  TotIPD_HOR %.0f TotIPD_VER %.0f TotIPD_UL %.0f TotIPD_UR %.0f \n", intra_IPD_DC, intra_IPD_HOR, intra_IPD_VER, intra_IPD_UL, intra_IPD_UR);
}

      //Writing back
#if 0
fd = fopen("/home/attilio/repos/HM-16.20/build/linux/dump.yuv", "rb");
#if AF_WRITE_BYTES
for (Int y=0; y<height; y++) {
    for (Int x=0; x<width; x++) {
      fread(pTrueDstOrig, sizeof(char) , 1, fd);
      pTrueDstOrig[(y*dstStrideTrue) +x] = (Pel) pixelChar;
    }
  }
#else
  fread(pTrueDstOrig, sizeof(Pel) * height * width , 1, fd);
#endif
fclose(fd);
#endif
