-- Settings
local WRITE_OFS_FILE = false;

-- debug info
print("Global variables defined by sub3d.dll:");
function print_var(t)
    local v = _G[t];
    print(string.format(" %15s = %s (%s)", t, tostring(v), tostring(type(v))));
end
print_var("XML_FILE");
print_var("SRT_FILE");
print_var("FRAMECOUNT");
print_var("FRAMERATE");
print_var("FPS_NUMERATOR");
print_var("FPS_DENOMINATOR");


local subfile;

if (XML_FILE) then
    print("Processing xml file: "..XML_FILE);
    subfile = XML_FILE;
elseif (SRT_FILE) then
    print("Processing srt/ass file: "..SRT_FILE);
    subfile = SRT_FILE;
end

local f3dp;

-- write .ofs header
if (WRITE_OFS_FILE) then
    -- open binary file
    f3dp = assert(io.open(subfile..".ofs", "wb"));
    -- signature
    f3dp:write(string.char(0x89, 0x4f, 0x46, 0x53, 0x0d, 0x0a, 0x1a, 0x0a));
    -- version
    f3dp:write("0100");
    -- guid. I'm about to calculate it as simple hash from subtitle filename.
    local hash = {};
    local idx = 0;
    for c in subfile:gmatch(".") do
        hash[idx+1] = (hash[idx+1] or 0) + string.byte(c);
        idx = (idx + 1) % 16;
    end
    for i=1,16 do
        f3dp:write(string.char((hash[i] or 0) & 0xFF));
    end
    -- framerate
    local fps = { 23.976, 24, 25, 29.97, 50, 59.94 };
    local selected = #fps;
    for i=1,#fps do
        if (fps[i] > FRAMERATE) then
            selected = i;
            break;
        end
    end
    if ((1 < selected) and (FRAMERATE - fps[selected-1] < fps[selected] - FRAMERATE)) then
        selected = selected - 1;
    end
    if (selected >= 5) then
        selected = selected + 1;
    end
    print("OFS HEADER: fps="..tostring(fps[selected])..", AviSynth fps="..tostring(FRAMERATE));
    local byte = selected * 0x10;
    -- drop_frame_flag
    if (selected == 4) then
        byte = byte + 1;
    end
    f3dp:write(string.char(byte));
    -- number_of_rolls
    f3dp:write(string.char(1));
    -- reserved
    f3dp:write(string.char(0, 0));

    -- marker_bits
    f3dp:write(string.char(0));
    -- start_timecode. we always start from first frame
    f3dp:write(string.char(0,0,0,0));
    -- number_of_frames. ofs format uses big-endian numbers
    f3dp:write(string.char((FRAMECOUNT >> 24) & 0xFF, (FRAMECOUNT >> 16) & 0xFF, (FRAMECOUNT >> 8) & 0xFF, FRAMECOUNT & 0xFF));
end


if (WRITE_OFS_FILE) then 
    local lastframenum = -1;                            -- last written frame number (stores this value between function calls)

    function ofs_writedepth(framenum, length, depth)
        print("writedepth("..tostring(framenum)..","..tostring(length)..","..tostring(depth)..")");
        local diff = framenum - lastframenum;
        assert(diff > 0, "We don't support backward writings");
        if (diff > 1) then
            f3dp:write(string.char(0x80):rep(diff-1));  -- create string of undefined depth values (0x80) with length (diff - 1) and write it to the file
        end
        depth = math.floor((depth + 1) / 2);            -- get away internal *2
        depth = math.max(-127, math.min(depth, 127));   -- apply limits
        if (depth < 0) then                             -- reinterpret_cast -127..127 -> 0..127,129..255
            depth = -depth + 0x80;                      -- 0x81 is for -1, 0x82 is for -2, ..., 0xFF is for -127 (.ofs format depth presentation)
        end
        f3dp:write(string.char(depth):rep(length));     -- create string with 'depth' bytes with length (length) and write it to the file
        lastframenum = framenum + length - 1;           -- last written framenum
        f3dp:flush();                                   -- flush written bytes to disc (optional)
    end

    function ofs_close()
        if (lastframenum < FRAMECOUNT-1) then
            ofs_writedepth(FRAMECOUNT-1, 1, 0);
        end
        assert(f3dp:close());               -- close file (and check for errors)
    end
else
    -- stubs
    local stub = function() end
    ofs_writedepth = stub;
    ofs_close = stub;
end





local depths = {};

function CalcForFrame(framenum)
--[[
    local raw = GetFrameRawData();
    print("raw depths");
    for _,v in ipairs(raw) do
        print(string.format("depth %3d conf %5.3g count %4d", -v.depth, v.conf, v.count));
    end
]]
    -- 0.8 at line below is confidence threshold (0.0 .. 1.0) : lesser values add more noise to depth results
    -- if not specified, 0.0 value is used
    local min_depth, max_depth, mean_depth, min_confidence, max_confidence, mean_confidence = CalcForFrameRaw(0.8);
    if (min_depth == nil) then
        print(string.format("debug %3d -- no depth for frame", framenum));
        return;
    end
    local res_depth = math.floor(-(mean_depth + min_depth) / 2);
    print(string.format("debug %3d front %3d back %3d mean %7.3g res %3d conf %g %g %g",
        framenum, -min_depth, -max_depth, -mean_depth, res_depth,
        min_confidence, max_confidence, mean_confidence));
    depths[#depths+1] = res_depth;
end

function CalcForSub(startframenum, lengthinframes)
    table.sort(depths);
    local index = math.floor(#depths / 2) + 1;
    local value = depths[index] or 0;
    print(string.format("depths: sub started at frame #%d has depth value %d", startframenum, value));
    ofs_writedepth(startframenum, lengthinframes, value);
    depths = {};    -- clear depths table for next subtitle processing
    return value;
end

function OnFinish()
    ofs_close();
    print("Processing finished");
end
