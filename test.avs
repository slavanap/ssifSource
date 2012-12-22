Debug = 0
SSIFFile = "indoor.MTS"

LoadPlugin(((Debug == 1) ? "Debug" : "Release") + "\ssifSource2.dll")
ssifSource2(SSIFFile, 1465, left_view = true, right_view = true)
