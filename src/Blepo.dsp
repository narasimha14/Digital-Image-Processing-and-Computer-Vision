# Microsoft Developer Studio Project File - Name="Blepo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Blepo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Blepo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Blepo.mak" CFG="Blepo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Blepo - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Blepo - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Blepo - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release-VC6"
# PROP Intermediate_Dir "Release-VC6"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "./" /I "../external/Microsoft/DirectX/DXSDK/BaseClasses" /I "../external/Microsoft/DirectX/DXSDK/Common" /I "../external/Microsoft/DirectX/DXSDK/Include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_VISUALC_" /D _WIN32_WINNT=0x400 /D "BLEPO_I_AM_USING_VISUAL_CPP_60__" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /ignore:4006,4221

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug-VC6"
# PROP Intermediate_Dir "Debug-VC6"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "./" /I "../external/Microsoft/DirectX/DXSDK/BaseClasses" /I "../external/Microsoft/DirectX/DXSDK/Common" /I "../external/Microsoft/DirectX/DXSDK/Include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_VISUALC_" /D _WIN32_WINNT=0x400 /D "BLEPO_I_AM_USING_VISUAL_CPP_60__" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /ignore:4006,4221

!ENDIF 

# Begin Target

# Name "Blepo - Win32 Release"
# Name "Blepo - Win32 Debug"
# Begin Group "Capture"

# PROP Default_Filter ""
# Begin Group "IEEE1394Camera"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Capture\IEEE1394Camera\1394camapi.h
# End Source File
# Begin Source File

SOURCE=.\Capture\IEEE1394Camera\1394Camera.h
# End Source File
# Begin Source File

SOURCE=.\Capture\IEEE1394Camera\1394CameraControl.h
# End Source File
# Begin Source File

SOURCE=.\Capture\IEEE1394Camera\1394CameraControlSize.h
# End Source File
# Begin Source File

SOURCE=.\Capture\IEEE1394Camera\1394CameraControlTrigger.h
# End Source File
# Begin Source File

SOURCE=.\Capture\IEEE1394Camera\1394common.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Capture\AVIFrameReader.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\AVIFrameReader.h
# End Source File
# Begin Source File

SOURCE=.\Capture\CanonVcc4.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\CanonVcc4.h
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureAvi.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureAvi.h
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureDirectShow.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureDirectShow.h
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureDt3120.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureDt3120.h
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureIEEE1394.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureIEEE1394.h
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureImageSequence.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureImageSequence.h
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureVlc.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\CaptureVlc.h
# End Source File
# Begin Source File

SOURCE=.\Capture\DirectShowForDummies.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\DirectShowForDummies.h
# End Source File
# Begin Source File

SOURCE=.\Capture\SerialPort.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture\SerialPort.h
# End Source File
# Begin Source File

SOURCE=.\Capture\TripleBuffer.h
# End Source File
# End Group
# Begin Group "Figure"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Figure\Figure.cpp
# End Source File
# Begin Source File

SOURCE=.\Figure\Figure.h
# End Source File
# Begin Source File

SOURCE=.\Figure\FigureGlut.cpp
# End Source File
# Begin Source File

SOURCE=.\Figure\FigureGlut.h
# End Source File
# Begin Source File

SOURCE=.\Figure\SimplePlot.cpp
# End Source File
# Begin Source File

SOURCE=.\Figure\SimplePlot.h
# End Source File
# End Group
# Begin Group "Image"

# PROP Default_Filter ""
# Begin Group "Algorithms"

# PROP Default_Filter ""
# Begin Group "edison"

# PROP Default_Filter ""
# Begin Group "segm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Image\edison\segm\ms.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\edison\segm\ms.h
# End Source File
# Begin Source File

SOURCE=.\Image\edison\segm\msImageProcessor.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\edison\segm\msImageProcessor.h
# End Source File
# Begin Source File

SOURCE=.\Image\edison\segm\RAList.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\edison\segm\RAList.h
# End Source File
# Begin Source File

