// canny_edge.cpp : Defines the entry point for the console application.
//

#include <afxwin.h>  // necessary for MFC to work properly
#include "canny_edge.h"
#include <algorithm>
#include "../../src/blepo.h"
#include <stack>

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
	//Convolve Horizontal
	for (int i = 0; i < img.Height(); ++i) {
		for (int j = halfwidth; j < img.Width() - halfwidth; ++j) {
			float val = 0;
			for (int k = 0; k < width; ++k) {
				val += (hKernel[k] * img(j + halfwidth - k, i));
			}
			tmp(j, i) = val;
		}
	}
	//Convolve Vertical
	for (int i = halfwidth; i < img.Height() - halfwidth; ++i) {
		for (int j = 0; j < img.Width(); ++j) {
			float val = 0;
			for (int k = 0; k < width; ++k) {
				val += (vKernel[k] * tmp(j, i + halfwidth - k));
			}
			out(j, i) = val;
		}
	}
}

/* Compute Non-Maximum Suppression */
void computeNonMaximumSuppression(ImgFloat magImg, ImgFloat phaseImg, int halfwidth, ImgFloat& suppressedImg) {
	for (int y = halfwidth; y < magImg.Height() - halfwidth; ++y) {
		for (int x = halfwidth; x < magImg.Width() - halfwidth; ++x) {

			if (phaseImg(x, y) >= (-PI / 8) && phaseImg(x, y) < (PI / 8)) {
				if (magImg(x - 1, y) > magImg(x, y) || magImg(x + 1, y) > magImg(x, y))
					suppressedImg(x, y) = 0;
				else
					suppressedImg(x, y) = magImg(x, y);
			}
			else if (phaseImg(x, y) >= (PI / 8) && phaseImg(x, y) < (3 * PI / 8)) {
				if (magImg(x - 1, y - 1) > magImg(x, y) || magImg(x + 1, y + 1) > magImg(x, y))
					suppressedImg(x, y) = 0;
				else
					suppressedImg(x, y) = magImg(x, y);
			}
			else if (phaseImg(x, y) >= (3 * PI / 8) && phaseImg(x, y) < (5 * PI / 8)) {
				if (magImg(x, y - 1) > magImg(x, y) || magImg(x, y + 1) > magImg(x, y))
					suppressedImg(x, y) = 0;
				else
					suppressedImg(x, y) = magImg(x, y);
			}
			else if (phaseImg(x, y) >= (5 * PI / 8) && phaseImg(x, y) < (7 * PI / 8)) {
				if (magImg(x + 1, y - 1) > magImg(x, y) || magImg(x - 1, y + 1) > magImg(x, y))
					suppressedImg(x, y) = 0;
				else
					suppressedImg(x, y) = magImg(x, y);
			}
		}
	}
}

