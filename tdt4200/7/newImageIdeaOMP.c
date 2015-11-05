#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "ppm.h"


// Image from:
// http://7-themes.com/6971875-funny-flowers-pictures.html

typedef struct {
  float red,green,blue;
} AccuratePixel;

typedef struct {
  int x, y;
  AccuratePixel *data;
} AccurateImage;

// Convert ppm to high precision format.
AccurateImage *convertImageToNewFormat(PPMImage *image) {
  // Make a copy
  AccurateImage *imageAccurate;
  imageAccurate = (AccurateImage *)malloc(sizeof(AccurateImage));
  imageAccurate->data = (AccuratePixel*)malloc(image->x * image->y * sizeof(AccuratePixel));
  for(int i = 0; i < image->x * image->y; i++) {
    imageAccurate->data[i].red   = (float) image->data[i].red;
    imageAccurate->data[i].green = (float) image->data[i].green;
    imageAccurate->data[i].blue  = (float) image->data[i].blue;
  }
  imageAccurate->x = image->x;
  imageAccurate->y = image->y;

  return imageAccurate;
}

AccurateImage *createEmptyImage(PPMImage *image){
  AccurateImage *imageAccurate;
  imageAccurate = (AccurateImage *)malloc(sizeof(AccurateImage));
  imageAccurate->data = (AccuratePixel*)malloc(image->x * image->y * sizeof(AccuratePixel));
  imageAccurate->x = image->x;
  imageAccurate->y = image->y;

  return imageAccurate;
}

// free memory of an AccurateImage
void freeImage(AccurateImage *image){
  free(image->data);
  free(image);
}

void bar(int senterY, int size, AccuratePixel *line_buffer, AccurateImage* imageIn,
          AccurateImage *imageOut)
{

  float sum_red = 0;
  float sum_blue = 0;
  float sum_green =0;
  int W = imageIn->x;
  int H = imageIn->y;

  int starty = senterY-size;
  int endy = senterY+size;


  // Initialize and update the line_buffer.
  // For OpenMP this might cause problems
  // Separating out the initialization part might help
  if (starty <=0) {
    starty = 0;
    for(int i=0; i<W; i++){
      // add the next pixel of the next line in the column x
      line_buffer[i].blue+=imageIn->data[W*endy+i].blue;
      line_buffer[i].red+=imageIn->data[W*endy+i].red;
      line_buffer[i].green+=imageIn->data[W*endy+i].green;
    }
  }

  else if (endy >= H ){
    // for the last lines, we just need to subtract the first added line
    endy = H-1;
    for(int i=0; i<W; i++){
      line_buffer[i].blue-=imageIn->data[W*(starty-1)+i].blue;
      line_buffer[i].red-=imageIn->data[W*(starty-1)+i].red;
      line_buffer[i].green-=imageIn->data[W*(starty-1)+i].green;
    }
  }else{
    // general case
    // add the next line and remove the first added line
    for(int i=0; i<W; i++){
      line_buffer[i].blue+=imageIn->data[W*endy+i].blue-imageIn->data[W*(starty-1)+i].blue;
      line_buffer[i].red+=imageIn->data[W*endy+i].red-imageIn->data[W*(starty-1)+i].red;
      line_buffer[i].green+=imageIn->data[W*endy+i].green-imageIn->data[W*(starty-1)+i].green;
    }
  }
  // End of line_buffer initialisation.


  sum_green =0;
  sum_red = 0;
  sum_blue = 0;
  for(int senterX = 0; senterX < W; senterX++) {
    // in this loop, we do exactly the same thing as before but only with the array line_buffer

    int startx = senterX-size;
    int endx = senterX+size;

    if (startx <=0){
      startx = 0;
      if(senterX==0){
        for (int x=startx; x < endx; x++){
          sum_red += line_buffer[x].red;
          sum_green += line_buffer[x].green;
          sum_blue += line_buffer[x].blue;
        }
      }
      sum_red +=line_buffer[endx].red;
      sum_green +=line_buffer[endx].green;
      sum_blue +=line_buffer[endx].blue;
    }else if (endx >= W){
      endx = W-1;
      sum_red -=line_buffer[startx-1].red;
      sum_green -=line_buffer[startx-1].green;
      sum_blue -=line_buffer[startx-1].blue;

    }else{
      sum_red += (line_buffer[endx].red-line_buffer[startx-1].red);
      sum_green += (line_buffer[endx].green-line_buffer[startx-1].green);
      sum_blue += (line_buffer[endx].blue-line_buffer[startx-1].blue);
    }

    // we save each pixel in the output image
    float count_rec=1.0 / ((endx-startx+1)*(endy-starty+1));

    imageOut->data[senterY*W + senterX].red = sum_red*count_rec;
    imageOut->data[senterY*W + senterX].green = sum_green*count_rec;
    imageOut->data[senterY*W + senterX].blue = sum_blue*count_rec;
  }
}


