#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <png++/png.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

const int blocksize = 25;

struct Point
{
	int x;
	int y;
};

void savePNG(uint32_t* bitmap, int width, int height, const char *path)
{
	FILE * fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	size_t x, y;
	png_byte ** row_pointers = NULL;

	int status = -1;
	int pixel_size = 4;
	int depth = 8;
	
	fp = fopen (path, "wb");
	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct (png_ptr);

	png_set_IHDR (png_ptr,
				  info_ptr,
				  width,
				  height,
				  depth,
				  PNG_COLOR_TYPE_RGB,
				  PNG_INTERLACE_NONE,
				  PNG_COMPRESSION_TYPE_DEFAULT,
				  PNG_FILTER_TYPE_DEFAULT);

	png_init_io (png_ptr, fp);
	png_set_rows (png_ptr, info_ptr, (unsigned char**)&bitmap);
	png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
}

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

void makeImage(Point min, Point max)
{
	Point position = {0, 0};
	unsigned prime;
	unsigned previous = 0;
	int direction = 0;

	png::image<png::rgb_pixel> image((max.x-min.x) + 2, (max.y-min.y) + 2);
	for (std::string line; std::getline(std::cin, line);)
	{
		prime = std::atoi(line.c_str());
		
		if(direction == 0) position.x += prime - previous;
		if(direction == 1) position.y += prime - previous;
		if(direction == 2) position.x -= prime - previous;
		if(direction == 3) position.y -= prime - previous;

		++direction &= 3;
		previous = prime;

		png::rgb_pixel& pixel = image[(position.y - min.y)/blocksize + 1][(position.x - min.x)/blocksize + 1];

		if(pixel.blue < 255)
			pixel.blue += 4;
		else if(pixel.green < 255)
			pixel.green += 4;
		else if(pixel.red < 255)
			pixel.red += 4;

	}

	std::cout << "plotting done" << std::endl;
	image.write("image.png");
}

void blit(png::image<png::rgb_pixel>& from, png::image<png::rgb_pixel>& to, Point fromMin, Point fromMax, Point toMin, Point toMax)
{
	double widthScale  = (double)(toMax.x - toMin.x) / (double)(fromMax.x - fromMin.x); 
	double heightScale = (double)(toMax.y - toMin.y) / (double)(fromMax.y - fromMin.y); 
	//Letterbox
	double scale = widthScale < heightScale ? widthScale : heightScale;
	std::cout << "from min " << fromMin.x << " " << fromMin.y << std::endl;
	std::cout << "from max " << fromMax.x << " " << fromMax.y << std::endl;
	std::cout << "to min " << toMin.x << " " << toMin.y << std::endl;
	std::cout << "to max " << toMax.x << " " << toMax.y << std::endl;
	std::cout << "scale " << scale << std::endl;

	if(scale >= 1)
	{
		//Source material is smaller than destination, for upscaling we use nearest neighbour
		for(int y = toMin.y; y < toMax.y; y++)
		{
			for(int x = toMin.x; x < toMax.x; x++)
			{
				to[y][x].blue += from[floor((y - toMin.y)/scale) + fromMin.y][floor((x - toMin.x)/scale) + fromMin.x].blue;
			}
		}
	}
	else
	{
		for(int y = toMin.y; y < toMax.y; y++)
		{
			for(int x = toMin.x; x < toMax.x; x++)
			{
				to[y][x].blue = 0;
			}
		}
		//Source material is bigger than destination, interpolate
		for(int y = fromMin.y + 4; y < fromMax.y-4; y++)
		{
			for(int x = fromMin.x + 4; x < fromMax.x-4; x++)
			{
				to[floor((y - fromMin.y)/scale) + toMin.y][floor((x - fromMin.x)/scale) + toMin.x].blue += from[y][x].blue;
			}
		}

	}

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
		blocks = 4;

	png::image<png::rgb_pixel> image((globalMax.x-globalMin.x)/blocks + 2, (globalMax.y-globalMin.y)/blocks + 2);
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
			png::image<png::rgb_pixel> frameOut(1920, 1080);
			blit(image, frameOut, {(min.x - globalMin.x)/blocks + 1, (min.y - globalMin.y)/blocks + 1}, {(max.x - globalMin.x)/blocks + 1, (max.y - globalMin.y)/blocks + 1}, {0,0}, {1920, 1080});
			std::stringstream ss;
			ss << "video/frame_" << std::setfill('0') << std::setw(5) << currentFrame << ".png";
			std::cout << "writing " << ss.str().c_str() << " @ " << prime << std::endl;
			frameOut.write(ss.str().c_str());
			currentFrame++;
		}

		png::rgb_pixel& pixel = image[(position.y - globalMin.y)/blocks + 1][(position.x - globalMin.x)/blocks + 1];

		if(pixel.blue < 250)
			pixel.blue += 127;
		else if(pixel.green < 250)
			pixel.green += 127;
		else if(pixel.red < 250)
			pixel.red += 127;

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
		else
			makeImage( { std::atoi(argv[1]),std::atoi(argv[2]) }, { std::atoi(argv[3]),std::atoi(argv[4]) } );
	}
}