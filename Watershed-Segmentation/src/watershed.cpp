// watershed.cpp : Defines the entry point for the console application.
//

#include <afxwin.h>  // necessary for MFC to work properly
#include "watershed.h"
#include <algorithm>
#include "../../src/blepo.h"
#include <stack>
#include <queue>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PI 3.1416

using namespace blepo;

/* Compute the Gaussian Convolution Kernel */
void computeGaussianKernel(float kernel[], float sigma) {
	int mu = round(2.5 * sigma - 0.5);
	int w = 2 * mu + 1;
	float sum = 0;
	for (int i = 0; i < w; ++i) {
		kernel[i] = exp(-(i - mu)*(i - mu) / (2 * sigma*sigma));
		sum += kernel[i];
	}
	for (int i = 0; i < w; ++i) {
		kernel[i] /= sum;
	}
}

/* Compute Gaussian Derivative Kernel */
void computeGaussianDerivativeKernel(float kernel[], float sigma) {
	int mu = round(2.5*sigma - 0.5);
	int w = 2 * mu + 1;
	float sum = 0;
	for (int i = 0; i < w; ++i) {
		kernel[i] = (i - mu) * exp(-(i - mu)*(i - mu) / (2 * sigma*sigma));
		sum += kernel[i] * i;
	}
	for (int i = 0; i < w; ++i) {
		kernel[i] /= sum;
	}
}

/* Convolution using separables required for computing X and Y Gradients */
void convolveSeparable(ImgGray img, int width, int halfwidth, float hKernel[], float vKernel[], ImgFloat& out) {
	ImgFloat tmp;
	tmp.Reset(img.Width(), img.Height());
	Set(&tmp, 0);
	int d;
	//Convolve Horizontal
	for (int i = 0; i < img.Height(); ++i) {
		for (int j = 0; j < img.Width(); ++j) {
			float val = 0;
			for (int k = 0; k < width; ++k) {
				if (j + halfwidth - k < 0)
					d = -j - halfwidth + k;
				else if (j + halfwidth - k >= img.Width())
					d = img.Width() - (j + halfwidth - k) - 1;
				else d = 0;
				val += (hKernel[k] * img(j + halfwidth - k + d, i));
			}
			tmp(j, i) = val;
		}
	}
	//Convolve Vertical
	for (int i = 0; i < img.Height(); ++i) {
		for (int j = 0; j < img.Width(); ++j) {
			float val = 0;
			for (int k = 0; k < width; ++k) {
				if (i + halfwidth - k < 0)
					d = -i - halfwidth + k;
				else if (i + halfwidth - k >= img.Height())
					d = img.Height() - (i + halfwidth - k) - 1;
				else d = 0;
				val += (vKernel[k] * tmp(j, i + halfwidth - k + d));
			}
			out(j, i) = val;
		}
	}
}

/* Floodfill required for Edge Linking with Hysteresis (Double Thresholding) */
void floodfill(const ImgInt& img, ImgInt& outputImg, int x, int y, int new_label) {
	if (x <0 || x >= img.Width() || y <0 || y >= img.Height()) return;

	stack<int> x_cor;
	stack<int> y_cor;
	int x1 = x;
	int y1 = y;
	x_cor.push(x1);
	y_cor.push(y1);
	while (!x_cor.empty() && !y_cor.empty())
	{
		x1 = x_cor.top();
		y1 = y_cor.top();
		x_cor.pop();
		y_cor.pop();
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				if (x1 + j < 0 || x1 + j >= img.Width() || y1 + i < 0 || y1 + i >= img.Height())
					continue;
				if (img(x1 + j, y1 + i) == img(x1, y1) && outputImg(x1 + j, y1 + i) != new_label)
				{
					x_cor.push(x1 + j);
					y_cor.push(y1 + i);
					outputImg(x1 + j, y1 + i) = new_label;
				}
			}
		}
	}
}


/* Floodfill required for Edge Linking with Hysteresis (Double Thresholding) */
void floodfillMarker(const ImgBinary& img, ImgInt& outputImg, int x, int y, int new_label) {
	if (x <0 || x >= img.Width() || y <0 || y >= img.Height()) return;

	stack<int> x_cor;
	stack<int> y_cor;
	int x1 = x;
	int y1 = y;
	x_cor.push(x1);
	y_cor.push(y1);
	while (!x_cor.empty() && !y_cor.empty())
	{
		x1 = x_cor.top();
		y1 = y_cor.top();
		x_cor.pop();
		y_cor.pop();
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				if (x1 + j < 0 || x1 + j >= img.Width() || y1 + i < 0 || y1 + i >= img.Height())
					continue;
				if (img(x1 + j, y1 + i) == 1 && outputImg(x1 + j, y1 + i) != new_label)
				{
					x_cor.push(x1 + j);
					y_cor.push(y1 + i);
					outputImg(x1 + j, y1 + i) = new_label;
				}
			}
		}
	}
}

