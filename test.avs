Debug = 1

LoadPlugin("c:\Users\Vyacheslav\Projects\Utils\ssifSource2\" + ((Debug == 1) ? "Debug" : "Release") + "\ssifSource2.dll")
src = ssifSource("c:\Users\Vyacheslav\Projects\Utils\test\mux.result.h264", 465)

#l = src.crop(0,0,src.width,src.height/2).subtitle("left")
#r = src.crop(0,src.height/2,src.width,src.height/2).subtitle("right")
#interleave(l,r)

return src
