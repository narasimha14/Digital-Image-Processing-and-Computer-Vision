#Feature-Tracking-using-Lucas-Kanade

This project implements the Lucas-Kanade algorithm to detect and track features through a video sequence of images.

Input files: flowergarden.zip, statue_sequence.zip

This project does the following:

1.  Reads 3 command-line parameters, which we will call filename-format, first-frame, last-frame, sigma, and window-size.  

2.  Use filename-format and first-frame to last-frame to determine the filename of the first image of the sequence. 

3.  Detect good features in the first frame.  The parameters for feature detection do not have to be the same as those for feature tracking.  Therefore, we hardcode a different sigma for our gradient computation here (independent of the sigma parameter), and use a small 3x3 window for constructing the gradient covariance matrix (as opposed to using window-size).  Use the Tomasi-Kanade method of thresholding the minimum eigenvalue to compute a measure of “cornerness” for every pixel in the image.  Then perform non-maximal suppression to set the “cornerness” to zero for every pixel that is not a local maximum in a 3x3 neighborhood (using either 4- or 8-neighbors).  Note that, unlike Canny, this non-max suppression will not care about the direction in which neighbors lie relative to the pixel, but instead will consider all the pixels in the neighborhood at once.  We also enforce a minimum distance between features, but it will be simpler to either allow no more than 1 feature in each 8x8 image block, or to increase the value of  (not sigma) accordingly.

4.  Loop over all the frame numbers, from first-frame to last-frame, using filename-format to determine the filename of the current image. 

5.  For each pair of consecutive frames, perform Lucas-Kanade tracking of all the features to update their 2D image positions.  Use sigma to compute the gradient of the image, and use window-size as the size of the window over which to accumulate information for constructing Z and e.  Keep the feature coordinates as floating point values throughout the tracking process, only rounding for display purposes; to handle non-integer values, use bilinear interpolation.

