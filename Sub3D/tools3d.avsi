
# Useful AviSynth function to convert 3D video from any to any format.
# For example, if you want to get half top/bottom from half left/right, try
# src.Convert3D(FORMAT_HALFTNB, FORMAT_HALFSBS)

global FORMAT_FULLSBS = 1       # Full side by side
global FORMAT_FULLTNB = 2       # Full top and bottom
global FORMAT_HALFSBS = 3       # Half side by side
global FORMAT_HALFTNB = 4       # Half top and bottom
global FORMAT_INTERLEAVED = 5   # Interleaved: left frame first (non-odd frame number), next right frame (odd frame number)

function Convert3D_Rescale(clip view, int new_width, int new_height) {
    return view.BicubicResize(new_width, new_height)
}

function Convert3D_IsSBS(int format) {
    return (format == FORMAT_FULLSBS) || (format == FORMAT_HALFSBS)
}

function Convert3D_IsHalf(int format) {
    return (format == FORMAT_HALFSBS) || (format == FORMAT_HALFTNB)
}

function Convert3D_Dim(clip c, int from, int to, bool isWidth) {
    assert(from != to)
    return \
        !Convert3D_IsHalf(from) ? ( \
            !Convert3D_IsHalf(to)  ? (isWidth ? c.width     : c.height) : \
            (to == FORMAT_HALFSBS) ? (isWidth ? c.width / 2 : c.height) : \
                                     (isWidth ? c.width     : c.height / 2) \
        ) : \
        !Convert3D_IsHalf(to) ? ( \
            (from == FORMAT_HALFSBS) ? (isWidth ? c.width * 2 : c.height) : \
                                       (isWidth ? c.width     : c.height * 2) \
        ) : \
        (from == FORMAT_HALFSBS) ? \
            (isWidth ? (c.width * 2) : (c.height / 2)) : \
            (isWidth ? (c.width / 2) : (c.height * 2))
}

function Convert3D_Helper(clip c, int from, int to, bool swap) {
    assert(from != to)
    assert( \
        (from == FORMAT_INTERLEAVED) \
        || ((Convert3D_IsSBS(from) ? c.width : c.height) % 2 == 0), \
        "Can't split clip to equal parts. Please check clip resolution" \
    )
    left = (from == FORMAT_INTERLEAVED) ? c.SelectEven : \
        Convert3D_IsSBS(from) ? c.crop(0,0,c.width/2,c.height) : c.crop(0,0,c.width,c.height/2)
    right = (from == FORMAT_INTERLEAVED) ? c.SelectOdd : \
        Convert3D_IsSBS(from) ? c.crop(c.width/2,0,c.width/2,c.height) : c.crop(0,c.height/2,c.width,c.height/2)

    resize = Convert3D_IsHalf(from) || Convert3D_IsHalf(to)
    new_width = Convert3D_Dim(left, from, to, true)
    new_height = Convert3D_Dim(left, from, to, false)

    left = resize ? left.Convert3D_Rescale(new_width, new_height) : left
    right = resize ? right.Convert3D_Rescale(new_width, new_height) : right

    left = swap ? right : left
    right = swap ? left : right

    return (to == FORMAT_INTERLEAVED) ? Interleave(left,right) : \
        Convert3D_IsSBS(to) ? StackHorizontal(left,right) : StackVertical(left,right)
}

function Convert3D(clip from, int from_format, int to_format, bool "swap_views") {
    swap_views = Default(swap_views, false)
    return (from_format == to_format) \
        ? from \
        : Convert3D_Helper(from, from_format, to_format, swap_views)
}
