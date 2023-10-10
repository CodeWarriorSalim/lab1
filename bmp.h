#pragma once
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
/* How many of the variables from below you use in your code? Like 3 or 4, 
 * so maybe it would be good to store all variables (starting somewhere from
 * the height) as one chunk of data. Try it*/
#pragma pack(push, 1)
struct BMPFileHeader {
	uint16_t file_type = 0x4D42;          // File type always BM which is 0x4D42
	uint32_t file_size = 0;               // Size of the file (in bytes)
	uint16_t reserved1 = 0;               // Reserved, always 0
	uint16_t reserved2 = 0;               // Reserved, always 0
	uint32_t offset_data = 0;             // Start position of pixel data (bytes from the beginning of the file)
};

struct BMPInfoHeader {
	uint32_t size{ 0 }; // Size of this header (in bytes)
	int32_t width{ 0 }; // width of bitmap in pixels
	int32_t height{ 0 }; // width of bitmap in pixels
						 //       (if positive, bottom-up, with origin in lower left corner)
						 //       (if negative, top-down, with origin in upper left corner)
	uint16_t planes{ 1 }; // No. of planes for the target device, this is always 1
	uint16_t bit_count{ 0 };   // No. of bits per pixel
	uint32_t compression{ 0 }; // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY
							   // UNCOMPRESSED BMP images
	uint32_t size_image{ 0 };  // 0 - for uncompressed images
	int32_t x_pixels_per_meter{ 0 };
	int32_t y_pixels_per_meter{ 0 };
	uint32_t colors_used{ 0 }; // No. color indexes in the color table. Use 0 for
							   // the max number of colors allowed by bit_count
	uint32_t colors_important{ 0 }; // No. of colors used for displaying the bitmap.
									// If 0 all colors are required
};

struct BMPColorHeader {
	uint32_t red_mask{ 0x00ff0000 };         // Bit mask for the red channel
	uint32_t green_mask{ 0x0000ff00 };       // Bit mask for the green channel
	uint32_t blue_mask{ 0x000000ff };        // Bit mask for the blue channel
	uint32_t alpha_mask{ 0xff000000 };       // Bit mask for the alpha channel
	uint32_t color_space_type{ 0x73524742 }; // Default "sRGB" (0x73524742)
	uint32_t unused[16]{ 0 };                // Unused data for sRGB color space
};
#pragma pack(pop)

struct BMP {
	BMPFileHeader file_header;
	BMPInfoHeader bmp_info_header;
	BMPColorHeader bmp_color_header;
	/* It would be better to create a struct for pixel, so it would be a vector of pixels */
	std::vector<uint8_t> data;

	BMP(const char *fname) { read(fname); }
	/* Definitions of member functions should be in BMP.cpp file. This allows separate compilation,
 	 * so recompilation after changes in a single file is a lot faster. */
	void read(const char *fname) {
		std::ifstream inp{ fname, std::ios_base::binary };
		if (inp) {
			inp.read((char *)&file_header, sizeof(file_header));
			if (file_header.file_type != 0x4D42) {
				throw std::runtime_error("Error! Unrecognized file format.");
			}
			inp.read((char *)&bmp_info_header, sizeof(bmp_info_header));
			/* We are solving the problem for 24 bits per pixel. Remove logic for 32 bits */
			// The BMPColorHeader is used only for transparent images
			if (bmp_info_header.bit_count == 32)
			{
				// Check if the file has bit mask color information
				if (bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader)))
				{
					inp.read((char *)&bmp_color_header, sizeof(bmp_color_header));
					// Check if the pixel data is stored as BGRA and if the color space
					// type is sRGB
					check_color_header(bmp_color_header);
				}
				else {
					std::cerr << "Error! The file \"" << fname
						<< "\" does not seem to contain bit mask information\n";
					throw std::runtime_error("Error! Unrecognized file format.");
				}
			}

			// Jump to the pixel data location
			inp.seekg(file_header.offset_data, inp.beg);

			// Adjust the header fields for output.
			// Some editors will put extra info in the image file, we only save the
			// headers and the data.
			if (bmp_info_header.bit_count == 32) {
				bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
				file_header.offset_data = sizeof(BMPFileHeader) +
					sizeof(BMPInfoHeader) +
					sizeof(BMPColorHeader);
			}
			else {
				bmp_info_header.size = sizeof(BMPInfoHeader);
				file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
			}
			file_header.file_size = file_header.offset_data;

			if (bmp_info_header.height < 0) {
				throw std::runtime_error("The program can treat only BMP images with "
					"the origin in the bottom left corner!");
			}