/*Computing Chamfer Distance*/
void computeChamferDistance(ImgBinary img, ImgInt& chamferImg) {
	int max = img.Width() * img.Height() + 1;
	//  Set(chamfer_dist, bignum);
	int x, y;

	// forward pass
	for (y = 0; y<img.Height(); y++)
	{
		for (x = 0; x<img.Width(); x++)
		{
			if (img(x, y))  chamferImg(x, y) = 0;
			else
			{
				int dist = max;
				if (y > 0)  dist = std::min(dist, chamferImg(x, y - 1) + 1);
				if (x > 0)  dist = std::min(dist, chamferImg(x - 1, y) + 1);
				chamferImg(x, y) = dist;
			}
		}
	}

	// backward pass
	for (y = img.Height() - 1; y >= 0; y--)
	{
		for (x = img.Width() - 1; x >= 0; x--)
		{
			if (img(x, y))  chamferImg(x, y) = 0;
			else
			{
				int dist = chamferImg(x, y);
				if (y < img.Height() - 1)  dist = std::min(dist, chamferImg(x, y + 1) + 1);
				if (x < img.Width() - 1)  dist = std::min(dist, chamferImg(x + 1, y) + 1);
				chamferImg(x, y) = dist;
			}
		}
	}
}

/* Watershed algorithm*/
void waterShed(ImgInt& imgLabel, const ImgInt& imgChamfer, int listSize, std::vector< std::vector<pair<int,int>> >& vec) {
	int globalLabel = 0;
	int width = imgLabel.Width();
	int height = imgLabel.Height();
	std::queue<pair<int, int> > frontier;
	pair<int, int> tmp;
	for (int g = 0; g < listSize + 1; ++g) {
		std::vector<pair<int, int> >::const_iterator ptr = vec[g].begin();
		while (ptr != vec[g].end()) {
			if (ptr->first > 0 && imgLabel(ptr->first - 1, ptr->second) >= 0) {
				imgLabel(ptr->first, ptr->second) = imgLabel(ptr->first - 1, ptr->second);
				frontier.push((*ptr));
			}
			if (ptr->first < (width - 1) && imgLabel(ptr->first + 1, ptr->second) >= 0) {
				imgLabel(ptr->first, ptr->second) = imgLabel(ptr->first + 1, ptr->second);
				frontier.push((*ptr));
			}
			if (ptr->second > 0 && imgLabel(ptr->first, ptr->second - 1) >= 0) {
				imgLabel(ptr->first, ptr->second) = imgLabel(ptr->first, ptr->second - 1);
				frontier.push((*ptr));
			}
			if (ptr->second < (height - 1) && imgLabel(ptr->first, ptr->second + 1) >= 0) {
				imgLabel(ptr->first, ptr->second) = imgLabel(ptr->first, ptr->second + 1);
				frontier.push((*ptr));
			}
			++ptr;
		}


		while (!frontier.empty()) {
			tmp = frontier.front();
			frontier.pop();
			std::pair<int, int> tmp2;
			if (tmp.first > 0 && imgChamfer(tmp.first - 1, tmp.second) == g && imgLabel(tmp.first - 1, tmp.second) == -1) {
				imgLabel(tmp.first - 1, tmp.second) = imgLabel(tmp.first, tmp.second);
				tmp2.first = tmp.first - 1;
				tmp2.second = tmp.second;
				frontier.push(tmp2);
			}
			if (tmp.first < (width - 1) && imgChamfer(tmp.first + 1, tmp.second) == g && imgLabel(tmp.first + 1, tmp.second) == -1) {
				imgLabel(tmp.first + 1, tmp.second) = imgLabel(tmp.first, tmp.second);
				tmp2.first = tmp.first + 1;
				tmp2.second = tmp.second;
				frontier.push(tmp2);
			}
			if (tmp.second > 0 && imgChamfer(tmp.first, tmp.second - 1) == g && imgLabel(tmp.first, tmp.second - 1) == -1) {
				imgLabel(tmp.first, tmp.second - 1) = imgLabel(tmp.first, tmp.second);
				tmp2.first = tmp.first;
				tmp2.second = tmp.second - 1;
				frontier.push(tmp2);
			}
			if (tmp.second < (height - 1) && imgChamfer(tmp.first, tmp.second + 1) == g && imgLabel(tmp.first, tmp.second + 1) == -1) {
				imgLabel(tmp.first, tmp.second + 1) = imgLabel(tmp.first, tmp.second);
				tmp2.first = tmp.first;
				tmp2.second = tmp.second + 1;
				frontier.push(tmp2);
			}

		}

		std::vector<pair<int, int> >::const_iterator ptr2 = vec[g].begin();
		while (ptr2 != vec[g].end()) {
			if (imgLabel(ptr2->first, ptr2->second) == -1) {
				floodfill(imgChamfer, imgLabel, ptr2->first, ptr2->second, globalLabel);
				++globalLabel;
			}
			++ptr2;
		}
	}
}