SOURCE=.\Image\edison\segm\rlist.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\edison\segm\rlist.h
# End Source File
# Begin Source File

SOURCE=.\Image\edison\segm\tdef.h
# End Source File
# End Group
# End Group
# Begin Group "klt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Image\klt\base.h
# End Source File
# Begin Source File

SOURCE=.\Image\klt\convolve.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\convolve.h
# End Source File
# Begin Source File

SOURCE=.\Image\klt\error.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\error.h
# End Source File
# Begin Source File

SOURCE=.\Image\klt\klt.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\klt.h
# End Source File
# Begin Source File

SOURCE=.\Image\klt\klt_util.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\klt_util.h
# End Source File
# Begin Source File

SOURCE=.\Image\klt\pnmio.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\pnmio.h
# End Source File
# Begin Source File

SOURCE=.\Image\klt\pyramid.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\pyramid.h
# End Source File
# Begin Source File

SOURCE=.\Image\klt\selectGoodFeatures.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\storeFeatures.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\trackFeatures.c
# End Source File
# Begin Source File

SOURCE=.\Image\klt\writeFeatures.c
# End Source File
# End Group
# Begin Group "opencv"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\external\Intel\OpenCV\cv.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Image\ActiveContour.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Calibration.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\CamShift.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Canny.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Chamfer.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\ChanVese.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\ConnectedComponents.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\CrossCorrelation.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Edison.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\EfrosLeung.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\EllipticalHeadTracker.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\EquivalenceTable.h
# End Source File
# Begin Source File

SOURCE=.\Image\FaceDetector.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\FastFeatureTracker.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\FHGraphSegmentation.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Filters.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Floodfill.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Histogram.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\HornSchunck.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\IntelHough.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\JointFeatureTracker.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\LineFitting.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\LucasKanade.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\LucasKanadeAffine.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\MaxFlowMinCut.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\MaxFlowMinCut.h
# End Source File
# Begin Source File

SOURCE=.\Image\Motion.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\RealTimeStereo.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\RegionProps.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\SplitAndMerge.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\WallFollow.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Watershed.cpp
# End Source File
# End Group
# Begin Group "FileIO"

# PROP Default_Filter ""
# Begin Group "Jpeglib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Image\jpeglib\cderror.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\cdjpeg.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcapimin.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcapistd.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jccoefct.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jccolor.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jchuff.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jchuff.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcinit.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcmainct.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcmarker.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcmaster.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcomapi.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcparam.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcphuff.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcprepct.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jcsample.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jctrans.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdapimin.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdapistd.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdatadst.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdcolor.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdct.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdhuff.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdhuff.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdinput.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdmainct.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdmarker.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdmaster.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdmerge.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdphuff.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdpostct.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdsample.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jdtrans.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jerror.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jerror.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jfdctflt.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jfdctfst.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jfdctint.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jidctflt.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jidctfst.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jidctint.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jidctred.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jinclude.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jmemdst.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jmemnobs.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jmemsrc.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jmemsys.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jpegint.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jpeglib.h
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jpegtran.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jquant1.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jquant2.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jutils.c
# End Source File
# Begin Source File

SOURCE=.\Image\jpeglib\jversion.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Image\Bmpfile.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Bmpfile.h
# End Source File
# Begin Source File

SOURCE=.\Image\Jpegfile.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Jpegfile.h
# End Source File
# Begin Source File

SOURCE=.\Image\PnmFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\PnmFile.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Image\Image.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\Image.h
# End Source File
# Begin Source File

SOURCE=.\Image\ImageAlgorithms.h
# End Source File
# Begin Source File

SOURCE=.\Image\ImageOperations.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\ImageOperations.h
# End Source File
# Begin Source File

SOURCE=.\Image\ImgIplImage.cpp
# End Source File
# Begin Source File

SOURCE=.\Image\ImgIplImage.h
# End Source File
# Begin Source File

SOURCE=.\Image\PointCloud.h
# End Source File
# End Group
# Begin Group "Matrix"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Matrix\CholeskyDecomp.cpp
# End Source File
# Begin Source File