			data.resize(bmp_info_header.width * bmp_info_header.height *
				bmp_info_header.bit_count / 8);

			// Here we check if we need to take into account row padding
			if (bmp_info_header.width % 4 == 0) {
				/* Use only one type of cast: either a C style casts or C++ style.
				 * C-style cast are more dangerous. */
				inp.read((char *)data.data(), data.size()); 
				file_header.file_size += static_cast<uint32_t>(data.size());
			}
			else {
				row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;
				uint32_t new_stride = make_stride_aligned(4);
				std::vector<uint8_t> padding_row(new_stride - row_stride);

				for (int y = 0; y < bmp_info_header.height; ++y) {
					inp.read((char *)(data.data() + row_stride * y), row_stride);
					inp.read((char *)padding_row.data(), padding_row.size());
				}
				file_header.file_size +=
					static_cast<uint32_t>(data.size()) +
					bmp_info_header.height * static_cast<uint32_t>(padding_row.size());
			}
		}
		else {
			throw std::runtime_error("Unable to open the input image file.");
		}
	}

	BMP(int32_t width, int32_t height, bool has_alpha = true) {
		if (width <= 0 || height <= 0) {
			throw std::runtime_error(
				"The image width and height must be positive numbers.");
		}

		bmp_info_header.width = width;
		bmp_info_header.height = height;
		if (has_alpha) {
			bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
			file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) +
				sizeof(BMPColorHeader);

			bmp_info_header.bit_count = 32;
			bmp_info_header.compression = 3;
			row_stride = width * 4;
			data.resize(row_stride * height);
			file_header.file_size = file_header.offset_data + data.size();
		}
		else {
			bmp_info_header.size = sizeof(BMPInfoHeader);
			file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

			bmp_info_header.bit_count = 24;
			bmp_info_header.compression = 0;
			row_stride = width * 3;
			data.resize(row_stride * height);

			uint32_t new_stride = make_stride_aligned(4);
			file_header.file_size =
				file_header.offset_data + static_cast<uint32_t>(data.size()) +
				bmp_info_header.height * (new_stride - row_stride);
		}
	}

	void applyGaussianFilter(BMP &bmp) {
		/* Make kernel of arbitrary radius and make that radius a parameter */
		// Gaussian kernel for a 5x5 filter
		int kernel[5][5] = { { 1, 2, 4, 2, 1 },
		{ 2, 4, 8, 4, 2 },
		{ 4, 8, 16, 8, 4 },
		{ 2, 4, 8, 4, 2 },
		{ 1, 2, 4, 2, 1 } };

		int kernelSize = 5;

		int width = bmp.bmp_info_header.width;
		int height = bmp.bmp_info_header.height;

		std::vector<uint8_t> newData(width * height *
			(bmp.bmp_info_header.bit_count / 8));
		/* Too much nested loops. Split somehow: by function or somehow else */
		// Apply the Gaussian filter
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int totalR = 0, totalG = 0, totalB = 0;
				int divisor = 0;

				for (int ky = -2; ky <= 2; ++ky) {
					for (int kx = -2; kx <= 2; ++kx) {
						int posX = x + kx;
						int posY = y + ky;

						if (posX >= 0 && posX < width && posY >= 0 && posY < height) {
							int index =
								(posY * width + posX) * (bmp.bmp_info_header.bit_count / 8);
							totalR += bmp.data[index] * kernel[ky + 2][kx + 2];
							totalG += bmp.data[index + 1] * kernel[ky + 2][kx + 2];
							totalB += bmp.data[index + 2] * kernel[ky + 2][kx + 2];
							divisor += kernel[ky + 2][kx + 2];
						}
					}
				}

				int index = (y * width + x) * (bmp.bmp_info_header.bit_count / 8);
				newData[index] = totalR / divisor;
				newData[index + 1] = totalG / divisor;
				newData[index + 2] = totalB / divisor;

				if (bmp.bmp_info_header.bit_count == 32) {
					newData[index + 3] = bmp.data[index + 3]; // Alpha channel
				}
			}
		}

		// Update the data
		bmp.data = newData;
	}

	void rotateImageCounterClockwise(BMP &bmp) {
		int width = bmp.bmp_info_header.width;
		int height = bmp.bmp_info_header.height;

		std::vector<uint8_t> newData(width * height *
			(bmp.bmp_info_header.bit_count / 8));

		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < height; ++j) {
				int srcIndex = (j * width + i) * (bmp.bmp_info_header.bit_count / 8);
				int destIndex = (i * height + (height - j - 1)) *
					(bmp.bmp_info_header.bit_count / 8);

				for (int k = 0; k < (bmp.bmp_info_header.bit_count / 8); ++k) {
					newData[destIndex + k] = bmp.data[srcIndex + k];
				}
			}
		}

		// Update the width and height in the BMP header
		std::swap(bmp.bmp_info_header.width, bmp.bmp_info_header.height);

		// Update the data
		bmp.data = newData;
	}

	void rotateImageClockwise(BMP &bmp) {
		int width = bmp.bmp_info_header.width;
		int height = bmp.bmp_info_header.height;

		std::vector<uint8_t> newData(width * height *
			(bmp.bmp_info_header.bit_count / 8));

		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < height; ++j) {
				int srcIndex = (j * width + i) * (bmp.bmp_info_header.bit_count / 8);
				int destIndex = ((width - i - 1) * height + j) *
					(bmp.bmp_info_header.bit_count / 8);

				for (int k = 0; k < (bmp.bmp_info_header.bit_count / 8); ++k) {
					newData[destIndex + k] = bmp.data[srcIndex + k];
				}
			}
		}

		// Update the width and height in the BMP header
		std::swap(bmp.bmp_info_header.width, bmp.bmp_info_header.height);

		// Update the data
		bmp.data = newData;
	}

	void write(const char *fname) {
		std::ofstream of{ fname, std::ios_base::binary };
		if (of) {
			if (bmp_info_header.bit_count == 32) {
				write_headers_and_data(of);
			}
			else if (bmp_info_header.bit_count == 24) {
				if (bmp_info_header.width % 4 == 0) {
					write_headers_and_data(of);
				}
				else {
					uint32_t new_stride = make_stride_aligned(4);
					std::vector<uint8_t> padding_row(new_stride - row_stride);

					write_headers(of);

					for (int y = 0; y < bmp_info_header.height; ++y) {
						of.write((const char *)(data.data() + row_stride * y), row_stride);
						of.write((const char *)padding_row.data(), padding_row.size());
					}
				}
			}
			else {
				throw std::runtime_error(
					"The program can treat only 24 or 32 bits per pixel BMP files");
			}
		}
		else {
			throw std::runtime_error("Unable to open the output image file.");
		}
	}

	void set_pixel(uint32_t x0, uint32_t y0, uint8_t B, uint8_t G, uint8_t R,
		uint8_t A) {
		if (x0 >= (uint32_t)bmp_info_header.width ||
			y0 >= (uint32_t)bmp_info_header.height || x0 < 0 || y0 < 0) {
			throw std::runtime_error("The point is outside the image boundaries!");
		}

		uint32_t channels = bmp_info_header.bit_count / 8;
		data[channels * (y0 * bmp_info_header.width + x0) + 0] = B;
		data[channels * (y0 * bmp_info_header.width + x0) + 1] = G;
		data[channels * (y0 * bmp_info_header.width + x0) + 2] = R;
		if (channels == 4) {
			data[channels * (y0 * bmp_info_header.width + x0) + 3] = A;
		}
	}

