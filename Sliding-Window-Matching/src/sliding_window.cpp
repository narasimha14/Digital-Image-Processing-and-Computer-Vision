// sliding_window.cpp : Defines the entry point for the console application.
//

#include <afxwin.h>  // necessary for MFC to work properly
#include "sliding_window.h"
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

		// Fetch the command line arguments and throw an error if insufficient arguments provided
		if (argc < 4) {
			cout << "Too few arguments\nUsage: sliding_window.exe <left filename> <right filename> <max disparity>\n" << endl;
			exit(0);
		}

		
		ImgBgr imgLeft, imgRight;
		Figure fig1(L"Original Left Image"), fig2(L"Original Right Image");

		string parentPath = "../../images/";
		string filename1 = parentPath + argv[1];
		string filename2 = parentPath + argv[2];
		int dmax = atoi(argv[3]);

		/*string filename = argv[1];
		string extension;
		size_t pos = filename.find('.');
		if (pos != -1) {
			extension = filename.erase(0, pos + 1);
		}
		else {
			BLEPO_ERROR("Filename must have an extension\n");
		}*/

		Load(filename1.c_str(), &imgLeft);					// Load the input image
		fig1.Draw(imgLeft);								// Display the input image in the figure window
		Load(filename2.c_str(), &imgRight);
		fig2.Draw(imgRight);

		if (imgLeft.Width() != imgRight.Width() || imgLeft.Height() != imgRight.Height()) {
			BLEPO_ERROR("Two files must be of the same size\n");
		}

		/* Convert to gray scale*/
		ImgGray imgLeftGray, imgRightGray;
		Convert(imgLeft, &imgLeftGray);
		Convert(imgRight, &imgRightGray);

		Figure figLeftGray(L"Gray scale Left Image"), figRightGray(L"Gray scale Right Image");
		figLeftGray.Draw(imgLeftGray);
		figRightGray.Draw(imgRightGray);

		int width = imgLeft.Width();
		int height = imgLeft.Height();
		//Compute Dbar
		ImgInt kernel = ImgInt(5, 1);
		Set(&kernel, 1);
		ImgInt kernelTranspose = ImgInt(1, 5);
		Set(&kernelTranspose, 1);
		std::vector<ImgInt> dBar(dmax);
		for (int d = 0; d < dmax; ++d) {
			dBar[d].Reset(width, height);
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					
					int tmp = (x - d) < 0 ? 0 : x - d;
					
					dBar[d](x, y) = blepo_ex::Abs <int> ( (int) imgLeftGray(x, y) - (int)imgRightGray( tmp, y) ); 
				}
			}
			ImgInt tmp;
			Convolve(dBar[d], kernel, &tmp);
			Convolve(tmp, kernelTranspose, &dBar[d]);
		}

		//Compute Disparity Map with left-right consistency check
		ImgInt d_left;
		d_left.Reset(width, height);
		Set(&d_left, 0);
		ImgInt disp_map;
		int depth_count = 0;
		disp_map.Reset(width, height);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int d_right;
				int leftMin, rightMin;
				d_right = 0;
				leftMin = rightMin = dBar[0](x, y);
				for (int d = 0; d < dmax; ++d) {
					if (dBar[d](x, y) < leftMin) {
						leftMin = dBar[d](x, y);
						d_left(x,y) = d;
					}
				}

				for (int d = 0; d < dmax; ++d) {
					// Not adding the d since the output was worse
					//int tmp = (x - d_left(x, y) + d) < 0 ? 0 : (x - d_left(x, y) + d);
					int tmp = (x - d_left(x,y) ) < 0 ? 0 : (x - d_left(x,y));
					if (dBar[d]( tmp , y) < rightMin) {
						rightMin = dBar[d](tmp, y);
						d_right = d;
					}
				}

				if (d_left(x,y) == d_right && d_left(x,y) != 0) {
					disp_map(x, y) = d_left(x,y);
					++depth_count;
				}
				else {
					disp_map(x,y) = 0;
				}
			}
		}

		Figure figDispWithout(L"Disparity Map without consistency check");
		figDispWithout.Draw(d_left);
		Figure out(L"Disparity Map with left-right consistency check");
		out.Draw(disp_map);

		// Output to ply file
		FILE * fptr;
		fptr = fopen("../../images/output.ply", "wt");
		fprintf(fptr, "ply\nformat ascii 0.1\nelement vertex %d\nproperty float x\nproperty float y\nproperty float z\nproperty uchar diffuse_red\nproperty uchar diffuse_green\nproperty uchar diffuse_blue\nend_header\n", depth_count);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				float z = disp_map(x, y) * 5;
				if (z != 0) {
					fprintf(fptr, "%d %d %f %d %d %d\n", x, -y, z , imgLeft(x, y).r, imgLeft(x, y).g, imgLeft(x, y).b);
				}
			}
		}
		fclose(fptr);
	}
	catch (const Exception& e) {
		e.Display();
		exit(0);
	}

	// Loop forever until user presses Ctrl-C in terminal window.
	EventLoop();

	return 0;
}


