#include "print.hh"

// putchar_s(const char) Must be defined in the applicaton code somewhere
void putchar_s(const char c);

void printone(const unsigned char *str)
{
	while (*str)
		putchar_s(*str++);
}

void printone(const char *str)
{
	while (*str)
		putchar_s(*str++);
}

void printone(char c) { putchar_s(c); }

void printone(Hex hex)
{
	auto value = hex.x;

	if (!value) {
		printone("0");
		return;
	}
	/// TODO: read left to right, not right to left, to avoid flipping digits
	constexpr int MAX_DIGITS = 8;
	char buf[MAX_DIGITS + 1];
	int len = 0;
	do {
		const char digit = (char)(value & 0xF);
		buf[len++] = digit < 10 ? '0' + digit : 'A' + digit - 10;
		value >>= 4;
	} while (value && (len < MAX_DIGITS));
	buf[len] = '\0';

	for (int i = 0; i < len / 2; i++) {
		auto tmp = buf[i];
		buf[i] = buf[len - i - 1];
		buf[len - i - 1] = tmp;
	}

	printone(buf);
}

void printone(unsigned long long value)
{
	if (!value) {
		printone("0");
		return;
	}

	constexpr int MAX_DIGITS = 20; // 2^64 has 20 digits in base-10
	char buf[MAX_DIGITS + 1];
	int len = 0;
	do {
		const char digit = (char)(value % 10);
		buf[len++] = '0' + digit;
		value /= 10;
	} while (value && (len < MAX_DIGITS));
	buf[len] = '\0';

	for (int i = 0; i < len / 2; i++) {
		auto tmp = buf[i];
		buf[i] = buf[len - i - 1];
		buf[len - i - 1] = tmp;
	}

	printone(buf);
}

void printone(unsigned long value) { printone((unsigned long long)value); }
void printone(unsigned value) { printone((unsigned long long)value); }

void printone(long long value)
{
	if (value < 0) {
		printone('-');
		printone(-value);
	} else
		printone((unsigned long long)value);
}
void printone(long value) { printone((long long)value); }
void printone(int value) { printone((long long)value); }
