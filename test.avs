Debug = 0

LoadPlugin("c:\Users\Vyacheslav\Projects\Utils\ssifSource2\" + ((Debug == 1) ? "Debug" : "Release") + "\ssifSource2.dll")
src = ssifSource("samples\ISO\BDMV\STREAM\SSIF\00001.ssif", 100+465, avc_view = false)

#l = src.crop(0,0,src.width,src.height/2).subtitle("left")
#r = src.crop(0,src.height/2,src.width,src.height/2).subtitle("right")
#interleave(l,r)

return src
