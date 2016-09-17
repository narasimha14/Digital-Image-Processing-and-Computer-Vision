rem #!/bin/bash          

rem Run this file to compile all the assembly files into object files,
rem    in case Visual Studio is not behaving.  On some machines, for some reason,
rem    Visual Studio cannot run C:\Windows\System32\nasmw, leading to the error
rem    'nasmw.exe' is not recognized as an internal or external command, operable program or batch file.

echo Compiling .asm files ...

nasmw mmx_absdiff.asm -o mmx_absdiff.obj
nasmw mmx_and.asm -o mmx_and.obj
nasmw mmx_const_and.asm -o mmx_const_and.obj
nasmw mmx_const_diff.asm -o mmx_const_diff.obj
nasmw mmx_const_or.asm -o mmx_const_or.obj
nasmw mmx_const_sum.asm -o mmx_const_sum.obj
nasmw mmx_const_xor.asm -o mmx_const_xor.obj
nasmw mmx_convolve_prewitt_horiz_abs.asm -o mmx_convolve_prewitt_horiz_abs.obj
nasmw mmx_convolve_prewitt_vert_abs.asm -o mmx_convolve_prewitt_vert_abs.obj
nasmw mmx_dilate.asm -o mmx_dilate.obj
nasmw mmx_erode.asm -o mmx_erode.obj
nasmw mmx_gauss_1x3.asm -o mmx_gauss_1x3.obj
nasmw mmx_gauss_3x1.asm -o mmx_gauss_3x1.obj
nasmw mmx_not.asm -o mmx_not.obj
nasmw mmx_or.asm -o mmx_or.obj
nasmw mmx_shiftleft.asm -o mmx_shiftleft.obj
nasmw mmx_shiftright.asm -o mmx_shiftright.obj
nasmw mmx_subtract.asm -o mmx_subtract.obj
nasmw mmx_sum.asm -o mmx_sum.obj
nasmw mmx_supported.asm -o mmx_supported.obj
nasmw mmx_xor.asm -o mmx_xor.obj
nasmw xmm_absdiff.asm -o xmm_absdiff.obj
nasmw xmm_and.asm -o xmm_and.obj
nasmw xmm_const_and.asm -o xmm_const_and.obj
nasmw xmm_const_diff.asm -o xmm_const_diff.obj
nasmw xmm_const_or.asm -o xmm_const_or.obj
nasmw xmm_const_sum.asm -o xmm_const_sum.obj
nasmw xmm_const_xor.asm -o xmm_const_xor.obj
nasmw xmm_not.asm -o xmm_not.obj
nasmw xmm_or.asm -o xmm_or.obj
nasmw xmm_shiftleft.asm -o xmm_shiftleft.obj
nasmw xmm_shiftright.asm -o xmm_shiftright.obj
nasmw xmm_subtract.asm -o xmm_subtract.obj
nasmw xmm_sum.asm -o xmm_sum.obj
nasmw xmm_xor.asm -o xmm_xor.obj


echo Done.  Now move .obj files to Debug and/or Release directories.
