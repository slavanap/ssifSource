SA_DEMUXER = 0
SA_MUXER   = 1
SA_DECODER = 2


Debug = 1

#LoadPlugin("bin\ssifSource4.dll")
LoadPlugin(((Debug == 1) ? "Debug" : "Release") + "\ssifSource4.dll")

src = mplsSource("c:\Users\Vyacheslav\Downloads\Oz.the.Great.and.Powerful.(2013).Blu-Ray.2D\BDMV\PLAYLIST\00800.mpls", mvc_view = false)

return src