/* Watershed algorithm*/
void waterShedWithMarkers(ImgInt& imgLabel, const ImgGray& imgGradient, const ImgBinary& imgMarker, int listSize, std::vector< std::vector<pair<int, int>> >& vec) {
	int globalLabel = 0;
	int width = imgLabel.Width();
	int height = imgLabel.Height();
	std::queue<pair<int, int> > frontier;
	pair<int, int> tmp;
	
	for (int g = 0; g < listSize + 1; ++g) {
		std::vector<pair<int, int> >::const_iterator ptr2 = vec[g].begin();
		while (ptr2 != vec[g].end()) {
			if (imgMarker(ptr2->first, ptr2->second) == 1) {
				floodfillMarker(imgMarker, imgLabel, ptr2->first, ptr2->second, globalLabel);
				++globalLabel;
			}
			++ptr2;
		}
	}
	
	for (int g = 0; g < listSize + 1; ++g) {		
		std::vector<pair<int, int> >::const_iterator ptr = vec[g].begin();
		while (ptr != vec[g].end()) {
			if (ptr->first > 0 && imgLabel(ptr->first - 1, ptr->second) >= 0) {
				imgLabel(ptr->first, ptr->second) = imgLabel(ptr->first - 1, ptr->second);
				frontier.push((*ptr));
			}
			if (ptr->first < (width - 1) && imgLabel(ptr->first + 1, ptr->second) >= 0) {
				imgLabel(ptr->first, ptr->second) = imgLabel(ptr->first + 1, ptr->second);
				frontier.push((*ptr));
			}
			if (ptr->second > 0 && imgLabel(ptr->first, ptr->second - 1) >= 0) {
				imgLabel(ptr->first, ptr->second) = imgLabel(ptr->first, ptr->second - 1);
				frontier.push((*ptr));
			}
			if (ptr->second < (height - 1) && imgLabel(ptr->first, ptr->second + 1) >= 0) {
				imgLabel(ptr->first, ptr->second) = imgLabel(ptr->first, ptr->second + 1);
				frontier.push((*ptr));
			}
			++ptr;
		}

		while (!frontier.empty()) {
			tmp = frontier.front();
			frontier.pop();
			std::pair<int, int> tmp2;
			if (tmp.first > 0 && imgGradient(tmp.first - 1, tmp.second) <= g && imgLabel(tmp.first - 1, tmp.second) == -1) {
				imgLabel(tmp.first - 1, tmp.second) = imgLabel(tmp.first, tmp.second);
				tmp2.first = tmp.first - 1;
				tmp2.second = tmp.second;
				frontier.push(tmp2);
			}
			if (tmp.first < (width - 1) && imgGradient(tmp.first + 1, tmp.second) <= g && imgLabel(tmp.first + 1, tmp.second) == -1) {
				imgLabel(tmp.first + 1, tmp.second) = imgLabel(tmp.first, tmp.second);
				tmp2.first = tmp.first + 1;
				tmp2.second = tmp.second;
				frontier.push(tmp2);
			}
			if (tmp.second > 0 && imgGradient(tmp.first, tmp.second - 1) <= g && imgLabel(tmp.first, tmp.second - 1) == -1) {
				imgLabel(tmp.first, tmp.second - 1) = imgLabel(tmp.first, tmp.second);
				tmp2.first = tmp.first;
				tmp2.second = tmp.second - 1;
				frontier.push(tmp2);
			}
			if (tmp.second < (height - 1) && imgGradient(tmp.first, tmp.second + 1) <= g && imgLabel(tmp.first, tmp.second + 1) == -1) {
				imgLabel(tmp.first, tmp.second + 1) = imgLabel(tmp.first, tmp.second);
				tmp2.first = tmp.first;
				tmp2.second = tmp.second + 1;
				frontier.push(tmp2);
			}

		}

		
	}
}


