Helper to decode BD3D with different ways. ssifSource preferes opensource solutions usage.
ssifSource is implemented as AviSynth plugin.

Decoding pipelne looks as follow:
* MPLS file reading and parsing
* SSIF file demuxing (MpegSource - opensource, modified)
* Combining 2 streams AVC & MVC into one (MVCCombine, proprietary)
* Video decoding (ldecod or intel decoder - both opensource)

You can attach files to any ssifSource layer.
mplsSource accepts .mpls files, ssifSource accepts .ssif files or 2 streams (avc & mvc), or even one combined stream.
The plugin uses Windows pipes to connect all executables in pipeline. This adds a little overhead,
but allows neither change original apps sources nor use additional disk space.

TODO:
1. AVISynth Audio support
2. DirectShow FileAsync implementation through libdvdread
3. MVCombiner replacement to opensource one
