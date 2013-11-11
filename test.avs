Debug = 0
SSIFFile = "samples\indoor.MTS"

LoadPlugin("c:\Users\Vyacheslav\Projects\Utils\ssifSource2\" + ((Debug == 1) ? "Debug" : "Release") + "\ssifSource2.dll")

SP_LEFTVIEW = 1
SP_RIGHTVIEW = 2
SP_HORIZONTAL = 4

#ssifSource("00001.ssif", 1920, 1080, 465, show_params = SP_RIGHTVIEW)
#ssifSource2("00001.ssif", 465, avc_view = true, mvc_view = true)
#ssifSource3("title01 (1)-004.mkv;1000", avc_view = true, mvc_view = true)

ssifSource2("2.mp4", avc_view = true, mvc_view = true).nnedi3
