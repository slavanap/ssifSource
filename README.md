This project contains a collection of AviSynth plugins.

* ssifSource - for reading BD3D,
* Sub3D - for rendering 3D subtitles and estimating they depth (partially closed-source),
* LoadHelper - for easy loading AviSynth plugins that requires additional .dll files,
* FilmTester - for reading through .avs file frame by frame (without a need of encoding with x264, for example)
* and others. 


**ssifSource** supports can open .ssif files or .mpls files for rendering. It uses other thirdparty software to make a pipeline for BD3D decoding without a need to write intermediate files on disk. The plugin uses Windows pipes to connect all executables in pipeline. This adds a little overhead, but allows neither change original apps sources nor use additional disk space. For more info, see [its own help file](https://github.com/slavanap/ssifSource/blob/master/ssifSource/ssifSource.avs)

**Sub3D** supports .srt subtitles, .xml/png subtitles and other AviSynth plugins that can render subtitles over casual 2D video. It is able to extract subtitles from 2D video (if original is available), compute their depth (according 3D video) and render them over 3D video. Plugin works in RGB32 colorspace. For more info, see [sample .avs file](https://github.com/slavanap/ssifSource/blob/master/Sub3D/sub3d.avs)

**LoadHelper** simple plugin that adds its own directory to process PATH variable when the plugin is loaded. Thus every .dll within LoadHelper.dll directory will be loaded properly if it is required by some other plugins. Currently the plugin is used in [BD3D2MK3D](http://forum.doom9.org/showthread.php?t=170828)

**FilmTester** simple tool that tests .avs file for readability.

**Additinally** Sub3D.dll plugin contains some other interesing functions:
* *CropDetect* - detects a clipbox of video and generates .avsi file for easy cropping.
* *SequentialToSeekable* - converts any AviSynth plugin that outputs its result sequentially to a seekable plugin. Beware, if you seek backwards, the dependent plugin will be reloaded because seeking is emulated and slow.
* *HistogramMatching* - tries to make color histogram of input clip similar to reference clip frame by frame.
