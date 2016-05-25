#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

//Video resolution
const int64_t WIDTH = 1920;
const int64_t HEIGHT = 1080;
//Bigger canvas size create better looking images
const int64_t MULTI = ((1<<14)-1) / WIDTH;
const double RATIO = ((float)WIDTH)/((float)HEIGHT);

struct Point
{
	int64_t x;
	int64_t y;
};
Point min(const Point& p, const Point& q) { return {p.x < q.x ? p.x : q.x, p.y < q.y ? p.y : q.y}; }
Point max(const Point& p, const Point& q) { return {p.x > q.x ? p.x : q.x, p.y > q.y ? p.y : q.y}; }

struct ColorRGB
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct Bitmap
{
	unsigned w;
	unsigned h;
	unsigned pitch;
	ColorRGB* pixels;
	bool soft = false;

	Bitmap(unsigned w, unsigned h, unsigned pitch, ColorRGB* pixels) : w(w), h(h), pitch(pitch), pixels(pixels), soft(true) {}
	Bitmap(unsigned w, unsigned h) : w(w), h(h), pitch(w), pixels(new ColorRGB[w*h])
	{
		std::memset(pixels, 0, w*h*sizeof(ColorRGB));
	}
	Bitmap(const Bitmap& original) : w(original.w), h(original.h), pitch(original.pitch), pixels(new ColorRGB[w*h])
	{
		std::memcpy(pixels, original.pixels, w*pitch);
	}
	~Bitmap()
	{
		if(!soft) delete[] pixels;
	}
};

Bitmap softClip(const Bitmap& bitmap, unsigned x, unsigned y, unsigned w, unsigned h)
{
	return Bitmap(w, h, bitmap.w, bitmap.pixels + y*bitmap.pitch + x);
}
void blit(const Bitmap& source, Bitmap& destination, unsigned destX, unsigned destY)
{
	for(unsigned y = 0; y < source.h; ++y)
		std::memcpy(destination.pixels + (y+destY)*destination.w + destX, source.pixels + y*source.pitch, source.w*sizeof(ColorRGB));
}

void saveBMP(const Bitmap& image, const char* filename);

//Stolen and modified from SDL_rotozoom
void zoom(const Bitmap& src, Bitmap& dst)
{
	int sx = (int)(1048576.0 * (float)src.w / (float)dst.w);
	int sy = (int)(1048576.0 * (float)src.h / (float)dst.h);

	int* sax = new int[dst.w + 1];
	int* say = new int[dst.h + 1];
	int* csax = sax;
	int* csay = say;

	for (unsigned x = 0, csx = 0; x <= dst.w; x++)
	{
		*(csax++) = csx;
		csx &= 0xfffff;
		csx += sx;
	}

	for (unsigned y = 0, csy = 0; y <= dst.h; y++)
	{
		*(csay++) = csy;
		csy &= 0xfffff;
		csy += sy;
	}

	ColorRGB* sp  = src.pixels;
	ColorRGB* csp = src.pixels;
	ColorRGB* dp  = dst.pixels;

	csay = say;
	for (unsigned y = 0; y < dst.h; y++)
	{
		sp = csp;
		csax = sax;
		for (unsigned x = 0; x < dst.w; x++)
		{
			*dp = *sp;
			sp += (*(++csax) >> 20);
			dp++;
		}
		csp += (*(++csay) >> 20) * (src.pitch);
		dp += dst.pitch - dst.w;
	}

	delete[] sax;
	delete[] say;
}

void letterbox(const Bitmap& src, int x, int y, int w, int h, Bitmap& dst)
{
	if(((float)w)/h > RATIO)
	{
		int64_t diff = (w / RATIO) - h;
		h += diff;
		y -= diff/2;
	}
	else
	{
		int64_t diff = (h * RATIO) - w;
		w += diff;
		x -= diff/2;
	}

	zoom(softClip(src, x, y, w, h), dst);
}

