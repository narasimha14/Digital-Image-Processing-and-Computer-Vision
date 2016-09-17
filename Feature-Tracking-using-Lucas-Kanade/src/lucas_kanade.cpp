// lucas_kanade.cpp : Defines the entry point for the console application.
//

#include <afxwin.h>  // necessary for MFC to work properly
#include "lucas_kanade.h"
#include <algorithm>
#include <cstring>
#include <atlstr.h>
#include "../../src/blepo.h"
#include <stack>
#include <queue>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PI 3.1416
#define SQR(a) ((a)*(a))

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


double InterpolateBilinear(ImgGray& frame, double x, double y)
{
	double x0, y0, ax, ay;
	x0 = floor(x);
	y0 = floor(y);
	ax = x - x0;
	ay = y - y0;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x0 >= frame.Width() - 1) x0 = frame.Width() - 2;
	if (y0 >= frame.Height() - 1) y0 = frame.Height() - 2;

	return ((1 - ax)*(1 - ay)*frame(x0, y0) + ax*(1 - ay)*frame(x0 + 1, y0) + (1 - ax)*ay*frame(x0, y0 + 1) + ax*ay*frame(x0 + 1, y0 + 1));
}

double InterpolateBilinear(ImgFloat& frame, double x, double y)
{
	double x0, y0, ax, ay;
	x0 = floor(x);
	y0 = floor(y);
	ax = x - x0;
	ay = y - y0;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x0 >= frame.Width() - 1) x0 = frame.Width() - 2;
	if (y0 >= frame.Height() - 1) y0 = frame.Height() - 2;

	return ((1 - ax)*(1 - ay)*frame(x0, y0) + ax*(1 - ay)*frame(x0 + 1, y0) + (1 - ax)*ay*frame(x0, y0 + 1) + ax*ay*frame(x0 + 1, y0 + 1));
}

void Compute2x2GradMat(int x, int y, double z[], ImgFloat& grad_x, ImgFloat& grad_y, int size)
{
	for (int j = -(size - 1) / 2; j < (size + 1) / 2; j++)
	{
		for (int i = -(size - 1) / 2; i < (size + 1) / 2; i++)
		{
			z[0] = z[0] + InterpolateBilinear(grad_x, x + i, y + j) * InterpolateBilinear(grad_x, x + i, y + j);	//gxx
			z[1] = z[1] + InterpolateBilinear(grad_x, x + i, y + j) * InterpolateBilinear(grad_y, x + i, y + j);	//gxy
			z[2] = z[2] + InterpolateBilinear(grad_y, x + i, y + j) * InterpolateBilinear(grad_y, x + i, y + j);	//gyy
		}
	}
}

void ComputeZGradMat(int x, int y, double z[], ImgFloat& grad_x, ImgFloat& grad_y)
{
	for (int j = -1; j < 2; j++)
	{
		for (int i = -1; i < 2; i++)
		{
			z[0] = z[0] + grad_x(x + i, y + j) * grad_x(x + i, y + j);	//gxx
			z[1] = z[1] + grad_x(x + i, y + j) * grad_y(x + i, y + j);	//gxy
			z[2] = z[2] + grad_y(x + i, y + j) * grad_y(x + i, y + j);	//gyy
		}
	}
}

void Compute2x1ErrorVector(double x, double y, double e[], double u[], ImgFloat& grad_x, ImgFloat& grad_y, ImgGray& currentframe, ImgGray& nextframe, int size)
{
	for (int j = -(size - 1) / 2; j < (size + 1) / 2; j++)
	{
		for (int i = -(size - 1) / 2; i < (size + 1) / 2; i++)
		{
			e[0] = e[0] + InterpolateBilinear(grad_x, x + i, y + j)*(InterpolateBilinear(currentframe, x + i, y + j) - InterpolateBilinear(nextframe, x + i + u[0], y + j + u[1]));
			e[1] = e[1] + InterpolateBilinear(grad_y, x + i, y + j)*(InterpolateBilinear(currentframe, x + i, y + j) - InterpolateBilinear(nextframe, x + i + u[0], y + j + u[1]));
		}
	}
}

