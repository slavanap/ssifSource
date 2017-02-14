test_dir = "x:\@_Research Material\3D\3D Video\"

vid = test_dir + "SupTitle_Sample\video_Full-SBS.mkv"
global sub = test_dir + "SupTitle_Sample\FRA_2D.sup"

LoadPlugin("c:\Program Files (x86)\ssifSource\bin\LoadHelper.dll")
LoadPlugin("sub3d.dll")
LoadPlugin(test_dir + "SupTitle_Sample\SupTitle.dll")

#sub3D_setlogger(console = true)

c = FFMpegSource2(vid).ConvertToRgb32("PC.709").trim(0,2000)

# global specifier is need because we gonna use it in avisynth function (see below)

# this function renders our subtitle over 'param' clip
# this function will be used as an argument to Sub3D_RestoreAlpha, that restores alpha
# channel for RGB32 by rendering subtitle over several backgrounds

function SuptitleExtract(clip param) {
    # these conversions is pretty slow, but what to do...
    return param.ConvertToYV12.SupTitle(sub).ConvertToRgb32
}

# global specifier is need because we gonna use it in avisynth function (see below)
global left = c.crop(0,0,c.width/2,c.height)

# note: no vsfilter is loaded right now, we define MaskSub function ourselves!
# only Sub3D_CalcSRTDepths gonna use it though
function MyMaskSub(string srtfile, int width, int height, float fps, int numframes) {
    Assert(width == left.width && height == left.height && fps == left.framerate && numframes == left.framecount, "Something happened")
    return Sub3D_RestoreAlpha("SuptitleExtract", left)
}


# SRT rendering

# you can set any name below instead of 'sub', the file (name + ".depths") will be created and used in rendering
Exist(sub + ".depths") ? NOP : c.Sub3D_CalcSRTDepths(sub, masksub = "MyMaskSub")
o = c.Sub3D_RenderSRT(sub, masksub = "MyMaskSub")

# interleave

l = o.crop(0,0,o.width/2,o.height).subtitle("left")
r = o.crop(o.width/2,0,o.width/2,o.height).subtitle("right")
return interleave(l,r)
