#Morphological-Operations-Thresholding-Floodfill

This project demonstrates the application of a number of basic pixel-based processing algorithms such as thresholding, floodfill, connected components, and morphological operations.

Input files: fruit1.bmp/fruit1.pgm, fruit2.bmp/fruit2.pgm

This project does the following:

1.  Reads 1 command-line parameter, which we will call filename.  

2.  Loads filename from the blepo/images directory into a Grayscale or BGR image and displays it in a figure window.

3.  Performs double thresholding using two thresholds that you determine by trial and error, which are hardcoded in your code.

4.  Perform noise removal (if needed) using mathematical morphology (i.e., some combination of erosion / dilation / opening / closing) either before or during thresholding.

5.  Runs connected components (either the classic union-find algorithm or by repeated applications of floodfill) to detect and count the foreground regions in the cleaned, thresholded image, distinguishing them from the background.  

6.  Computes the properties of each foreground region, including
    *  zeroth-, first- and second-order moments (regular and centralized)

    *  compactness (To compute the area, simply count the number of pixels.  To compute the perimeter, apply the logical XOR to the thresholded image and the result of eroding this image with a 3x3 structuring element of all ones; the result will be the number of 4-connected foreground boundary pixels.)

    *  eccentricity (or elongatedness), using eigenvalues

    *  direction, using either eigenvectors (PCA) or the moments formula (they are equivalent)


7.  Automatically classifies each piece of fruit into one of three categories:  apple, grapefruit, or banana, using a combination of these foreground properties.

8.  Detects the banana stem.

