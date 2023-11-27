#pragma once

struct Hex {
	unsigned x;
};

void printone(const unsigned char *str);
void printone(const char *str);
void printone(char str);

void printone(int value);
void printone(unsigned value);
void printone(long value);
void printone(unsigned long value);
void printone(long long value);
void printone(unsigned long long value);

void printone(Hex value);

template<typename... Types>
void print(Types... args)
{
	(printone(args), ...);
}
