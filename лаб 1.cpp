#include "stdafx.h"
#include <iostream>
#include <fstream>

using namespace std;

#pragma pack(push, 1)// ������������� ������������ �� 1 �����
struct BMPHeader {
	char signature[2];
	uint32_t fileSize;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t dataOffset;
	uint32_t headerSize;
	int32_t width;
	int32_t height;
	uint16_t planes;
	uint16_t bitsPerPixel;
	uint32_t compression;
	uint32_t imageSize;
	int32_t xPixelsPerMeter;
	int32_t yPixelsPerMeter;
	uint32_t colorsUsed;
	uint32_t importantColors;
};
#pragma pack(pop) // ��������������� ���������� ������������

int main() {
	ifstream file("salim.bmp", ios::binary);
	if (!file) {
		cout << "Failed to open image file." << endl;
		return 1;
	}

	BMPHeader header;
	file.read(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

	// ���������, ��� ����������� �������� 24-������ BMP
	if (header.bitsPerPixel != 24) {
		cout << "Only 24-bit BMP images are supported." << endl;
		return 1;
	}

	// ��������� ���������� ������ ��� �������� �����������
	int imageSize = header.width * header.height * (header.bitsPerPixel / 8);

	// ��������� ����������� � ������
	char* imageData = new char[imageSize];
	file.seekg(header.dataOffset, ios::beg);
	file.read(imageData, imageSize);

	// ������� ����� ��������� ��� ����������� �����������
	BMPHeader rotatedHeader = header;
	rotatedHeader.width = header.height;
	rotatedHeader.height = header.width;

	// ��������� ���������� ������ ��� ����������� �����������
	int rotatedImageSize = rotatedHeader.width * rotatedHeader.height * (rotatedHeader.bitsPerPixel / 8);

	// ������� ������ ��� ����������� �����������
	char* rotatedImageData = new char[rotatedImageSize];

	// ������������ ����������� �� ������� �������
	for (int y = 0; y < header.height; y++) {
		for (int x = 0; x < header.width; x++) {
			int rotatedX = y;
			int rotatedY = header.width - x - 1;

			int originalIndex = (y * header.width + x) * 3;
			int rotatedIndex = (rotatedY * rotatedHeader.width + rotatedX) * 3;

			rotatedImageData[rotatedIndex] = imageData[originalIndex];
			rotatedImageData[rotatedIndex + 1] = imageData[originalIndex + 1];
			rotatedImageData[rotatedIndex + 2] = imageData[originalIndex + 2];
		}
	}

	// ��������� ��������� �� ������� �������
	ofstream outputFile("rotateImageClockwise.bmp", ios::binary);
	outputFile.write(reinterpret_cast<char*>(&rotatedHeader), sizeof(BMPHeader));
	outputFile.write(rotatedImageData, rotatedImageSize);

	// ������������ ����������� ������ ������� �������
	for (int y = 0; y < header.height; y++) {
		for (int x = 0; x < header.width; x++) {
			int rotatedX = header.height - y - 1;
			int rotatedY = x;

			int originalIndex = (y * header.width + x) * 3;
			int rotatedIndex = (rotatedY * rotatedHeader.width + rotatedX) * 3;

			rotatedImageData[rotatedIndex] = imageData[originalIndex];
			rotatedImageData[rotatedIndex + 1] = imageData[originalIndex + 1];
			rotatedImageData[rotatedIndex + 2] = imageData[originalIndex + 2];
		}
	}

	// ��������� ��������� ������ ������� �������
	ofstream outputFile1("rotateImageCounterClockwise.bmp", ios::binary);
	outputFile1.write(reinterpret_cast<char*>(&rotatedHeader), sizeof(BMPHeader));
	outputFile1.write(rotatedImageData, rotatedImageSize);

	// ����������� ������
	delete[] imageData;
	delete[] rotatedImageData;

	file.close();
	outputFile.close();

	return 0;
}
