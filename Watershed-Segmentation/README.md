#Watershed-Segmentation-Algorithm

This project implements the simplified Vincent-Soille marker-based watershed segmentation algorithm.

Input files: holes.pgm, cells_small.pgm

This project does the following:

1.  Reads 2 command-line parameters, which we will call filename and threshold.  

2.  The very first thing the program does is print a suggested threshold value for both images that we will be using (holes.pgm and cells_small.pgm). 

3.  If fewer than 2 parameters are provided, then the program exits.

4.  Detect markers, as defined by the threshold.

5.  Perform the marker-based watershed algorithm.  The basic algorithm involves two preliminary steps:  (1) Compute the magnitude of the image gradient, quantized; (2) Construct a data structure allowing fast access to all the pixels with a certain value.  Then the algorithm iterates through the image gradient values, applying three steps in sequence:  

    *  Grow existing catchment basins by one pixel

    *  Apply breadth-first search to flood the pixels, assigning pixels to the nearest existing catchment basin, if there is one.  Be sure to use the FIFO queue of std::queue or std::deque (unless you want to write your own) to ensure breadth-first search.

    *  Create new catchment basins for pixels that are not adjacent to an existing catchment basin using Floodfill.


