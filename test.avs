Debug = 0
SSIFFile = "samples\indoor.MTS"

LoadPlugin("c:\Users\Vyacheslav\Projects\Utils\ssifSource2\" + ((Debug == 1) ? "Debug" : "Release") + "\ssifSource2.dll")

SP_LEFTVIEW = 1
SP_RIGHTVIEW = 2
SP_HORIZONTAL = 4

#ssifSource("00001.ssif", 1920, 1080, 465, show_params = SP_RIGHTVIEW)
#ssifSource2("00001.ssif", 465, avc_view = true, mvc_view = true)
#ssifSource3("title01 (1)-004.mkv;1000", avc_view = true, mvc_view = true)

src = ssifSource3("samples\00001.ssif;450;samples\00001.ssif;450;samples\00001.ssif;450", avc_view = true, mvc_view = true)

l = src.crop(0,0,src.width,src.height/2).subtitle("left")
r = src.crop(0,src.height/2,src.width,src.height/2).subtitle("right")
interleave(l,r)
#return src
