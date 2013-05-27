Debug = 1
SSIFFile = "indoor.MTS"

LoadPlugin("c:\Users\Vyacheslav\Projects\Utils\ssifSource2\" + ((Debug == 1) ? "Debug" : "Release") + "\ssifSource2.dll")
ssifSource3("indoor.MTS;0", \
	avc_view = true, mvc_view = true)