SOURCE=.\Matrix\LinearAlgebra.cpp
# End Source File
# Begin Source File

SOURCE=.\Matrix\LinearAlgebra.h
# End Source File
# Begin Source File

SOURCE=.\Matrix\Matrix.cpp
# End Source File
# Begin Source File

SOURCE=.\Matrix\Matrix.h
# End Source File
# Begin Source File

SOURCE=.\Matrix\MatrixOperations.cpp
# End Source File
# Begin Source File

SOURCE=.\Matrix\MatrixOperations.h
# End Source File
# End Group
# Begin Group "Quick"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Quick\mmx_absdiff.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_absdiff.asm
InputName=mmx_absdiff

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_absdiff.asm
InputName=mmx_absdiff

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_and.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_and.asm
InputName=mmx_and

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_and.asm
InputName=mmx_and

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_const_and.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_const_and.asm
InputName=mmx_const_and

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_const_and.asm
InputName=mmx_const_and

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_const_diff.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_const_diff.asm
InputName=mmx_const_diff

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_const_diff.asm
InputName=mmx_const_diff

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_const_or.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_const_or.asm
InputName=mmx_const_or

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_const_or.asm
InputName=mmx_const_or

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_const_sum.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_const_sum.asm
InputName=mmx_const_sum

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_const_sum.asm
InputName=mmx_const_sum

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_const_xor.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_const_xor.asm
InputName=mmx_const_xor

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_const_xor.asm
InputName=mmx_const_xor

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_convolve_prewitt_horiz_abs.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_convolve_prewitt_horiz_abs.asm
InputName=mmx_convolve_prewitt_horiz_abs

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_convolve_prewitt_horiz_abs.asm
InputName=mmx_convolve_prewitt_horiz_abs

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_convolve_prewitt_vert_abs.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_convolve_prewitt_vert_abs.asm
InputName=mmx_convolve_prewitt_vert_abs

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_convolve_prewitt_vert_abs.asm
InputName=mmx_convolve_prewitt_vert_abs

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_dilate.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_dilate.asm
InputName=mmx_dilate

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_dilate.asm
InputName=mmx_dilate

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_erode.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_erode.asm
InputName=mmx_erode

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_erode.asm
InputName=mmx_erode

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_gauss_1x3.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_gauss_1x3.asm
InputName=mmx_gauss_1x3

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_gauss_1x3.asm
InputName=mmx_gauss_1x3

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_gauss_3x1.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_gauss_3x1.asm
InputName=mmx_gauss_3x1

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_gauss_3x1.asm
InputName=mmx_gauss_3x1

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_not.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_not.asm
InputName=mmx_not

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_not.asm
InputName=mmx_not

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_or.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_or.asm
InputName=mmx_or

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_or.asm
InputName=mmx_or

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_shiftleft.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_shiftleft.asm
InputName=mmx_shiftleft

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_shiftleft.asm
InputName=mmx_shiftleft

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_shiftright.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_shiftright.asm
InputName=mmx_shiftright

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_shiftright.asm
InputName=mmx_shiftright

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_subtract.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_subtract.asm
InputName=mmx_subtract

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_subtract.asm
InputName=mmx_subtract

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_sum.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_sum.asm
InputName=mmx_sum

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_sum.asm
InputName=mmx_sum

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_supported.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_supported.asm
InputName=mmx_supported

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_supported.asm
InputName=mmx_supported

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\mmx_xor.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\mmx_xor.asm
InputName=mmx_xor

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\mmx_xor.asm
InputName=mmx_xor

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\Quick.cpp
# End Source File
# Begin Source File

