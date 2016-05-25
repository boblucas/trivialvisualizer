#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "savepng.c"
#include "SDL_rotozoom.h"
#include <SDL.h>
#undef SAVE_32BIT_BMP

const int64_t WIDTH = 3200;
const int64_t HEIGHT = 1800;
const int64_t MULTI = 5;
const double RATIO = 16.0/9.0;

struct Point
{
	int64_t x;
	int64_t y;
};

SDL_Surface* remapCanvas(SDL_Surface* canvas)
{
	SDL_Surface* buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w/2, canvas->h/2, 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
	zoomSurfaceRGBA(canvas, buffer, 1);
	SDL_Surface* buffer2 = SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h, 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
	SDL_Rect r;
	r.x = canvas->w/4;
	r.y = canvas->h/4;
	r.w = canvas->w/2;
	r.h = canvas->h/2;
	SDL_BlitSurface(buffer, 0, buffer2, &r);
	SDL_FreeSurface(buffer);
	return buffer2;
}

void makeFrames()
{
	Point globalMin = {-WIDTH*MULTI, -HEIGHT*MULTI};
	Point min = {0,0};
	Point max = {0,0};
	Point position = {0, 0};
	
	uint64_t prime = 0;
	uint64_t previous = 0;
	
	int64_t blocks = 2;
	int64_t direction = 0;

	SDL_Surface* image = SDL_CreateRGBSurface(SDL_SWSURFACE, WIDTH*MULTI, HEIGHT*MULTI, 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
	SDL_Surface* frameOut = SDL_CreateRGBSurface(SDL_SWSURFACE, WIDTH, HEIGHT, 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
	SDL_FillRect(image, 0, 0);

	double nextFrame = 1;
	uint64_t currentStep = 0;
	uint64_t currentFrame = 0;
	
	for (std::string line; std::getline(std::cin, line);)
	{
		prime = std::atoll(line.c_str());
		
		if(direction == 0) position.x += prime - previous;
		if(direction == 1) position.y += prime - previous;
		if(direction == 2) position.x -= prime - previous;
		if(direction == 3) position.y -= prime - previous;

		min.x = position.x < min.x ? position.x : min.x;
		min.y = position.y < min.y ? position.y : min.y;
		max.x = position.x > max.x ? position.x : max.x;
		max.y = position.y > max.y ? position.y : max.y;

		++direction &= 3;
		previous = prime;

		if(currentStep >= nextFrame)
		{
			currentStep = 0;
			nextFrame += 0.05;
			nextFrame *= 1.01;

			SDL_Rect srcRect;
			srcRect.x = (min.x - globalMin.x)/blocks - 10;
			srcRect.y = (min.y - globalMin.y)/blocks - 10;
			srcRect.w = (max.x - min.x)/blocks + 20;
			srcRect.h = (max.y - min.y)/blocks + 20;

			if(((float)srcRect.w)/srcRect.h > RATIO)
			{
				int64_t diff = (srcRect.w / RATIO) - srcRect.h;
				srcRect.h += diff;
				srcRect.y -= diff/2;
			}
			else
			{
				int64_t diff = (srcRect.h * RATIO) - srcRect.w;
				srcRect.w += diff;
				srcRect.x -= diff/2;
			}


			SDL_Surface* clipped = SDL_CreateRGBSurfaceFrom(
				((uint8_t*)image->pixels) + (srcRect.y*image->pitch) + srcRect.x*3 , 
				srcRect.w, srcRect.h, 24, image->pitch, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
			zoomSurfaceRGBA(clipped, frameOut, 0);

			std::stringstream ss;
			ss << "video/frame_" << std::setfill('0') << std::setw(5) << currentFrame << ".bmp";
			std::cout << "writing " << ss.str().c_str() << " @ " << prime << std::endl;
			SDL_SaveBMP(frameOut, ss.str().c_str());

			currentFrame++;
		}

		int64_t x = (position.x - globalMin.x)/blocks + 1;
		int64_t y = (position.y - globalMin.y)/blocks + 1;
		uint32_t* pixel = (uint32_t*) (((uint8_t*)image->pixels) + (y*image->pitch) + x*3);
		uint32_t* last = (uint32_t*) (((uint8_t*)image->pixels) + (image->h*image->pitch));
		if(pixel > last)
		{
			blocks *= 2;
			globalMin.x *= 2;
			globalMin.y *= 2;
			
			SDL_Surface* newimage = remapCanvas(image);
			SDL_FreeSurface(image);
			image = newimage;
			x = (position.x - globalMin.x)/blocks + 1;
			y = (position.y - globalMin.y)/blocks + 1;
			pixel = (uint32_t*)(((uint8_t*)image->pixels) + (y*image->pitch) + x*3);
		}

		if( (*pixel & 0xff) < 250)
		{
			*pixel += (255/(blocks-1)) << 8;	
			//*pixel += (64/blocks) << 8;
		}
		//else if( ((*pixel >> 8) & 0xff) < 240)
		//	*pixel += 127 << 8;
		//else if( ((*pixel >> 16) & 0xff) < 240)
		//	*pixel += 127 << 16;

		currentStep++;
	}

}

int main(int argc, char* argv[])
{
	makeFrames( );
}