// Perform the new idea:
// The code in this function should run in parallel
// Try to find a good strategy for dividing the problem into individual parts.
// Using OpenMP inside this function itself might be avoided
// You may be able to do this only with a single OpenMP directive
void performNewIdeaIteration(AccurateImage *imageOut, AccurateImage *imageIn,int size) {

  int W = imageIn->x;
  int H = imageIn->y;

  // line buffer that will save the sum of some pixel in the column
  AccuratePixel *line_buffer = (AccuratePixel*) malloc(W*sizeof(AccuratePixel));
  memset(line_buffer,0,W*sizeof(AccuratePixel));
  AccuratePixel *line_buffer1 = (AccuratePixel*) malloc(W*sizeof(AccuratePixel));
  memset(line_buffer1,0,W*sizeof(AccuratePixel));
  AccuratePixel *line_buffer2 = (AccuratePixel*) malloc(W*sizeof(AccuratePixel));
  memset(line_buffer2,0,W*sizeof(AccuratePixel));
  AccuratePixel *line_buffer3 = (AccuratePixel*) malloc(W*sizeof(AccuratePixel));
  memset(line_buffer3,0,W*sizeof(AccuratePixel));

  int each = H/4;
  // Iterate over each line of pixelx.
  #pragma omp parallel for num_threads(4)
  for(int senterY = 0; senterY < H; senterY++) {
    // first and last line considered  by the computation of the pixel in the line senterY
    if (omp_get_thread_num() == 0) {
      if (senterY == 0) {
        for(int y=0; y < size; y++) {
          for(int i=0; i<W; i++){
            line_buffer[i].blue+=imageIn->data[W*y+i].blue;
            line_buffer[i].red+=imageIn->data[W*y+i].red;
            line_buffer[i].green+=imageIn->data[W*y+i].green;
          }
        }
      }
      bar(senterY, size, line_buffer, imageIn, imageOut);
    } else if (omp_get_thread_num() == 1) {
      //printf("%d, ", senterY);
      if (senterY == each) {
        for(int y=senterY-size-1; y < senterY+size; y++) {
          for(int i=0; i<W; i++){
            line_buffer1[i].blue+=imageIn->data[W*y+i].blue;
            line_buffer1[i].red+=imageIn->data[W*y+i].red;
            line_buffer1[i].green+=imageIn->data[W*y+i].green;
          }
        }
      }
      bar(senterY, size, line_buffer1, imageIn, imageOut);
    } else if (omp_get_thread_num() == 2) {
      if (senterY == 2*each) {
        for(int y=senterY-size-1; y < senterY+size; y++) {
          for(int i=0; i<W; i++){
            line_buffer2[i].blue+=imageIn->data[W*y+i].blue;
            line_buffer2[i].red+=imageIn->data[W*y+i].red;
            line_buffer2[i].green+=imageIn->data[W*y+i].green;
          }
        }
      }
      bar(senterY, size, line_buffer2, imageIn, imageOut);
    } else {
      if (senterY == 3*each) {
        for(int y=senterY-size-1; y < senterY+size; y++) {
          for(int i=0; i<W; i++){
            line_buffer3[i].blue+=imageIn->data[W*y+i].blue;
            line_buffer3[i].red+=imageIn->data[W*y+i].red;
            line_buffer3[i].green+=imageIn->data[W*y+i].green;
          }
        }
      }
      bar(senterY, size, line_buffer3, imageIn, imageOut);
    }
  }

  // free memory
  free(line_buffer);
}