SOURCE=.\Quick\Quick.h
# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_absdiff.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_absdiff.asm
InputName=xmm_absdiff

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_absdiff.asm
InputName=xmm_absdiff

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_and.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_and.asm
InputName=xmm_and

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_and.asm
InputName=xmm_and

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_const_and.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_const_and.asm
InputName=xmm_const_and

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_const_and.asm
InputName=xmm_const_and

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_const_diff.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_const_diff.asm
InputName=xmm_const_diff

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_const_diff.asm
InputName=xmm_const_diff

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_const_or.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_const_or.asm
InputName=xmm_const_or

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_const_or.asm
InputName=xmm_const_or

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_const_sum.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_const_sum.asm
InputName=xmm_const_sum

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_const_sum.asm
InputName=xmm_const_sum

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_const_xor.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_const_xor.asm
InputName=xmm_const_xor

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_const_xor.asm
InputName=xmm_const_xor

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_not.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_not.asm
InputName=xmm_not

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_not.asm
InputName=xmm_not

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_or.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_or.asm
InputName=xmm_or

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_or.asm
InputName=xmm_or

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_shiftleft.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_shiftleft.asm
InputName=xmm_shiftleft

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_shiftleft.asm
InputName=xmm_shiftleft

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_shiftright.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_shiftright.asm
InputName=xmm_shiftright

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_shiftright.asm
InputName=xmm_shiftright

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_subtract.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_subtract.asm
InputName=xmm_subtract

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_subtract.asm
InputName=xmm_subtract

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_sum.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_sum.asm
InputName=xmm_sum

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_sum.asm
InputName=xmm_sum

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Quick\xmm_xor.asm

!IF  "$(CFG)" == "Blepo - Win32 Release"

# Begin Custom Build
OutDir=.\Release-VC6
InputPath=.\Quick\xmm_xor.asm
InputName=xmm_xor

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "Blepo - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug-VC6
InputPath=.\Quick\xmm_xor.asm
InputName=xmm_xor

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw.exe -f win32 -o "$(OUTDIR)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Utilities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Utilities\Array.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities\Array.h
# End Source File
# Begin Source File

SOURCE=.\Utilities\Exception.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities\Exception.h
# End Source File
# Begin Source File

SOURCE=.\Utilities\Math.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities\Math.h
# End Source File
# Begin Source File

SOURCE=.\Utilities\mt.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities\Mutex.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities\Mutex.h
# End Source File
# Begin Source File

SOURCE=.\Utilities\PointSizeRect.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities\PointSizeRect.h
# End Source File
# Begin Source File

SOURCE=.\Utilities\Reallocator.h
# End Source File
# Begin Source File

SOURCE=.\Utilities\Utilities.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities\Utilities.h
# End Source File
# End Group
# Begin Group "Lib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\external\FFTW\fftw3.lib
# End Source File
# Begin Source File

SOURCE=..\external\IEEE1394\1394camera.lib
# End Source File
# Begin Source File

SOURCE=..\external\WinGsl\WinGsl.lib
# End Source File
# Begin Source File

SOURCE=..\external\DataTranslation\DT3120\Olfg32.lib
# End Source File
# Begin Source File

SOURCE=..\external\DataTranslation\DT3120\DtColorSdk.lib
# End Source File
# Begin Source File

SOURCE=..\external\DataTranslation\DT3120\Olimg32.lib
# End Source File
# Begin Source File

SOURCE=..\external\Microsoft\DirectX\strmiids.lib
# End Source File
# Begin Source File

SOURCE=..\external\Microsoft\DirectX\dmoguids.lib
# End Source File
# Begin Source File

SOURCE=..\external\Microsoft\DirectX\WINMM.LIB
# End Source File
# Begin Source File

SOURCE=..\external\Microsoft\DirectX\strmbase.lib
# End Source File
# Begin Source File

SOURCE=..\external\Microsoft\VFW32.LIB
# End Source File
# Begin Source File

SOURCE="..\external\OpenCV-1\highgui.lib"
# End Source File
# Begin Source File

SOURCE="..\external\OpenCV-1\cv.lib"
# End Source File
# Begin Source File

SOURCE="..\external\OpenCV-1\cxcore.lib"
# End Source File
# Begin Source File

SOURCE=..\external\OpenGL\freeglut.lib
# End Source File
# Begin Source File

SOURCE=..\external\OpenGL\glu.lib
# End Source File
# Begin Source File

SOURCE=..\external\OpenGL\opengl.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\blepo.h
# End Source File
# End Target
# End Project
