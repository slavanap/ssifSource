LoadPlugin("Release\cropdetect.dll")

FFMpegSource2("sample.mkv")
# CropDetect(clip, outfile, threshold, num_frames)
#   clip       -- the video for crop detection
#   outfile    -- filename to store the crop. Output file is AviSynth compatible
#   threshold  -- detection threshold - higher values will result more regions been cropped. Can be omitted, default is 24.
#   num_frames -- number of frames to detect crop for. These frames is uniformly selected from source clip. Can be omitted, default is 20.

CropDetect("sample.crop", 24, 20)
Import("sample.crop")
