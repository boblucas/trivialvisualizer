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

const int blocksize = 25;

struct Point
{
	int x;
	int y;
};

void getDimensions()
{
	Point min = {0,0};
	Point max = {0,0};
	Point position = {0, 0};

	unsigned prime;
	unsigned previous = 0;
	int direction = 0;

	for (std::string line; std::getline(std::cin, line);)
	{
		prime = std::atoi(line.c_str());
		
		position.x += (prime - previous) * ((direction == 0) - (direction == 2));
		position.y += (prime - previous) * ((direction == 1) - (direction == 3));

		min.x = position.x < min.x ? position.x : min.x;
		min.y = position.y < min.y ? position.y : min.y;
		max.x = position.x > max.x ? position.x : max.x;
		max.y = position.y > max.y ? position.y : max.y;
		
		++direction &= 3;
		previous = prime;
	}

	std::cout << min.x << ", " << min.y << "   " << max.x << ", " << max.y << std::endl; 
}

void makeFrames(Point globalMin, Point globalMax)
{
	Point min = {0,0};
	Point max = {0,0};
	Point position = {0, 0};
	
	unsigned prime;
	unsigned previous = 0;
	
	int direction = 0;

	int blocks = 2;
	if(globalMax.x - globalMin.x > 50000)
		blocks = 8;

	SDL_Surface* image = SDL_CreateRGBSurface(SDL_SWSURFACE, (globalMax.x-globalMin.x)/blocks + 2, (globalMax.y-globalMin.y)/blocks + 2, 32, 0xFF0000, 0xFF00, 0xFF, 0);
	SDL_Surface* frameOut = SDL_CreateRGBSurface(SDL_SWSURFACE, 1920, 1080, 32, 0xFF0000, 0xFF00, 0xFF, 0);
	SDL_FillRect(image, 0, 0xFF000000);

	double nextFrame = 1;
	int currentStep = 0;
	int currentFrame = 0;
	
	for (std::string line; std::getline(std::cin, line);)
	{
		prime = std::atoi(line.c_str());
		
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
			nextFrame += 0.1;

			SDL_Rect srcRect;
			srcRect.x = (min.x - globalMin.x)/blocks;
			srcRect.y = (min.y - globalMin.y)/blocks;
			srcRect.w = (max.x - min.x)/blocks + 1;
			srcRect.h = (max.y - min.y)/blocks + 1;

			if(((float)srcRect.w)/srcRect.h > 16.0/9.0)
				srcRect.h = (srcRect.w / (16.0/9.0));
			else
				srcRect.w = (srcRect.h * (16.0/9.0));

			SDL_Surface* clipped = SDL_CreateRGBSurfaceFrom( ((uint8_t*)image->pixels) + (srcRect.y*image->pitch) + srcRect.x*4 , srcRect.w, srcRect.h, 32, image->pitch, 0xFF0000, 0xFF00, 0xFF, 0);

			SDL_BlitSurface(image, &srcRect, clipped, 0);
			zoomSurfaceRGBA(clipped, frameOut, 0);

			std::stringstream ss;
			ss << "video/frame_" << std::setfill('0') << std::setw(5) << currentFrame << ".png";
			std::cout << "writing " << ss.str().c_str() << " @ " << prime << std::endl;
			SDL_SavePNG_RW(frameOut, SDL_RWFromFile(ss.str().c_str(), "wb"), 1);

			currentFrame++;
		}

		int x = (position.x - globalMin.x)/blocks + 1;
		int y = (position.y - globalMin.y)/blocks + 1;
		uint32_t* pixel = (uint32_t*)(((uint8_t*)image->pixels) + (y*image->pitch) + x*4);

		if( (*pixel & 0xff) < 250)
			*pixel += 50;
		//else if( ((*pixel >> 8) & 0xff) < 240)
		//	*pixel += 127 << 8;
		//else if( ((*pixel >> 16) & 0xff) < 240)
		//	*pixel += 127 << 16;

		currentStep++;
	}
}

int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		getDimensions();
	}
	else
	{
		if(argv[1][0] == 'v')
			makeFrames( { std::atoi(argv[2]),std::atoi(argv[3]) }, { std::atoi(argv[4]),std::atoi(argv[5]) } );
		//else
		//	makeImage( { std::atoi(argv[1]),std::atoi(argv[2]) }, { std::atoi(argv[3]),std::atoi(argv[4]) } );
	}
}