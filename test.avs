Debug = 1
SSIFFile = "00001.ssif"#"indoor.MTS"

LoadPlugin(((Debug == 1) ? "Debug" : "Release") + "\ssifSource2.dll")

#ssifSource2(SSIFFile, 1465, left_view = true, right_view = true)

#ssifSource2(SSIFFile, 465, left_view = true, right_view = true)

c = ssifSource2(SSIFFile, 1465, left_view = true, right_view = false)#.reverse
return c

