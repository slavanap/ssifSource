Debug = 1
SSIFFile = "00001.ssif"

LoadPlugin(((Debug == 1) ? "Debug" : "Release") + "\ssifSource2.dll")
ssifSource2(SSIFFile, 465, left_view = true, right_view = true)
