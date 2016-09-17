// thresholding_floodfill.cpp : Defines the entry point for the console application.
//

#include <afxwin.h>  // necessary for MFC to work properly
#include "thresholding_floodfill.h"
#include <algorithm>
#include "../../src/blepo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace blepo;

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
		//Fetch the command line arguments and throw an error if insufficient arguments provided
		if (argc < 2) {
			cout << "Too few arguments\nUsage: thresholding_floodfill.exe <filename1>\n" << endl;
			exit(0);
		}
		else if (argc > 2) {
			cout << "We accept only one file name as input argument" << endl;
			cout << "Usage: thresholding_floodfill.exe <filename1>" << endl;
			exit(0);
		}

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

		if (extension == "jpeg" || extension == "jpg") {
			BLEPO_ERROR("JPEG extension image not supported\n");
		}


		Load(filename1.c_str(), &img1);					// Load the input image
		fig1.Draw(img1);								// Display the input image in the figure window

		// Store the width and height of the image
		const int width = img1.Width();						
		const int height = img1.Height();

		//For high threshold
		Figure figHighThreshold(L"High Threshold Image");
		ImgGray imgHighThreshold;
		const int highThreshold = 200;
		imgHighThreshold.Reset(width, height);

		//For low threshold
		Figure figLowThreshold(L"Low Threshold Image");
		ImgGray imgLowThreshold;
		const int lowThreshold = 100;
		imgLowThreshold.Reset(width, height);
		
		//For Double Threshold
		Figure figDoubleThreshold(L"Double Threshold Image");
		ImgGray imgDoubleThreshold;
		imgDoubleThreshold.Reset(width, height);

		//Determining high and low threshold images
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				imgHighThreshold(x, y) = img1(x, y) > highThreshold ? 255 : 0;
				imgLowThreshold(x, y) = img1(x, y) > lowThreshold ? 255 : 0;
				imgDoubleThreshold(x, y) = 0;
			}
		}

		//Double Threshold image
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				if (img1(x, y) > highThreshold) {
					FloodFill4(imgLowThreshold, x, y, 255, &imgDoubleThreshold);
				}
			}
		}

		// Output the thresholded images onto the Figure Window
		figHighThreshold.Draw(imgHighThreshold);
		figLowThreshold.Draw(imgLowThreshold);
		figDoubleThreshold.Draw(imgDoubleThreshold);

		/* Determining the background & foreground regions */
		/* Connected Components using floodfill*/
		ImgInt labels;
		std::vector< ConnectedComponentProperties<ImgGray::Pixel> > reg;  		

		ConnectedComponents4(imgDoubleThreshold, &labels, &(reg));

		Figure figLabels(L"labels");
		figLabels.Draw(labels);

		

		/* Calculation of Regional Properties*/

		int labelsCount = reg.size();			// Total number of regions
		
		// Vectors for calculating various Region Properties
		std::vector<double> m00(labelsCount), m01(labelsCount), m10(labelsCount), m11(labelsCount), m20(labelsCount), m02(labelsCount),
			mu00(labelsCount), mu01(labelsCount), mu10(labelsCount), mu11(labelsCount), mu20(labelsCount), mu02(labelsCount),
			area(labelsCount), xc(labelsCount), yc(labelsCount), eccentricity(labelsCount), direction(labelsCount),
			major_axis_x(labelsCount), major_axis_y(labelsCount), minor_axis_x(labelsCount), minor_axis_y(labelsCount), compactness(labelsCount);


		std::vector<Point> chain;				// Used to hold the pixels values of the boundaries of a region

		const double PI = 3.141592653589793238463;

		/* Calculation of regular and centralized moments (zeroth, first and second order) */
		for (int i = 0; i < labelsCount; ++i) {
			m00[i] = m10[i] = m11[i] = m01[i] = m20[i] = m02[i] = 0;
		}
		
		/* Compute Moments*/
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				if (labels(x, y) <= labelsCount) {
					m00[labels(x, y) ] += 1;
					m10[labels(x, y) ] += x;
					m01[labels(x, y) ] += y;
					m11[labels(x, y) ] += x * y;
					m20[labels(x, y) ] += x * x;
					m02[labels(x, y) ] += y * y;
				}
			}
		}

		/* Calculation of other properties using moments*/
		for (int i = 0; i < labelsCount; ++i) {
			// Area
			area[i] = m00[i];

			// Centroid
			xc[i] = (m10[i]) / (m00[i]);
			yc[i] = (m01[i]) / (m00[i]);

			// Central Moments
			mu00[i] = m00[i];
			mu10[i] = 0;
			mu01[i] = 0;
			mu11[i] = m11[i] - yc[i] * m10[i];
			mu20[i] = m20[i] - xc[i] * m10[i];
			mu02[i] = m02[i] - yc[i] * m01[i];

			// Direction
			direction[i] = 0.5 * (atan2(2 * mu11[i], (mu20[i] - mu02[i])));

			// Eccentricity, or Elongatedness
			double lambda1, lambda2;
			{
				double a = mu20[i] + mu02[i];
				double b = mu20[i] - mu02[i];
				double c = sqrt(4 * mu11[i] * mu11[i] + b * b);
				lambda1 = (a + c) / (2 * mu00[i]);
				lambda2 = (a - c) / (2 * mu00[i]);
				eccentricity[i] = sqrt((lambda1 - lambda2) / lambda1);

				// Major and Minor Axes
				major_axis_x[i] = sqrt(lambda1) * cos(direction[i]);
				major_axis_y[i] = sqrt(lambda1) * sin(direction[i]);
				minor_axis_x[i] = sqrt(lambda2) * sin(direction[i]);
				minor_axis_y[i] = sqrt(lambda2) * (-cos(direction[i]));

			}

		}

		ImgBgr colorImg;
		Figure colorFig(L"Final Output Image");
		Convert(img1, &colorImg);

		for (unsigned int i = 0; i < reg.size(); ++i) {
			if (reg[i].npixels != 0 && reg[i].value == 255) {
				chain.clear();
				// Using the Wall Follow algorithm to find the boundary pixels and the perimeter of the region
				WallFollow(labels, i, &chain);
				double perimeter = chain.size();
				
				compactness[i] = ( PI * 4 * area[i] ) / ( perimeter*perimeter);

				Point major_x1_y1 = Point((int)(xc[i] + major_axis_x[i]), (int)(yc[i] + major_axis_y[i] ));
				Point major_x2_y2 = Point((int)(xc[i] - major_axis_x[i]), (int)(yc[i] - major_axis_y[i] ));
				Point minor_x1_y1 = Point((int)(xc[i] + minor_axis_x[i]), (int)(yc[i] + minor_axis_y[i] ));
				Point minor_x2_y2 = Point((int)(xc[i] - minor_axis_x[i]), (int)(yc[i] - minor_axis_y[i] ));
				
				/* Display the Region Properties onto the console */
				cout << "Region Properties:" << endl;
				cout << "Moments:" << endl;
				
				cout << "m00 =" << m00[i] << " m01 =" << m01[i] << 
					" m10 =" << m10[i] << " m11 =" << m11[i] << 
					" m02 =" << m02[i] << " m20 =" << m20[i] << endl;

				cout << "Central Moments:" << endl;
				cout << "mu00 =" << mu00[i] << " mu01 =" << mu01[i] <<
					" mu10 =" << mu10[i] << " mu11 =" << mu11[i] <<
					" mu02 =" << mu02[i] << " mu20 =" << mu20[i] << endl;

				cout << "Area: " << area[i];
				cout << "Perimeter: " << perimeter << endl;
				cout << "Compactness: " << compactness[i] << endl;
				
				cout << "Eccentricity: " << eccentricity[i] << endl;
				cout << "Direction: " << direction[i] << endl;
				cout << "Centroid: (" << xc[i] << "," << yc[i] << ")" << endl;
				cout << "Major Axes Points: (" << major_x1_y1.x << ", " << major_x1_y1.y << ") , (" << major_x2_y2.x << ", " << major_x2_y2.y << ")" << endl;
				cout << "Minor Axes Points: (" << minor_x1_y1.x << ", " << minor_x1_y1.y << ") , (" << minor_x2_y2.x << ", " << minor_x2_y2.y << ")" << endl;

				/* Drawing a cross mark at the centroid with lengths corresponding to the major and minor axes*/
				DrawLine(major_x1_y1, major_x2_y2, &colorImg, Bgr(255, 0, 0), 1);
				DrawLine(minor_x1_y1, minor_x2_y2, &colorImg, Bgr(255, 0, 0), 1);

				/* Fruit Classification using the already computed Region Properties */
				if (eccentricity[i] >= 0.8) {
					cout << "\nThese region properties corresponds to a Banana\n" << endl;
					for (unsigned int j = 0; j < chain.size();++j) {						
						colorImg(chain[j].x, chain[j].y) = Bgr::YELLOW;		//Color the boundary yellow
					}

					/* Finding the banana stem
					   Erosions followed by subsequent dilations 
					   and then XORing with the original image.
					   Eroding and Dilating again to remove the unwanted regions */
					ImgBinary tmp1, tmp2, origImg, stem;
					Convert(labels, &origImg);
					Erode3x3(origImg, &tmp1);
					Erode3x3(tmp1, &tmp2);
					Erode3x3(tmp2, &tmp1);
					Erode3x3(tmp1, &tmp2);
					Erode3x3(tmp2, &tmp1);
										
					Dilate3x3(tmp1, &tmp2);
					Dilate3x3(tmp2, &tmp1);
					Dilate3x3(tmp1, &tmp2);
					Dilate3x3(tmp2, &tmp1);
					Dilate3x3(tmp1, &tmp2);
					
					Xor(tmp2, origImg, &stem);

					Erode3x3Cross(stem, &tmp1);
					Erode3x3Cross(tmp1, &stem);
					Dilate3x3Cross(stem, &tmp1);
					Dilate3x3Cross(tmp1, &stem);
					
					ImgBinary::ConstIterator p = stem.Begin();
					ImgBgr::Iterator out = colorImg.Begin();
					
					while(p != stem.End()){
						if (*p++ && *out == Bgr::YELLOW) {
							*out = Bgr::MAGENTA;
						}
						++out;
					}
				}
				else if (eccentricity[i] < 0.5 && compactness[i] >= 0.35 && area[i] >= 5500) {
					cout << "\nThese region properties corresponds to a Grapefruit\n" << endl;
					for (unsigned int j = 0; j < chain.size(); ++j) {
						colorImg(chain[j].x, chain[j].y) = Bgr::GREEN;		//Color the boundary green
					}
				}
				else if(eccentricity[i] < 0.5 && area[i] < 5500){
					cout << "\nThese region properties corresponds to an Apple\n" << endl;
					for (unsigned int j = 0; j < chain.size();++j) {
						colorImg(chain[j].x, chain[j].y) = Bgr::RED;		//Color the boundary red
					}
				}
			}
		}
		colorFig.Draw(colorImg);
	}
	catch (const Exception& e) {
		e.Display();
		exit(0);
	}

	// Loop forever until user presses Ctrl-C in terminal window.
	EventLoop();

	return 0;
}