/* Simple algorithm for Edge Detection*/
void edgeDetection(const ImgInt& imgLabel, ImgBinary& imgEdge) {
	for (int y = 1; y < imgLabel.Height() - 1; ++y) {
		for (int x = 1; x < imgLabel.Width() - 1; ++x) {
			if (imgLabel(x - 1, y) != imgLabel(x, y) || imgLabel(x + 1, y) != imgLabel(x, y) ||
				imgLabel(x, y - 1) != imgLabel(x, y) || imgLabel(x, y + 1) != imgLabel(x, y) ||
				imgLabel(x - 1, y - 1) != imgLabel(x, y) || imgLabel(x - 1, y + 1) != imgLabel(x, y) ||
				imgLabel(x + 1, y - 1) != imgLabel(x, y) || imgLabel(x + 1, y + 1) != imgLabel(x, y)) {
				imgEdge(x, y) = 1;
			}
		}
	}
}

/* Logical OR operation on two input images*/
void logicalOr(const ImgBinary& input1, const ImgBinary& input2, ImgBinary& out) {
	for (int y = 0; y < input1.Height() ; ++y) {
		for (int x = 0; x < input1.Width() ; ++x) {
			if (input1(x, y) == 1 || input2(x, y) == 1) {
				out(x, y) = 1;
			}
		}
	}
}

void quantizeImage(ImgGray& out, const ImgFloat& imgMag) {
	float fmax = Max(imgMag);
	float fmin = Min(imgMag);

	int gmax = 255;
	int gmin = 0;
	for (int y = 0; y < imgMag.Height(); ++y) {
		for (int x = 0; x < imgMag.Width(); ++x) {
			out(x, y) = ((imgMag(x, y) - fmin) / (fmax - fmin)*(gmax - gmin)) + gmin;
		}
	}
}

