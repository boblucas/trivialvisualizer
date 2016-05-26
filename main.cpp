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
//Bigger canvas size creates better looking images
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
	uint8_t b;
	uint8_t g;
	uint8_t r;

	ColorRGB operator=(const uint32_t& rhs)
	{
		int level = rhs / 1000;
		b = level > 255 ? 255 : level;
		level -= 255;
		g = level > 255 ? 255 : (level > 0 ? level : 0);
		level -= 255;
		r = level > 255 ? 255 : (level > 0 ? level : 0);
		return *this;
	}
};

template<typename T>
struct Bitmap
{
	unsigned w;
	unsigned h;
	unsigned pitch;
	T* pixels;
	bool soft = false;

	Bitmap(unsigned w, unsigned h, unsigned pitch, T* pixels) : w(w), h(h), pitch(pitch), pixels(pixels), soft(true) {}
	Bitmap(unsigned w, unsigned h) : w(w), h(h), pitch(w), pixels(new T[w*h])
	{
		std::memset(pixels, 0, w*h*sizeof(T));
	}
	Bitmap(const Bitmap<T>& original) : w(original.w), h(original.h), pitch(original.pitch), pixels(new T[w*h])
	{
		std::memcpy(pixels, original.pixels, w*pitch);
	}
	~Bitmap()
	{
		if(!soft) delete[] pixels;
	}
};

template<typename T>
Bitmap<T> softClip(const Bitmap<T>& bitmap, unsigned x, unsigned y, unsigned w, unsigned h)
{
	return Bitmap<T>(w, h, bitmap.w, bitmap.pixels + y*bitmap.pitch + x);
}
template<typename T>
void blit(const Bitmap<T>& source, Bitmap<T>& destination, unsigned destX, unsigned destY)
{
	for(unsigned y = 0; y < source.h; ++y)
		std::memcpy(destination.pixels + (y+destY)*destination.w + destX, source.pixels + y*source.pitch, source.w*sizeof(T));
}

void saveBMP(const Bitmap<ColorRGB>& image, const char* filename);

//Stolen and modified from SDL_rotozoom
template<typename T, typename U>
void zoom(const Bitmap<T>& src, Bitmap<U>& dst)
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

	T* sp  = src.pixels;
	T* csp = src.pixels;
	U* dp  = dst.pixels;

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

template<typename T, typename U>
void letterbox(const Bitmap<T>& src, int x, int y, int w, int h, Bitmap<U>& dst)
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

	zoom<>(softClip<>(src, x, y, w, h), dst);
}

void remapCanvas(Bitmap<uint32_t>& canvas)
{
	Bitmap<uint32_t> buffer(canvas.w/2, canvas.h/2);
	zoom<>(canvas, buffer);
	std::memset(canvas.pixels, 0, canvas.w*canvas.h*sizeof(uint32_t));
	blit<>(buffer, canvas, canvas.w/4, canvas.h/4);
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

	Bitmap<uint32_t> image = Bitmap<uint32_t>(WIDTH*MULTI, HEIGHT*MULTI);
	Bitmap<ColorRGB> frameOut = Bitmap<ColorRGB>(WIDTH, HEIGHT);

	double nextFrame = 1;
	uint64_t currentStep = 0;
	uint64_t currentFrame = 0;
	double illuminationFactor = 1.5;
	
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
				letterbox<>(image, x, y, w, h, frameOut);
			}

			std::stringstream ss;
			ss << "video/frame_" << std::setfill('0') << std::setw(5) << currentFrame << ".bmp";
			saveBMP(frameOut, ss.str().c_str());
			std::cout << "wrote " << ss.str().c_str() << " @ " << prime << std::endl;

			currentFrame++;
		}

		int64_t x = (position.x + (WIDTH *MULTI*blocks/2) )/blocks + 1;
		int64_t y = (position.y + (HEIGHT*MULTI*blocks/2) )/blocks + 1;
		uint32_t* pixel = image.pixels + y*image.pitch + x;
		while(pixel > image.pixels + image.h*image.pitch)
		{
			blocks *= 2;
			illuminationFactor = (1.0/((double)blocks*blocks)) * log(currentStep);

			std::cout << "blocksize is now " << blocks << std::endl;
			remapCanvas(image);
			x = (position.x + (WIDTH *MULTI*blocks/2))/blocks + 1;
			y = (position.y + (HEIGHT*MULTI*blocks/2))/blocks + 1;
			pixel = image.pixels + y*image.pitch + x;
		}

		*pixel += ceil(illuminationFactor * 200000);
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

void saveBMP(const Bitmap<ColorRGB>& image, const char* filename)
{
    FILE* file = fopen(filename, "wb");
    BMPHeader header = BMPHeader(image.w, image.h);
    fwrite(&header, sizeof(BMPHeader), 1, file);
    
    ColorRGB* bits = image.pixels + (image.h * image.pitch);
    while (bits > image.pixels)
        fwrite(bits -= image.pitch, 3, image.w, file);

    fclose(file);
}
