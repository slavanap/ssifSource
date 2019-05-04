[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/05825vxmi4nweh8e/branch/master?svg=true)](https://ci.appveyor.com/project/slavanap/ssifsource/branch/master)

> If you like the stuff, you may [donate me](https://rocketbank.ru/vyacheslav-napadovsky-dark-frog).

Collection of AviSynth plugins
------------------------------

* ssifSource - for reading BD3D,
* Sub3D - for rendering 3D subtitles and estimating their depth,
* LoadHelper - for easy loading AviSynth plugins that requires additional .dll files,
* FilmTester - for reading through .avs file frame by frame
* and others. 


**ssifSource** is able to render .ssif and .mpls files from BD3D. Rendering directly from ISO files is also supported. The plugin uses thirdparty software to build a pipeline for BD3D decoding without a need for intermediate files. The plugin uses Windows pipes to connect all executables. This adds a small overhead, but allows neither change original apps nor use additional storage. See [sample avs file](https://github.com/slavanap/ssifSource/blob/master/ssifSource/ssifSource.avs) for more info.

**Sub3D** can execute any plugin that renders subtitles over 2D video and then convert them to 3D subtitles according depth analysis results. Estimated depth can be adjusted with Lua scripts. The plugin also supports .srt and .xml subtitles as an input. For other subtitle renderers there's a function for alpha channel extraction. The plugin works only in RGB32 colorspace. See [sample avs file](https://github.com/slavanap/ssifSource/blob/master/Sub3D/sub3d.avs) for more info.

**LoadHelper** when loaded, adds its own path to process PATH environment variable. This allows other .dll files that are in the same folder as LoadHelper.dll to be loaded properly (and without full path specification) if they are required by some other plugins. Currently the plugin is used in [BD3D2MK3D](http://forum.doom9.org/showthread.php?t=170828)

**FilmTester** simple tool that tests .avs file for readability.

**Additinally** `Sub3D.dll` contains functions for:
* *CropDetect* - detects a clipbox of video and generates .avsi file for easy cropping.
* *SequentialToSeekable* - converts any AviSynth plugin that outputs its result sequentially to a seekable plugin. Beware, if you seek backwards, the dependent plugin will be reloaded because seeking is emulated.
* *HistogramMatching* - tries to make color histogram of input clip similar to reference clip, frame by frame.
* *VideoCorrelation{,Preprocess,GetShift}* - merge video sequence with its subsequence by video frame data when sequences alignment isn't known.

**AvsTools** static library contains more video processing alrogithms.
