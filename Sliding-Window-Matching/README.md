#Sliding-Window-Matching

This project implements efficient block-based Sliding Window Matching of two rectified stereo images.

Input files: tsukuba_left.ppm and tsukuba_right.ppm, lamp_left.pgm and lamp_right.pgm

This project does the following:

1.  Reads 3 command-line parameters, which we will call left-filename, right-filename, and max-disparity.  

2.  Load the two (grayscale or color) images.  If these images are color, then convert to grayscale, so that only grayscale is used for matching.  This makes the code easier to debug and more computationally efficient, without sacrificing anything in the quality of the results.  Nevertheless, it reads the color values so that we can output them in the MeshLab PLY file in step 4) below.

3.  Perform block-based matching of the two images.  For efficiency, this code precomputes the 3D array of dissimilarities, followed by a series of separable convolutions (one pair of convolutions per disparity).  Uses the SAD dissimilarity measure.  Implements the left-to-right consistency check, retaining a value in the left disparity map only if the corresponding point in the right disparity map agrees in its disparity.  The resulting disparity map is valid only at the pixels that pass the consistency check; sets other pixels to zero.  (Note: For simplicity, we do not worry about pixels along the left border of the left image; it is ok the produce erroneous values there.)

4.  Use triangulation to determine the depth of each matched pixel.  The formula is depth = k / disparity, where k is the focal length times the baseline.  Since we do not know the value of k, we will have to manually try a few values until you get a result that looks plausible.  Output a PLY file that can be read by MeshLab (details below).  This project outputs six columns (x y z r g b) for each matched pixel, ignoring the normal components.  

    *  Use orthographic projection (rather than perspective) to get the x,y,z coordinates, because it is simpler to implement and will yield a more aesthetically pleasing point cloud, even though it is less accurate mathematically. In other words, if the coordinates of the matched pixel in the image are (x,y), and the depth is z (from the triangulation formula above), then the 3D coordinates are (x,y,z).  

    *  Although there is no reason for our code to use the RGB values for matching, we use RGB color to make our PLY file more aesthetically pleasing. 