private:
	uint32_t row_stride{ 0 };

	void write_headers(std::ofstream &of) {
		of.write((const char *)&file_header, sizeof(file_header));
		of.write((const char *)&bmp_info_header, sizeof(bmp_info_header));
		if (bmp_info_header.bit_count == 32) {
			of.write((const char *)&bmp_color_header, sizeof(bmp_color_header));
		}
	}

	void write_headers_and_data(std::ofstream &of) {
		write_headers(of);
		of.write((const char *)data.data(), data.size());
	}

	// Add 1 to the row_stride until it is divisible with align_stride
	uint32_t make_stride_aligned(uint32_t align_stride) {
		uint32_t new_stride = row_stride;
		while (new_stride % align_stride != 0) {
			new_stride++;
		}
		return new_stride;
	}

	// Check if the pixel data is stored as BGRA and if the color space type is
	// sRGB
	void check_color_header(BMPColorHeader &bmp_color_header) {
		BMPColorHeader expected_color_header;
		if (expected_color_header.red_mask != bmp_color_header.red_mask ||
			expected_color_header.blue_mask != bmp_color_header.blue_mask ||
			expected_color_header.green_mask != bmp_color_header.green_mask ||
			expected_color_header.alpha_mask != bmp_color_header.alpha_mask) {
			throw std::runtime_error(
				"Unexpected color mask format! The program expects the pixel data to "
				"be in the BGRA format");
		}
		if (expected_color_header.color_space_type !=
			bmp_color_header.color_space_type) {
			throw std::runtime_error(
				"Unexpected color space type! The program expects sRGB values");
		}
	}
};
#pragma once  // You already have that. Why there is more?
#pragma once
