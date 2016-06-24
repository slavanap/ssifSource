test_dir = "c:\Users\Vyacheslav\Projects\ssifSource\test sequences\MM1_test\"
vid = test_dir + "MM1-001.mkv"
sub = test_dir + "MM1_ANSI.srt"

LoadPlugin("c:\Program Files (x86)\ssifSource\bin\LoadHelper.dll")
LoadPlugin("sub3d.dll")
LoadPlugin("VSFilter.klite.dll")

#sub3D_setlogger(console = true)

hou = FFMpegSource2(vid).AssumeFPS("ntsc_film").KillAudio
left  = Crop(hou, 0, 0, 0, hou.height/2).BicubicResize(hou.width, hou.height).converttorgb32("PC.709")
right = Crop(hou, 0, hou.height/2, 0, 0).BicubicResize(hou.width, hou.height).converttorgb32("PC.709")
hstack = stackhorizontal(left, right)

Exist(sub + ".depths") ? nop : hstack.CalcSRTDepths(sub)
o = hstack.RenderSRT(sub)

# interleave

l = o.crop(0,0,o.width/2,o.height).subtitle("left")
r = o.crop(o.width/2,0,o.width/2,o.height).subtitle("right")
return interleave(l,r)
