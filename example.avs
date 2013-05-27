LoadPlugin("ssifSource2.dll")

# Usage:
#   ssifSource2(ssif_file, frame_count, left_view, right_view, horizontal_stack)
# Defaults:
#   frame_count = <autodetect> | may guess wrong framecount
#   left_view = false          | show left view
#   right_view = true          | show right view
#   horizontal_stack = false   | stack views vertically or horizontally
#
# Seeking stream may seek to wrong position in video.
#
# Example:
#   ssifSource2("00001.ssif", 465)

# Parameters for conversion:
left_file = "titanic_disk1_00001_left.mkv"
ssif_file = "00001.ssif"
ssif_disk = "G"

########################################################################
src_path = "z:\14v_nap\3d films right views\"
left_file = src_path + left_file
ssif_file = ssif_disk + ":\BDMV\STREAM\SSIF\" + ssif_file

left_clip = FFmpegsource2(left_file)
frames = left_clip.framecount

return ssifSource2(ssif_file, frames, left_view = false, right_view = true)
