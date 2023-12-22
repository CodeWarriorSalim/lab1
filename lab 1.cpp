#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

#pragma pack(push, 1)// Setting the alignment to 1 byte
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
#pragma pack(pop) // Restoring the previous alignment
// Function to apply a 3x3 Gaussian filter to an image
void applyGaussianFilter(char* imageData, int width, int height) {
    float kernel[3][3] = {
        {1.0 / 16, 2.0 / 16, 1.0 / 16},
        {2.0 / 16, 4.0 / 16, 2.0 / 16},
        {1.0 / 16, 2.0 / 16, 1.0 / 16}
    };

    char* tempImageData = new char[width * height * 3];

    // Apply the filter to each pixel in the image
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            for (int channel = 0; channel < 3; channel++) {
                float sum = 0.0;

                // Apply the convolution operation
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        int pixelX = x + i;
                        int pixelY = y + j;
                        int index = (pixelY * width + pixelX) * 3 + channel;

                        sum += static_cast<float>(imageData[index]) * kernel[i + 1][j + 1];
                    }
                }

                tempImageData[(y * width + x) * 3 + channel] = static_cast<char>(sum);
            }
        }
    }

    // Copy the filtered data back to the original image
    memcpy(imageData, tempImageData, width * height * 3);

    delete[] tempImageData;
}

int main() {
	ifstream file("salim.bmp", ios::binary);
	if (!file) {
		cout << "Failed to open image file." << endl;
		return 1;
	}

	BMPHeader header;
	file.read(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

	// Check that the image is a 24-bit BMP
	if (header.bitsPerPixel != 24) {
		cout << "Only 24-bit BMP images are supported." << endl;
		return 1;
	}

	// Calculating the amount of memory to load the image
	int imageSize = header.width * header.height * (header.bitsPerPixel / 8);

	// Loading the image into memory
	char* imageData = new char[imageSize];
	file.seekg(header.dataOffset, ios::beg);
	file.read(imageData, imageSize);

	// Creating a new title for the rotated image
	BMPHeader rotatedHeader = header;
	rotatedHeader.width = header.height;
	rotatedHeader.height = header.width;

	// Calculating the amount of memory for the rotated image
	int rotatedImageSize = rotatedHeader.width * rotatedHeader.height * (rotatedHeader.bitsPerPixel / 8);

	// Creating an array for the rotated image
	char* rotatedImageData = new char[rotatedImageSize];

	// Rotate the image clockwise
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

	// Saving the result clockwise
	ofstream outputFile("rotateImageClockwise.bmp", ios::binary);
	outputFile.write(reinterpret_cast<char*>(&rotatedHeader), sizeof(BMPHeader));
	outputFile.write(rotatedImageData, rotatedImageSize);

	// Rotate the image counterclockwise
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

	// Saving the result counterclockwise
	ofstream outputFile1("rotateImageCounterClockwise.bmp", ios::binary);
	outputFile1.write(reinterpret_cast<char*>(&rotatedHeader), sizeof(BMPHeader));
	outputFile1.write(rotatedImageData, rotatedImageSize);
	 // Apply Gaussian filter to the clockwise rotated image
    applyGaussianFilter(rotatedImageData, rotatedHeader.width, rotatedHeader.height);

    // Saving the result clockwise with Gaussian filter
    ofstream outputFile2("rotateImageClockwiseWithGaussianFilter.bmp", ios::binary);
    outputFile2.write(reinterpret_cast<char*>(&rotatedHeader), sizeof(BMPHeader));
    outputFile2.write(rotatedImageData, rotatedImageSize);

    // Freeing up memory
    delete[] imageData;
    delete[] rotatedImageData;

    file.close();
    outputFile.close();
    outputFile1.close();
    outputFile2.close();


    return 0;


}