void DrawSquare(int x, int y, ImgBgr& input)
{
	if ((x >0) && (y>0) && (x < input.Width() - 1) && (y < input.Height() - 1))
	{
		for (int j = -1; j < 2; j++)
		{
			for (int i = -1; i < 2; i++)
			{
				if ((x + i == x) && (y + j == y))
					continue;
				else
					input(x + i, y + j) = Bgr(0, 0, 255);
			}
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
		if (argc < 6) {
			cout << "Too few arguments\nUsage: lucas_kanade.exe <left filename> <right filename> <max disparity>\n" << endl;
			exit(0);
		}

		
		
		CString parentPath = "../../images/";
		CString filename;

		CString format = argv[1];
		int firstFrame = atoi(argv[2]);
		int lastFrame = atoi(argv[3]);
		float sigma = atof(argv[4]);
		int winSize = atoi(argv[5]);
		filename.Format(format, firstFrame);

		filename = parentPath + filename;

		/*string filename = argv[1];
		string extension;
		size_t pos = filename.find('.');
		if (pos != -1) {
			extension = filename.erase(0, pos + 1);
		}
		else {
			BLEPO_ERROR("Filename must have an extension\n");
		}*/

		
		ImgGray firstImg;
		Figure figFirst(L"First Image");
		Load(filename, &firstImg);					// Load the input image
		figFirst.Draw(firstImg);					// Display the input image in the figure window
		
		int width = firstImg.Width();
		int height = firstImg.Height();

		// Calculate Gradient
		/*Compute the Guassian Kernels*/
		float tmpSigma = 0.8;
		int mu = round(2.5*tmpSigma - 0.5);
		int w = 2 * mu + 1;

		/*Compute Gaussian Convolution Kernel*/
		float gaussianKernel[100];
		computeGaussianKernel(gaussianKernel, tmpSigma);
		/*std::cout << "Gaussian Convolution Kernel is " << std::endl;
		for (int i = 0; i < w; ++i) {
			std::cout << gaussianKernel[i] << std::endl;
		}*/

		/*Compute Gaussian Derivative Kernel*/
		float gaussianDerivativeKernel[100];
		computeGaussianDerivativeKernel(gaussianDerivativeKernel, tmpSigma);
		/*std::cout << "Gaussian Derivative Kernel is " << std::endl;
		for (int i = 0; i < w; ++i) {
			std::cout << gaussianDerivativeKernel[i] << std::endl;
		}*/

		/* Compute the X and Y Gradients */
		ImgFloat Gx, Gy;
		Gx.Reset(width, height);
		Set(&Gx, 0);
		Gy.Reset(width, height);
		Set(&Gy, 0);

		convolveSeparable(firstImg, w, mu, gaussianDerivativeKernel, gaussianKernel, Gx);
		convolveSeparable(firstImg, w, mu, gaussianKernel, gaussianDerivativeKernel, Gy);

		ImgFloat corner;
		corner.Reset(width, height);
		Set(&corner, 0);

		int threshold = 2000;
		if (firstFrame >= 30) threshold = 4000; //for flowergarden
		if (firstFrame >= 588) threshold = 2000; // for state_seq
		std::cout << "Threshold : " << threshold << std::endl;

		//find cornerness
		for (int y = 1; y < height - 1; y++)
		{
			for (int x = 1; x < width - 1; x++)
			{
				double z[3] = { 0, 0, 0 };
				double lambda1 = 0;
				double lambda2 = 0;
				double cornerness = 0;

				ComputeZGradMat(x, y, z, Gx, Gy);
				lambda1 = 0.5*((z[0] + z[2]) + sqrt(SQR(z[0] - z[2]) + 4 * z[1] * z[1]));
				lambda2 = 0.5*((z[0] + z[2]) - sqrt(SQR(z[0] - z[2]) + 4 * z[1] * z[1]));
				cornerness = min(lambda1, lambda2);
				corner(x, y) = ((lambda2 > threshold) ? lambda2 : 0);
			}
		}

		int featurecount = 0;
		ImgBgr goodfeature;
		Convert(firstImg, &goodfeature);

		for (int y = winSize / 2; y < height - (winSize / 2); y++)
		{
			for (int x = winSize / 2; x < width - (winSize / 2); x++)
			{
				if (corner(x, y) < corner(x - 1, y) || corner(x, y) < corner(x + 1, y) ||
					corner(x, y) < corner(x, y - 1) || corner(x, y) < corner(x, y + 1))
				{
					corner(x, y) = 0;
				}
				if (corner(x, y) > threshold)
				{
					DrawSquare(x, y, goodfeature);
					featurecount++;
				}
			}
		}

		//store coordinates of good feature points
		std::vector<double> feature_x(featurecount + 1);
		std::vector<double> feature_y(featurecount + 1);

		int count = 0;
		for (int y = winSize / 2; y < firstImg.Height() - (winSize / 2); y++)
		{
			for (int x = winSize / 2; x < firstImg.Width() - (winSize / 2); x++)
			{
				if (corner(x, y) > threshold)
				{
					feature_x[count] = x;
					feature_y[count] = y;
					count++;
				}
			}
		}

		/*for (int i = 0; i < featurecount; i++)
		{
			std::cout << "featureX[" << i << "] = " << feature_x[i] << "featureY[" << i << "] = " << feature_y[i] << std::endl;
		}*/

		Figure figGoodFeature(L"Image with Good Feature Points");
		figGoodFeature.Draw(goodfeature);

		/* Lucas Kanade Algorithm */
		/*Compute the Guassian Kernels*/
		mu = round(2.5*sigma - 0.5);
		w = 2 * mu + 1;

		/*Compute Gaussian Convolution Kernel*/
		float gaussKernel[100];
		computeGaussianKernel(gaussKernel, sigma);

		/*Compute Gaussian Derivative Kernel*/
		float gaussDerivativeKernel[100];
		computeGaussianDerivativeKernel(gaussDerivativeKernel, sigma);
		

		/* Compute the X and Y Gradients */
		ImgFloat Gradx, Grady;
		Gradx.Reset(width, height);
		Set(&Gradx, 0);
		Grady.Reset(width, height);
		Set(&Grady, 0);

		ImgGray currentImg, nextImg;
		currentImg.Reset(width, height);
		nextImg.Reset(width, height);
		Set(&currentImg, 0);
		Set(&nextImg, 0);

		CString file;
		ImgBgr imgFinalFeature;
		imgFinalFeature.Reset(width, height);
		Figure figFinalFeatureTracking;
		figFinalFeatureTracking.SetTitle("Final image with Feature Tracking");

		//for each pair of image
		for (int k = firstFrame; k < lastFrame; k++)
		{
			//Take first frame
			file.Format((LPCWSTR) format, k);
			file = parentPath + file;
			Load(file, &currentImg);

			//Take second frame
			file.Format((LPCWSTR) format, k + 1);
			file = parentPath + file;
			Load(file, &nextImg);

			//Compute gradient of first frame
			convolveSeparable(currentImg, w, mu, gaussDerivativeKernel, gaussKernel, Gradx);
			convolveSeparable(currentImg, w, mu, gaussKernel, gaussDerivativeKernel, Grady);

			for (int i = 0; i < featurecount; i++)
			{
				//LK algorithm
				//initialize Z, U vectors
				double zmat[3] = { 0, 0, 0 };
				double umat[2] = { 0, 0 };
				double emat[2] = { 0, 0 };
				int iteration = 0;

				if ((feature_x[i] < 1) || (feature_y[i] < 1))
					continue;
				else
				{
					Compute2x2GradMat(feature_x[i], feature_y[i], zmat, Gradx, Grady, winSize);
				}

				while (iteration < 1)
				{
					double det = 0;
					double u_delta = 0;
					double v_delta = 0;

					Compute2x1ErrorVector(feature_x[i], feature_y[i], emat, umat, Gradx, Grady, currentImg, nextImg, winSize);

					det = zmat[0] * zmat[2] - SQR(zmat[1]);
					if (det == 0) det = 1;

					u_delta = ((zmat[2] * emat[0]) - (zmat[1] * emat[1])) / det;
					v_delta = ((zmat[0] * emat[1]) - (zmat[1] * emat[0])) / det;

					umat[0] += u_delta;
					umat[1] += v_delta;

					iteration++;
				}
				feature_x[i] += umat[0];
				feature_y[i] += umat[1];
			}

			Convert(nextImg, &imgFinalFeature);
			for (int i = 0; i < featurecount; i++)
			{
				DrawSquare(round(feature_x[i]), round(feature_y[i]), imgFinalFeature);
			}
			figFinalFeatureTracking.Draw(imgFinalFeature);

		}

	}
	catch (const Exception& e) {
		e.Display();
		exit(0);
	}

	// Loop forever until user presses Ctrl-C in terminal window.
	EventLoop();

	return 0;
}


