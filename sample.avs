# function proto:
# ssifSource(
#	string ssif_file,
#	int frame_count,
#	bool avc_view = true,
#	bool mvc_view = true,
#	bool horizontal_stack = false,
#	bool swap_views = false,
#	string intel_params = "" );

LoadPlugin("release\ssifSource2.dll")
return ssifSource("samples\ISO\BDMV\STREAM\SSIF\00001.ssif", 465, intel_params = "-d3d11")
