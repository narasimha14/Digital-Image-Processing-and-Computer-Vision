// image_manipulation.cpp : Defines the entry point for the console application.
//

#include <afxwin.h>  // necessary for MFC to work properly
#include "image_manipulation.h"
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
		if (argc < 4) {
			printf("Too few arguments\nUsage: image_manipulation.exe <filename1> <filename2> <filename3>\n");
			exit(0);
		}

		ImgBgr img1;
		Figure fig1, fig2;

		string parentPath = "../../images/";
		string filename1 = parentPath + argv[1];

		Load(filename1.c_str(), &img1);

		fig1.Draw(img1);								//Display filename1 in the figure window

		ImgGray img2;
		int width = img1.Width();
		int height = img1.Height();
		img2.Reset(width, height);

		/* Creating a 100x100 squared region at the center */
		//If the image is smaller than 100x100, set all pixels to 255
		if (height*width < 100 * 100) {
			Set(&img2, 255);
		}
		else {
			//First set all pixels to 0
			Set(&img2, 0);

			//Using Blepo built-in class Rect to create the square at the center
			Rect r(width / 2 - 50, height / 2 - 50, width / 2 + 50, height / 2 + 50);
			Set(&img2, r, 255);

			//Can also create the square by looping through and setting the required pixel values
			/*for (int y = height / 2 - 50; y < height / 2 + 50; ++y) {
				for (int x = width / 2 - 50; x < width / 2 + 50; ++x) {
					img2(x, y) = 255;
				}
			}*/
		}

		fig2.Draw(img2);								// Display the  synthetic grayscale image in the figure window

		/* Split argv[2] and find the extension and print an error if it is jpeg*/
		string filename2;
		string extension;
		string file_split = argv[2];
		size_t pos = file_split.find('.');
		if (pos != -1) {
			filename2 = file_split.substr(0, pos);
			extension = file_split.erase(0, pos + 1);
		}
		else {
			BLEPO_ERROR("Filename must have an extension\n");
		}

		if (extension == "jpeg" || extension == "jpg") {
			BLEPO_ERROR("JPEG extension image not supported\n");
		}

		//Set the full path for saving the image
		filename2 = parentPath + filename2 + "." + extension;
		Save(img2, filename2.c_str(), extension.c_str());			// Save the synthetic grayscale image as filename2


		Figure fig3, fig4;

		ImgGray img3;
		string filename3 = parentPath + argv[3];
		Load(filename3.c_str(), &img3);
		fig3.Draw(img3);								//Display filename3 in the figure window

		if (width != img3.Width() || height != img3.Height()) {
			//Throw an error if filename3 and filename1 do not match in size
			BLEPO_ERROR("Image used for masking is of a different size\n");
		}
		else {
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					//Mask the BGR image
					if (img3(x, y) == 0) {
						img1(x, y).b = 0;
						img1(x, y).g = 0;
						img1(x, y).r = 0;
					}
				}
			}

			fig4.Draw(img1);							// Display the masked BGR image in the figure window
		}

	}
	catch (const Exception& e) {
		e.Display();
	}

	// Loop forever until user presses Ctrl-C in terminal window.
	EventLoop();

	return 0;
}