/* Main function starts here */
int main(int argc, const char* argv[], const char* envp[])
{
	// Initialize MFC and return if failure
	HMODULE hModule = ::GetModuleHandle(NULL);
	if (hModule == NULL || !AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
	{
		printf("Fatal Error: MFC initialization failed (hModule = %x)\n", (int)hModule);
		return 1;
	}

	try {
		std::cout << "Suggested threshold value for holes.png is 70" << std::endl;
		std::cout << "Suggested threshold value for cells_small.png is 32" << std::endl;

		// Fetch the command line arguments and throw an error if insufficient arguments provided
		if (argc < 3) {
			cout << "Too few arguments\nUsage: watershed.exe <filename> <threshold>\n" << endl;
			exit(0);
		}

		float threshold = atof(argv[2]);
		ImgGray img1;
		Figure fig1(L"Original Image");

		string parentPath = "../../images/";
		string filename1 = parentPath + argv[1];

		string filename = argv[1];
		string extension;
		size_t pos = filename.find('.');
		if (pos != -1) {
			extension = filename.erase(0, pos + 1);
		}
		else {
			BLEPO_ERROR("Filename must have an extension\n");
		}

		Load(filename1.c_str(), &img1);					// Load the input image
		fig1.Draw(img1);								// Display the input image in the figure window

		int width = img1.Width();
		int height = img1.Height();

		/* Gradient Image Calculation*/
		
		int sigma = 1;
		/*Compute the Guassian Kernels*/
		int mu = round(2.5*sigma - 0.5);
		int w = 2 * mu + 1;

		/*Compute Gaussian Convolution Kernel*/
		float gaussianKernel[100];
		computeGaussianKernel(gaussianKernel, sigma);

		/*Compute Gaussian Derivative Kernel*/
		float gaussianDerivativeKernel[100];
		computeGaussianDerivativeKernel(gaussianDerivativeKernel, sigma);

		/* Compute the X and Y Gradients */
		ImgFloat Gx, Gy;
		Gx.Reset(img1.Width(), img1.Height());
		Set(&Gx, 0);
		Gy.Reset(img1.Width(), img1.Height());
		Set(&Gy, 0);

		convolveSeparable(img1, w, mu, gaussianDerivativeKernel, gaussianKernel, Gx);
		convolveSeparable(img1, w, mu, gaussianKernel, gaussianDerivativeKernel, Gy);

		/* Compute the Gradient Magnitudes */
		ImgFloat imgMag;
		imgMag.Reset(img1.Width(), img1.Height());
		for (int y = 0; y < img1.Height(); ++y) {
			for (int x = 0; x < img1.Width(); ++x) {
				imgMag(x, y) = max(abs(Gx(x, y)), abs(Gy(x, y)));
			}
		}


		/* End of Gradient Image Calculation*/

		/* Thresholding*/
		ImgBinary imgThreshold;
		imgThreshold.Reset(width, height);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				imgThreshold(x, y) = img1(x, y) > threshold ? 0 : 1;
			}
		}

		/* Compute the Chamfer Distance */
		ImgInt imgChamfer;
		imgChamfer.Reset(width, height);
		computeChamferDistance(imgThreshold, imgChamfer);

		Figure figThreshold(L"Thresholded Image");
		figThreshold.Draw(imgThreshold);
		Figure figChamfer(L"Chamfer Distance Image");
		figChamfer.Draw(imgChamfer);

		// Find the maximum value for gray level. Avoids unnecessary iteration if the max is < 255. Reduces only the Space Complexity.
		int max = Max(imgChamfer);
	
		//List of pixels
		std::vector< std::vector<pair<int, int>> > vec(max+1);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				pair<int, int> p;
				p.first = x;
				p.second = y;
				vec[imgChamfer(x, y)].push_back(p);
			}
		}
		
		ImgInt imgLabel;
		imgLabel.Reset(width, height);
		Set(&imgLabel, -1);

		waterShed(imgLabel, imgChamfer, max, vec);

		Figure figLabel(L"Non-Marker-based Watershed Image");
		figLabel.Draw(imgLabel);

		ImgBinary imgEdge;
		imgEdge.Reset(width, height);
		Set(&imgEdge, 0);

		edgeDetection(imgLabel, imgEdge);
		
		Figure figEdge(L"Edge Image");
		figEdge.Draw(imgEdge);

		//Logical OR
		ImgBinary imgOr;
		imgOr.Reset(width, height);
		Set(&imgOr, 0);

		logicalOr(imgThreshold, imgEdge, imgOr);
		
		
		Figure figOr(L"Marker Image");
		figOr.Draw(imgOr);

		Figure figMag(L"Gradient Magnitude Image");
		figMag.Draw(imgMag);

		// Quantize the Gradient Magnitude Image
		ImgGray imgQuantized;
		imgQuantized.Reset(width, height);
		Set(&imgQuantized, 0);
		quantizeImage(imgQuantized, imgMag);
		
		Figure figQuantized(L"Quantized Gradient Magnitude Image");
		figQuantized.Draw(imgQuantized);

		// Find the maximum value for gray level. Avoids unnecessary iteration if the max is < 255. Reduces only the Space Complexity.
		int max_marker = Max(imgQuantized);

		// List of pixels
		std::vector< std::vector<pair<int, int>> > vec_marker(max_marker + 1);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				pair<int, int> p;
				p.first = x;
				p.second = y;
				vec_marker[imgQuantized(x, y)].push_back(p);
			}
		}

		// WaterShed using Marker Image
		ImgInt imgMarkedLabel;
		imgMarkedLabel.Reset(width, height);
		Set(&imgMarkedLabel, -1);
		waterShedWithMarkers(imgMarkedLabel, imgQuantized, imgOr, max_marker, vec_marker);

		ImgBinary imgMarkedEdge;
		imgMarkedEdge.Reset(width, height);
		Set(&imgMarkedEdge, 0);
		edgeDetection(imgMarkedLabel, imgMarkedEdge);
	    
		/*Figure figLabelMarker(L"Label Image Using Markers");
		figLabelMarker.Draw(imgMarkedLabel);
		Figure figEdgeMarker(L"Edge Image Using Markers");
		figEdgeMarker.Draw(imgMarkedEdge);*/

		/* Color the edges in the input image */

		// Copy the input image onto the final image
		ImgBgr imgFinal;
		imgFinal.Reset(width, height);
		Set(&imgFinal, Bgr(0, 0, 0));
		ImgGray::ConstIterator ptr = img1.Begin();
		ImgBgr::Iterator ptr2 = imgFinal.Begin();
		while (ptr != img1.End() || ptr2  != imgFinal.End() ) {
			ptr2->b = *ptr;
			ptr2->g = *ptr;
			ptr2->r = *ptr;
			++ptr;
			++ptr2;
		}

		// Color the edges green
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				if (imgMarkedEdge(x, y) == 1) {
					imgFinal(x, y).b = 0;
					imgFinal(x, y).g = 255;
					imgFinal(x, y).r = 0;
				}
			}
		}

		Figure figFinal(L"Final Marker-based WaterShed Image");
		figFinal.Draw(imgFinal);
	}
	catch (const Exception& e) {
		e.Display();
		exit(0);
	}

	// Loop forever until user presses Ctrl-C in terminal window.
	EventLoop();

	return 0;
}
