SA_DEMUXER = 0
SA_MUXER   = 1
SA_DECODER = 2


Debug = 1

LoadPlugin("c:\Users\Vyacheslav\Projects\Utils\ssifSource2\" + ((Debug == 1) ? "Debug" : "Release") + "\ssifSource4.dll")

#mplsSource("samples\ISO\BDMV\PLAYLIST\00001.mpls", mvc_view = false, debug=false)
#mplsSource("x:\JRNY_CNTR_EARTH_3D_BLUEBIRD.iso|BDMV\PLAYLIST\00004.mpls", right_view = false, debug=false)
#ssifSource("d:\BDMV\STREAM\SSIF\00009.ssif", 100000, debug = true, stop_after=SA_MUXER, muxed264="c:\Users\Vyacheslav\Projects\Utils\test\000\dump.h264" )


SAVC = "AVC.h264" #Source base stream
SMVC = "MVC.h264" #Source dependant stream
FC = 1000 #(manually predetermined)
Swap = False #(manually predetermined)
TC = 16
BC = 32
LC = 8
RC = 4	#Cropping amounts (manually predetermined)

Source = ssifSource (avc264 = SAVC, mvc264 = SMVC, frame_count = FC, use_ldecod = true, swap_views = Swap)
Left = Source.Crop (LC, TC, -RC, -BC - 1080)
Right = Source.Crop (LC, 1080 + TC, -RC, -BC)
Left = SelectEven (SeparateRows (Blur (Left, 0, 1), 2))
Right = SelectOdd (SeparateRows (Blur (Right, 0, 1), 2))
HOU = StackVertical (Left, Right)
return HOU.Trim (0, FC - 1)