#Image-Manipulation-Using-Blepo

This Visual Studio Project demonstrates simple manipulation of a given image thus illustrating the use of Blepo library.

This project does the following:
1.  Reads 3 command-line parameters, which we will call filename1, filename2, and filename3.  

2.  Loads filename1 from the blepo/images directory into a BGR image and displays it in a figure window.

3.  Creates a synthetic grayscale image of the same size as filename1 with each pixel in a 100x100 square region centered in the image set to the value 255, and all other pixels set to 0.  If filename1 is smaller than 100x100, the application sets all pixels in the synthetic image to 255.

4.  Displays the synthetic image in a separate figure window, and saves the synthetic image to filename2 in the blepo/images directory.

5.  Loads filename3 from the blepo/images directory into a grayscale image and displays it in a separate figure window.  (Note:  If filename3 is the same as filename2, then the square that you just created is loaded.)

6.  Masks the BGR image with the grayscale image just loaded.  That is, every pixel in the BGR image whose corresponding pixel in the grayscale image is 0 is set to black, (0,0,0), while all other pixels are left untouched.  If the BGR and grayscale images are different sizes, then an error is printed.

7.  Displays the resulting masked image in a separate figure window.