/* Floodfill required for Edge Linking with Hysteresis (Double Thresholding) */
void floodfill(ImgBinary& img, ImgFloat suppressedImg, int x, int y, float low_threshold) {
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
				if (img(x1 + j, y1 + i) == 0 && suppressedImg(x1 + j, y1 + i) >= low_threshold)
				{
					x_cor.push(x1 + j);
					y_cor.push(y1 + i);
					img(x1 + j, y1 + i) = 1;
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

/*Compute Inverse Probability Map*/
void computeInverseProbabilityMap(ImgInt chamferImg, ImgGray templateImg, ImgBinary hystImg, ImgInt& probabilityMapImg) {
	for (int y = 0; y < chamferImg.Height() - templateImg.Height(); ++y) {
		for (int x = 0; x < chamferImg.Width() - templateImg.Width(); ++x) {
			float sum = 0;
			for (int j = 0; j < templateImg.Height(); ++j) {
				for (int i = 0; i < templateImg.Width(); ++i) {
					if (hystImg(i, j) == 1)
						sum += chamferImg(x + i, y + j);
				}
			}
			probabilityMapImg(x, y) = sum;
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
		if (argc < 3) {
			cout << "Too few arguments\nUsage: canny_edge.exe <sigma> <filename> <template>\n" << endl;
			exit(0);
		}
		else if (argc > 4) {
			cout << "Too many arguments" << endl;
			cout << "Usage: canny_edge.exe <sigma> <filename> <template>" << endl;
			exit(0);
		}

		float sigma = atof(argv[1]);
		ImgGray img1;
		Figure fig1(L"Original Image");

		string parentPath = "../../images/";
		string filename1 = parentPath + argv[2];

		string filename = argv[2];
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

														/*Compute the Guassian Kernels*/
		int mu = round(2.5*sigma - 0.5);
		int w = 2 * mu + 1;

		/*Compute Gaussian Convolution Kernel*/
		float gaussianKernel[100];
		computeGaussianKernel(gaussianKernel, sigma);
		std::cout << "Gaussian Convolution Kernel is " << std::endl;
		for (int i = 0; i < w; ++i) {
			std::cout << gaussianKernel[i] << std::endl;
		}

		/*Compute Gaussian Derivative Kernel*/
		float gaussianDerivativeKernel[100];
		computeGaussianDerivativeKernel(gaussianDerivativeKernel, sigma);
		std::cout << "Gaussian Derivative Kernel is " << std::endl;
		for (int i = 0; i < w; ++i) {
			std::cout << gaussianDerivativeKernel[i] << std::endl;
		}

		/* Compute the X and Y Gradients */
		ImgFloat Gx, Gy;
		Gx.Reset(img1.Width(), img1.Height());
		Set(&Gx, 0);
		Gy.Reset(img1.Width(), img1.Height());
		Set(&Gy, 0);

		convolveSeparable(img1, w, mu, gaussianDerivativeKernel, gaussianKernel, Gx);
		convolveSeparable(img1, w, mu, gaussianKernel, gaussianDerivativeKernel, Gy);

		/* Compute the Gradient Magnitudes and Phase */
		ImgFloat imgMag, imgPhase;
		imgMag.Reset(img1.Width(), img1.Height());
		imgPhase.Reset(img1.Width(), img1.Height());
		for (int y = 0; y < img1.Height(); ++y) {
			for (int x = 0; x < img1.Width(); ++x) {
				imgMag(x, y) = max(abs(Gx(x, y)), abs(Gy(x, y)));
				imgPhase(x, y) = atan2(Gy(x, y), Gx(x, y));
				while (imgPhase(x, y) >= 7 * PI / 8) imgPhase(x, y) -= PI;
				while (imgPhase(x, y) < -PI / 8) imgPhase(x, y) += PI;
			}
		}

		/* Compute Non-Maximum Suppression*/
		ImgFloat imgSuppressed;
		imgSuppressed.Reset(img1.Width(), img1.Height());
		Set(&imgSuppressed, 0);

		computeNonMaximumSuppression(imgMag, imgPhase, mu, imgSuppressed);

		Figure fig2(L"X Gradient Image"), fig3(L"Y Gradient Image"), fig4(L"Gradient Magnitude Image"),
			fig5(L"Gradient Phase Image"), fig6(L"Non-Maximum Suppression Image");
		fig2.Draw(Gx);
		fig3.Draw(Gy);
		fig4.Draw(imgMag);
		fig5.Draw(imgPhase);
		fig6.Draw(imgSuppressed);

		/* Edge Linking with hysteresis */
		// Finding the threshold values
		std::vector<float> tmpVec;
		tmpVec.reserve(imgSuppressed.Height()*imgSuppressed.Width());
		int i = -1;
		for (int y = 0; y < imgSuppressed.Height(); ++y) {
			for (int x = 0; x < imgSuppressed.Width(); ++x) {
				tmpVec.push_back(imgSuppressed(x, y));
			}
		}

		std::sort(tmpVec.begin(), tmpVec.end());
		int idx = 9.5 * imgMag.Width() * imgMag.Height() / 10;
		float high_threshold = tmpVec[idx];
		float low_threshold = high_threshold / 5;

		std::cout << "high threshold : " << high_threshold << " low threshold : " << low_threshold << std::endl;
		// Floodfill with the threshold values
		ImgBinary imgHyst;
		imgHyst.Reset(imgSuppressed.Width(), imgSuppressed.Height());
		Set(&imgHyst, 0);

		for (int y = 0; y < imgSuppressed.Height(); ++y) {
			for (int x = 0; x < imgSuppressed.Width(); ++x) {
				if (imgSuppressed(x, y) >= high_threshold && imgHyst(x, y) != 1) {
					imgHyst(x, y) = 1;
					floodfill(imgHyst, imgSuppressed, x, y, low_threshold);
				}
			}
		}

		Figure fig7(L"Final Canny Edges Image");
		fig7.Draw(imgHyst);

		/* Compute the Chamfer Distance */
		ImgInt imgChamfer;
		imgChamfer.Reset(imgHyst.Width(), imgHyst.Height());
		computeChamferDistance(imgHyst, imgChamfer);

		Figure fig8(L"Chamfer Distance Image");
		fig8.Draw(imgChamfer);

		/* Template Matching */
		if (argc == 4) {
			string templateImgFile, templateFilename;
			templateImgFile = argv[3];
			templateFilename = parentPath + argv[3];

			ImgGray templateImg;
			Load(templateFilename.c_str(), &templateImg);

			Figure template_fig1(L"Original Template Image");
			template_fig1.Draw(templateImg);

			/* Compute X and Y Gradients */
			ImgFloat template_Gx, template_Gy;
			template_Gx.Reset(templateImg.Width(), templateImg.Height());
			Set(&template_Gx, 0);
			template_Gy.Reset(templateImg.Width(), templateImg.Height());
			Set(&template_Gy, 0);

			convolveSeparable(templateImg, w, mu, gaussianDerivativeKernel, gaussianKernel, template_Gx);
			convolveSeparable(templateImg, w, mu, gaussianKernel, gaussianDerivativeKernel, template_Gy);

			/* Compute the Gradient Magnitudes and Phase */
			ImgFloat template_imgMag, template_imgPhase;
			template_imgMag.Reset(templateImg.Width(), templateImg.Height());
			template_imgPhase.Reset(templateImg.Width(), templateImg.Height());
			for (int y = 0; y < templateImg.Height(); ++y) {
				for (int x = 0; x < templateImg.Width(); ++x) {
					template_imgMag(x, y) = max(abs(template_Gx(x, y)), abs(template_Gy(x, y)));
					template_imgPhase(x, y) = atan2(template_Gy(x, y), template_Gx(x, y));
					while (template_imgPhase(x, y) >= 7 * PI / 8) template_imgPhase(x, y) -= PI;
					while (template_imgPhase(x, y) < -PI / 8) template_imgPhase(x, y) += PI;
				}
			}

			/* Compute Non-Maximum Suppression */
			ImgFloat template_imgSuppressed;
			template_imgSuppressed.Reset(templateImg.Width(), templateImg.Height());
			Set(&template_imgSuppressed, 0);

			computeNonMaximumSuppression(template_imgMag, template_imgPhase, mu, template_imgSuppressed);

			/* Edge Linking with hysteresis */
			//Finding the threshold values
			std::vector<float> template_tmpVec;
			template_tmpVec.reserve(template_imgSuppressed.Height()*template_imgSuppressed.Width());
			int i = -1;
			for (int y = 0; y < template_imgSuppressed.Height(); ++y) {
				for (int x = 0; x < template_imgSuppressed.Width(); ++x) {
					template_tmpVec.push_back(template_imgSuppressed(x, y));
				}
			}

			std::sort(template_tmpVec.begin(), template_tmpVec.end());
			int idx = 9.5 * template_imgMag.Width() * template_imgMag.Height() / 10;
			float template_high_threshold = template_tmpVec[idx];
			float template_low_threshold = template_high_threshold / 5;

			std::cout << "Template Image Threshold Values:\n" << "high threshold : " << template_high_threshold << " low threshold : " << template_low_threshold << std::endl;

			// Floodfill with the threshold values
			ImgBinary template_imgHyst;
			template_imgHyst.Reset(template_imgSuppressed.Width(), template_imgSuppressed.Height());
			Set(&template_imgHyst, 0);

			for (int y = 0; y < template_imgSuppressed.Height(); ++y) {
				for (int x = 0; x < template_imgSuppressed.Width(); ++x) {
					if (template_imgSuppressed(x, y) >= template_high_threshold && template_imgHyst(x, y) != 1) {
						template_imgHyst(x, y) = 1;
						floodfill(template_imgHyst, template_imgSuppressed, x, y, template_low_threshold);
					}
				}
			}

			Figure template_fig2(L"Canny Edges of template image");
			template_fig2.Draw(template_imgHyst);

			/* Compute the Inverse Probability Map */
			ImgInt probMapImg;
			probMapImg.Reset(imgChamfer.Width() - templateImg.Width(), imgChamfer.Height() - templateImg.Height());
			Set(&probMapImg, 255);
			computeInverseProbabilityMap(imgChamfer, templateImg, template_imgHyst, probMapImg);

			/* Get the lowest value and the corresponding x and y co-ordinates from the Inverse Probability Map*/
			float minValue = probMapImg(0, 0);
			int min_x = 0, min_y = 0;
			for (int y = 0; y < probMapImg.Height(); ++y) {
				for (int x = 0; x < probMapImg.Width(); ++x) {
					if (probMapImg(x, y) < minValue) {
						minValue = probMapImg(x, y);
						min_x = x;
						min_y = y;
					}
				}
			}

			Figure probMap(L"Inverse Probability Map of the Template Image");
			probMap.Draw(probMapImg);
			Figure finalImg(L"Final Template Matched Image");
			finalImg.Draw(img1);
			Rect rect(min_x, min_y, min_x + templateImg.Width(), min_y + templateImg.Height());
			finalImg.DrawRect(rect, Bgr::GREEN);
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