int getValue(float value)
{
  if(value > 255) {
    return 255;
  } else if (value < -1.0) {
    value += 257;
    if(value > 255)
      return 255;
  } else if (value > -1.0 && value < 0.0) {
    return 0;
  }
  return (int) value;
}

    // Perform the final step, and save it as a ppm in imageOut
void performNewIdeaFinalization(AccurateImage *imageInSmall, AccurateImage *imageInLarge, PPMImage *imageOut)
{
  imageOut->x = imageInSmall->x;
  imageOut->y = imageInSmall->y;

  for(int i=0; i<imageInSmall->x * imageInSmall->y; i+=1)
  {
    float value = imageInLarge->data[i].red - imageInSmall->data[i].red;
    imageOut->data[i].red = getValue(value);
    value = imageInLarge->data[i].green - imageInSmall->data[i].green;
    imageOut->data[i].green = getValue(value);
    value = imageInLarge->data[i].blue - imageInSmall->data[i].blue;
    imageOut->data[i].blue = getValue(value);
  }
}

void fiveIterations(AccurateImage *imageNew, AccurateImage *imageUnchanged, AccurateImage *imageBuffer, int size)
{
  performNewIdeaIteration(imageNew, imageUnchanged, size);
  performNewIdeaIteration(imageBuffer, imageNew, size);
  performNewIdeaIteration(imageNew, imageBuffer, size);
  performNewIdeaIteration(imageBuffer, imageNew, size);
  performNewIdeaIteration(imageNew, imageBuffer, size);
}


int main(int argc, char** argv) {

	PPMImage *image;

	if(argc > 1) {
		image = readPPM("flower.ppm");
	} else {
		image = readStreamPPM(stdin);
	}

	AccurateImage *imageUnchanged = convertImageToNewFormat(image); // save the unchanged image from input image
	AccurateImage *imageBuffer = createEmptyImage(image);
	AccurateImage *imageSmall = createEmptyImage(image);
	AccurateImage *imageBig = createEmptyImage(image);

	PPMImage *imageOut;
	imageOut = (PPMImage *)malloc(sizeof(PPMImage));
	imageOut->data = (PPMPixel*)malloc(image->x * image->y * sizeof(PPMPixel));

	// Process the tiny case:
	fiveIterations(imageSmall, imageUnchanged, imageBuffer, 2);

	// Process the small case:
  fiveIterations(imageBig, imageUnchanged, imageBuffer, 3);

	// save tiny case result
	performNewIdeaFinalization(imageSmall,  imageBig, imageOut);
	if(argc > 1) {
		writePPM("flower_tiny.ppm", imageOut);
	} else {
		writeStreamPPM(stdout, imageOut);
	}

	// Process the medium case:
  fiveIterations(imageSmall, imageUnchanged, imageBuffer, 5);

	// save small case
	performNewIdeaFinalization(imageBig,  imageSmall,imageOut);
	if(argc > 1) {
		writePPM("flower_small.ppm", imageOut);
	} else {
		writeStreamPPM(stdout, imageOut);
	}

	// process the large case
  fiveIterations(imageBig, imageUnchanged, imageBuffer, 8);

	// save the medium case
	performNewIdeaFinalization(imageSmall,  imageBig, imageOut);
	if(argc > 1) {
		writePPM("flower_medium.ppm", imageOut);
	} else {
		writeStreamPPM(stdout, imageOut);
	}

	// free all memory structures
	freeImage(imageUnchanged);
	freeImage(imageBuffer);
	freeImage(imageSmall);
	freeImage(imageBig);
	free(imageOut->data);
	free(imageOut);
	free(image->data);
	free(image);

	return 0;
}
