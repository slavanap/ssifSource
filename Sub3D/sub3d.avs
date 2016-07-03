test_dir = "c:\Users\Vyacheslav\Projects\ssifSource-misc\test sequences\"

#
# About: Plugin to create 3D subtitles with correct depth for 3D video files.
# Author: Vyacheslav Napadovsky, 2016.
# License: MIT.
#
# main functions:
#   * CalcXMLDepths(clip hstack, string xmlfile, [bool RewriteAlreadyExistingValues = true])
#       tries to calculate depths for captions in xml file. If RewriteAlreadyExistingValues is false the algorithm
#       won't touch depth values already defined in the file
#   * RenderXML(clip hstack, string xmlfile, [bool RenderForcedCaptionsOnly = false])
#       renders a subtitle xml file over the streams with depth values specified in xml. If there's no depth
#       value for a caption, then it'll be rendered with 0 depth.
#   * CalcSRTDepths(clip hstack, string srtfile, [string masksub], [progress = false])
#       tries to calculate depths for captions in srt/ass/ssa file. The algorithm will produce a file with name
#       (strfile + ".depths"). The file format is straightforward: each line contains (from left to right):
#       subtitle number, number of frame where subtitle starts, length in frames of subtitle, computed depth value in pixels.
#       'masksub' is to replace default MaskSub function with custom function (see below). Default is "MaskSub" with '.flipvertical'
#       'progress' controls whether to print processing progress to console
#   * RenderSRT(clip hstack, string srtfile, [string masksub])
#       renders a srt/ass subtitle file (combined with .depths file) over the clip. For frames that does not have a record
#       in the .depths file no subtitle is rendered (for process speed up)
#
# tune internals:
#     SetDepthComputationAlg(string luafile = "sub3D.lua")
#       sets the algorithm for depth computation. Lua file must contain 2 global functions.
#         function CalcForFrame(framenum)
#       should compute the depth value for each frame individually.
#       The function is ignored -- use Lua global variables to store intermediate results.
#         function CalcForSub(startframenum, lengthinframes)
#       the return value of this function must be an integer (it will be used as depth value for subtitle at rendering stage)
#       or return nil if you want to skip this subtitle.
#       The plugin uses Lua 5.3.2 internally. All default lua libraries are available (and you can require anything).
#       The plugin sets some lua global values. See sub3d.lua for details.
#       The lua "print" function respects AviSynth "SetLogger" parameters.
#
# utilities:
#   * SequentialToSeekable(string command) : clip
#       makes the video stream returned by the command seekable even when the command produce a non-seekable clip.
#       if you try to seek forward, it will read and skip all the frames up to the requested frame;
#       if you try to seek backwards, it will restart the command to recreate the stream and skip the unnecessary frames.
#       Example: SequentialToSeekable("""DGMVCSource("avc.264", "mvc.mvc", view = 0, frames = 6935, hw = 0)""")
#       Note the """ enclosing the command, necessary if the command has strings with quotes.
#   * SetLogger([string filename = ""], [bool console = false])
#       set logging style for Sub3D plugin. _filename_ is the name of the log file.
#       Set filename to "nul" to hide all messages. _console_ controls wheather to create
#       a console window for the plugin's log messages (it's useful when you debug your depths with some GUI).
#       Empty value in _filename_ means that output will be directed to stdout or to the console window.
#       Do not call this function to not touch the system defaults (stdout, stderr).
#   * RestoreAlpha(string renderer, clip reference)
#       This is a complex one. This function accepts other function name in 'renderer' parameter.
#       The "renderer" function should have this prototype:
#           function MyRendererXXX(clip param)
#       RestoreAlpha will call "renderer" function 3 times with different clip as a parameter.
#       The "renderer" function should render subtitles over 'param' clip.
#       RestoreAlpha will return clip with restored alpha channel (similar to MaskSub result).
#       'reference' parameter will be used as a reference for creation special clips.
#       'reference' clip will not be rendered. Only its VideoInfo descriptor will be used (similar to BlankClip).
#       For example see sub3d-3.avs
#
# NOTE: Avisynth function names are case insensitive. If you want to
# use a function in a specific dll you can use the DllName_FunctionName naming scheme.
# Example: SequentialToSeekable or Sub3D_SequentialToSeekable functions are similar.

# LoadHelper plugin.
# Changes process PATH environment varible, so that all dlls necessary to the avs script and specified without the full path
# and located in the same dir as loadhelper.dll will be found and loaded. Useful tool for complex plugins that depends on
# third-party dlls.  It's necessary to load sub3d.dll, as otherwise vcomp140.dll will not be found.

LoadPlugin("c:\Program Files (x86)\ssifSource\bin\LoadHelper.dll")
LoadPlugin("sub3d.dll")

# Optionally, specify the lua script to load
# Under the hood: the file extension (if specified) will be ignored. The function will add search path to
# Lua search path (package.path) if filenane with full path specified. And then call Lua "require" function
# for a filename (without extension).
# (If not specified, it will use sub3d.lua from the directory containing sub3d.dll)

SetDepthComputationAlg("sub3d.lua")

# DirectVobSub vsfilter.
# This filter is required for CalcSRTDepths function. CalcSRTDepths calls MaskSub().
# Note: You can even define your own MaskSub function with same proto.
# The function should render RGB32 clip (with Alpha channel present!).
# This clip must contain only text subtitles over a black background.
# Right now MaskSub is called like this:
#     MaskSub(srtfile, hstack.width / 2, hstack.height, hstack.framerate, hstack.framecount) \
#         [.flipvertical (for missing masksub parameter only)]

LoadPlugin("VSFilter.klite.dll")



# show plugin output in separate console
SetLogger(console = true)

# source video filename
vid = test_dir + "SupTitle_Sample\video_Full-SBS.mkv"

# Note that the input video must be in Full-SBS format, and in RGB32
clip = ffmpegsource2(vid).converttorgb32("PC.709")



# XML rendering

# the xml file should be in the directory with subtitle .png images
sub = test_dir + "SupTitle_Sample\XML-PNG\FRA_2D.xml"
clip.CalcXMLDepths(sub)
out = clip.RenderXML(sub)



# SRT rendering

sub = test_dir + "MM1_test\MM1_ANSI.srt"
clip.CalcSRTDepths(sub)
out = clip.RenderSRT(sub)



# Convert RGB32 clip back to YV12 (if necessary)
#out = out.ConvertToYV12(matrix = "PC.709")

return out



# skip rendering (if you need only to add the depth tags in the XML file
# or to create the .depths file for SRT) 

#clip.BlankClip(length=1) # renders clip with only one black frame
