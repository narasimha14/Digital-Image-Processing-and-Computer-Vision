#Canny-Edge-Detector

This project implements the Canny Edge Detector and demonstrates the usage of the detector to find an object by matching the edges of an image with the edges of the object’s template via the chamfer distance.

Input files: cat.pgm , cameraman.pgm

This project does the following:

1.  Reads 2 or 3 command-line parameters, which we will call sigma, filename, and template.  In other words, the third parameter is optional when calling your program.  To properly call your program a user should either specify sigma and filename or sigma, filename, and template.  Examples (note that canny_edge is the name of the program):

    *  canny_edge 2.0 cat.pgm

    *  canny_edge 1.5 cherrypepsi.jpg cherrypepsi_template.jpg

2.  Construct a 1D Gaussian convolution kernel and a 1D Gaussian derivative convolution kernel, both with the specified value for sigma.  The length of each kernel should be automatically selected as explained in the notes.

3.  Compute the gradient of the image.  Use the principle of separability to apply the 1D Gaussian and 1D Gaussian derivative kernels.  Do not worry about image borders; the simplest solution is to simply set the border pixels in the convolution result to zero rather than extending the image, but extension is fine, too.

4.  Perform non-maximum suppression using the gradient magnitude and phase.

5.  Perform thresholding with hysteresis (i.e., double-thresholding).  Automatically compute the threshold values based upon image statistics.

6.  Compute the chamfer distance on the edge image with the Manhattan distance.  

7.  If the third parameter is specified (the “template”), then perform Canny edge detection on the template, too.  Then perform an exhaustive search (for simplicity, only consider locations for which the template is completely in bounds) for the best location of the template in the image.  If the image and/or template are color, then convert from color to grayscale before computing the edges.


