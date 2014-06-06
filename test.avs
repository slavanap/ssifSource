SA_DEMUXER = 0
SA_MUXER   = 1
SA_DECODER = 2


Debug = 1

#LoadPlugin("bin\ssifSource4.dll")
LoadPlugin("c:\Users\Vyacheslav\Projects\Utils\ssifSource2\" + ((Debug == 1) ? "Debug" : "Release") + "\ssifSource4.dll")

#src = mplsSource("c:\Users\Vyacheslav\Downloads\Oz.the.Great.and.Powerful.(2013).Blu-Ray.2D\BDMV\PLAYLIST\00800.mpls", mvc_view = false)
#ssifSource("..\samples\ISO\BDMV\STREAM\SSIF\00001.ssif", 465)
#mplsSource("d:\BDMV\PLAYLIST\00800.mpls", mvc_view = false)
#ssifSource("d:\BDMV\STREAM\SSIF\00501.ssif", 108155, debug = true, stop_after=SA_MUXER, muxed264="c:\Users\Vyacheslav\Projects\Utils\test\000\dump.h264" )

#mplsSource("samples\ISO\BDMV\PLAYLIST\00001.mpls", mvc_view = false, debug=false)
#mplsSource("samples\ISO\BDMV\PLAYLIST\00001.mpls", mvc_view = false, debug=false)
ssifSource("d:\BDMV\STREAM\SSIF\00009.ssif", 100000, debug = true, stop_after=SA_MUXER, muxed264="c:\Users\Vyacheslav\Projects\Utils\test\000\dump.h264" )