Bitmap* remapCanvas(Bitmap& canvas)
{
	Bitmap buffer(canvas.w/2, canvas.h/2);
	zoom(canvas, buffer);
	Bitmap* buffer2 = new Bitmap(canvas.w, canvas.h);
	blit(buffer, *buffer2, canvas.w/4, canvas.h/4);
	return buffer2;
}

void makeFrames()
{
	Point topLeft = {0,0};
	Point bottomRight = {0,0};
	Point position = {0, 0};
	
	uint64_t prime = 0;
	uint64_t previous = 0;
	
	int64_t blocks = 2;
	int64_t direction = 0;

	Bitmap image = Bitmap(WIDTH*MULTI, HEIGHT*MULTI);
	Bitmap frameOut = Bitmap(WIDTH, HEIGHT);

	double nextFrame = 1;
	uint64_t currentStep = 0;
	uint64_t currentFrame = 0;
	
	for (std::string line; std::getline(std::cin, line);)
	{
		prime = std::atoll(line.c_str());
		position.x += (~direction & 1) * -(direction-1) * (prime - previous);
		position.y += ( direction & 1) * -(direction-2) * (prime - previous);
		previous = prime;
		++direction &= 3;

		topLeft = min(position, topLeft);
		bottomRight = max(position, bottomRight);

		if(currentStep >= nextFrame)
		{
			currentStep = 0;
			nextFrame += 0.05;
			nextFrame *= 1.01;

			{
				int64_t x = (topLeft.x + (WIDTH *MULTI*blocks/2))/blocks - 10;
				int64_t y = (topLeft.y + (HEIGHT*MULTI*blocks/2))/blocks - 10;
				int64_t w = (bottomRight.x - topLeft.x)/blocks + 20;
				int64_t h = (bottomRight.y - topLeft.y)/blocks + 20;
				letterbox(image, x, y, w, h, frameOut);
			}

			std::stringstream ss;
			ss << "video/frame_" << std::setfill('0') << std::setw(5) << currentFrame << ".bmp";
			saveBMP(frameOut, ss.str().c_str());
			std::cout << "wrote " << ss.str().c_str() << " @ " << prime << std::endl;

			currentFrame++;
		}

		int64_t x = (position.x + (WIDTH *MULTI*blocks/2) )/blocks + 1;
		int64_t y = (position.y + (HEIGHT*MULTI*blocks/2) )/blocks + 1;
		ColorRGB* pixel = image.pixels + y*image.pitch + x;
		while(pixel > image.pixels + image.h*image.pitch)
		{
			blocks *= 2;
			image = *remapCanvas(image);
			x = (position.x + (WIDTH *MULTI*blocks/2))/blocks + 1;
			y = (position.y + (HEIGHT*MULTI*blocks/2))/blocks + 1;
			pixel = image.pixels + y*image.pitch + x;
		}

		if( (*pixel).g < 255)
			(*pixel).g += (255/(blocks-1));

		currentStep++;
	}

}

int main()
{
	makeFrames();
	return 0;
}

struct BMPHeader
{
    char B = 'B'; char M = 'M';
    uint32_t filesize;
    uint32_t reserved = 0;
    uint32_t offset = sizeof(BMPHeader);
    uint32_t headerSize = 12;
    uint16_t width, height;
	uint16_t planes = 1;
    uint16_t bpp = 24;
    BMPHeader(int width, int height) : filesize(sizeof(BMPHeader) + width*height*3), width(width), height(height){}
} __attribute__((packed));

void saveBMP(const Bitmap& image, const char* filename)
{
    FILE* file = fopen(filename, "wb");
    BMPHeader header = BMPHeader(image.w, image.h);
    fwrite(&header, sizeof(BMPHeader), 1, file);
    
    ColorRGB* bits = image.pixels + (image.h * image.pitch);
    while (bits > image.pixels)
        fwrite(bits -= image.pitch, 3, image.w, file);

    fclose(file);
}
