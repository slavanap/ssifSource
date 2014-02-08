#LoadPlugin("Win32\Debug\ssifSource.dll")
LoadPlugin("Release\ssifSource2.dll")

#
# ssifSource plugin
# Parameters: 
#   1 | ssif_file   | string  | name of .ssif file
#   2 | width       | integer | width of video
#   3 | height      | integer | height of video
#   4 | frame_count | integer | number of frames to decode
#   5 | left_track  | integer | left eye track in eac3to output, that will be used
#   6 | right_track | integer | same for right eye
#   7 | left_264    | string  | name of .h264 video file used for manual decoding for left eye
#   8 | right_264   | string  | same for right right eye (delta)
#   9 | show_params | integer | set of flags such as SP_LEFTVIEW, SP_RIGHTVIEW, SP_HORIZONTAL
#								use their sum to get effect of showing only one frame or stack frames horizontally
#								default value is (SP_LEFTVIEW + SP_RIGHTVIEW)
#
# Examples:
#  1. ssifSource("e:\BDMV\STREAM\SSIF\00005.ssif", 1920, 1080, 1000)
#     or
#     ssifSource(ssif_file = "e:\BDMV\STREAM\SSIF\00005.ssif", width = 1920, height = 1080, frame_count = 1000)
#
#  2. ssifSource("e:\BDMV\PLAYLIST\00001.mpls", 1920, 1080, 1000, 2, 3)
#     note: make sure about track numbers before using the filter. Defaults are: left_track = 1, right_track = 2
#
#  3. ssifSource(width = 1920, height = 1080, frame_count = 1000, left_264 = "left.h264", right_264 = "right.h264")
#     note: it's better when filenames of .h264 files don't have spaces and special characters, 
#           because sometimes ldecod.exe fails to parse such commandline
#
#  WARNING: ssifSource plugin ALWAYS produce video of 24000/1001 fps, so make sure you set proper fps settings before encoding!
#

SP_LEFTVIEW = 1
SP_RIGHTVIEW = 2
SP_HORIZONTAL = 4

#src = ssifSource("e:\BDMV\PLAYLIST\00001.mpls", 1920, 1080, 1000, 2, 3)
#src = ssifSource("e:\BDMV\PLAYLIST\00001.mpls", 1920, 1080, 1000, 2, 3, show_params = SP_LEFTVIEW + SP_RIGHTVIEW + SP_HORIZONTAL)
#src = ssifSource("e:\BDMV\STREAM\SSIF\00091.ssif", 1920, 1080, 1000)

src = ssifSource2("f:\BDMV\STREAM\SSIF\00896.ssif", avc_view = false, mvc_view = true)#.trim(98280, -200)

#src = ssifSource2("2.mp4")

return src
